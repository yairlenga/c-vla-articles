# Temporary Memory Isn’t Free: Allocation Strategies and Their Hidden Costs

## Introduction

In many discussions, memory allocation is treated as an O(1) operation — a constant-time primitive that can be safely ignored in performance-critical code. In practice, that constant can be surprisingly large. Allocating memory may involve managing free lists, splitting or merging blocks, or occasionally requesting additional pages from the operating system. These costs are usually hidden behind a fast path, but when allocation happens repeatedly inside tight loops, the “constant time” assumption starts to leak.

This becomes particularly relevant in financial analytics, where temporary memory is not just a convenience but often a requirement. Models are typically structured as independent components, each producing intermediate results that must be preserved for auditability, explainability, or reuse in downstream calculations. A typical valuation may generate multiple time series — such as scheduled principal, prepayments, interest, and losses — which are then aggregated or fed into other models. This separation improves modularity and traceability, but it also means that even relatively simple calculations rely on temporary arrays that are allocated and discarded repeatedly.

As a result, allocation patterns that might seem avoidable in theory become common in practice — especially when applied across large portfolios, where these temporary structures are created thousands or millions of times.

---

## A Simple Use Case: Loan Portfolio PV Calculation

A typical loan valuation computes cashflows over a time grid (often daily or monthly) from origination to maturity. For each time step, the model derives scheduled principal, interest, and expected losses based on the current balance and assumptions such as prepayment and default rates. These values are usually stored in arrays, both to support multi-pass calculations (e.g., aggregation, stress adjustments) and to provide a full audit trail of intermediate results. The discounted present value is then obtained by applying a corresponding discount factor curve to each time step.

### Per-loan processing

Each loan is evaluated independently over a time grid (daily, simplifying into 12 months X 30 days each) from origination to maturity. At each step, the model updates the outstanding balance and computes the corresponding cashflows based on contractual terms and behavioral assumptions (e.g., prepayments, defaults).

### Arrays over time: `S, P, I, L, DF`

The calculation typically materializes several time series, coming from different models.

- `S[t]` — Schedule payments  - Cash Flow model
- `I[t]` — interest payments  - Cash Flow model
- `U[t]` — Unscheduled payments (pre-pays) - Prepay model
- `L[t]` — losses (defaults / write-offs) - Loss model.
- `DF[t]` — discount factors - Interest rate model

These arrays are indexed by time and span the full horizon of the loan. In our example, we will cap the horizon to 50 years.

### Temporary workspace per loan

Even when only the final present value is required, intermediate results are usually stored in arrays. This supports:
- multi-pass calculations (generation → aggregation → adjustments)
- scenario or stress overlays
- auditability and explainability of results

In practice, this means allocating a working set proportional to the number of time steps for each loan.

## A Reasonable Implementation

This implementation is representative of real-world code:

- Each loan is processed independently  
- Temporary arrays (`S, P, I, L`) are allocated per loan  
- Discount factors are computed once and reused  

Nothing here looks unusual or inefficient.  
The expectation is that allocation overhead is small compared to the numerical work.

This assumption is what we test next.

```c
static struct portfolio_result
port_pv_heap_per_loan(int loans, int sim_days)
{
    struct portfolio_result res = {0};

    double *DF = xmalloc(STATIC_MAX_DAYS * sizeof(*DF));
    calc_DF(sim_days, DF, 5.0);

    for (int loan = 0; loan < loans; ++loan) {

        struct loan_info info = get_loan_info(loan, sim_days);
        int loan_days = info.days;

        double *P = xmalloc(loan_days * sizeof(*P));
        double *S = xmalloc(loan_days * sizeof(*S));
        double *I = xmalloc(loan_days * sizeof(*I));
        double *L = xmalloc(loan_days * sizeof(*L));

        model_loan(&info, S, P, I, L);
        res.pv += loan_pv(loan_days, S, P, I, L, DF);

        free(L);
        free(I);
        free(P);
        free(S);
    }

    free(DF);
    return res;
}
```

# The loan modeling code

The modeling logic itself is straightforward and operates over the provided workspace:

```c
static void model_loan(
    const struct loan_info *loan,
    double *S,
    double *P,
    double *I,
    double *L
) {
    // Setup
    double bal = ...

    // Cash flows
    for (int t = 0; t < loan->days; ++t) {
        // Calculate scheduled_principal, prepay, intrest and losses
        S[t] = scheduled_principal ;
        P[t] = prepay;
        I[t] = interest;
        L[t] = loss;
    }

}
```
---

## Allocation Strategies

To understand the impact of allocation, we compare several strategies that differ only in how temporary memory is managed. The computation itself is identical in all cases.

### Static reusable buffers (reference)

A single set of static buffers is allocated once and reused across all loans.

This approach has effectively **zero allocation overhead** during the benchmark and serves as the reference point. It represents the best-case scenario where memory management is fully amortized and removed from the hot path.

### VLA (stack allocation)

Temporary arrays are allocated on the stack per loan using Variable Length Arrays (VLA).

This avoids heap allocation entirely and keeps allocation cost very low and predictable. However, it is constrained by stack size and may require safeguards for large problem sizes.

### Heap allocation per loan (`malloc` / `free`)

Each loan allocates its own working arrays using `malloc` and releases them after processing.

This is the most straightforward and modular approach, but it introduces allocation overhead directly into the hot loop and stresses the allocator under repeated use.

### Heap reuse (per portfolio)

Buffers are allocated once per portfolio (or per thread) and reused across loans. Per-loan data is allocated to the maximum possible size.

This removes most allocation overhead while preserving flexibility. It is a common compromise in performance-sensitive systems, but make the code less modular - callers to the financial engine have to anticipate workspace requirements.

### Bulk allocation

A single large block is allocated and partitioned into the required arrays (`S, P, I, L, DF`).

This reduces the number of allocation calls and improves locality, but still relies on the heap allocator and may incur setup cost.

---

In all cases, the only difference is how memory is obtained and released.  
This allows us to isolate the cost of allocation itself.

---

## Benchmark Design

## Benchmark Design

To capture allocator behavior across realistic environments, we run the same benchmark under several common compiler and allocator combinations:

- **GCC + glibc (default Linux)**  
  Baseline configuration used in most production Linux systems.

- **Clang + glibc**  
  Same allocator, different compiler — highlights code generation effects independent of allocation.

- **GCC + musl**  
  Lightweight allocator commonly used in containers (e.g., Alpine); known for different performance trade-offs.

- **GCC + mimalloc**  
  Modern allocator optimized for fast paths and low fragmentation, widely used in performance-sensitive systems.

- **GCC + jemalloc**  
  Mature allocator with strong scalability and fragmentation control, used in databases and large-scale services.

- **GCC + tcmalloc**  
  Google’s allocator, optimized for high-throughput multi-threaded workloads.

All tests are run in release mode with optimizations enabled, with minimal run-time checks.
No debugging, tracing, or instrumentation features are active in any allocator - using default "out-of-the-box" setting.

All measurements are performed in a single-threaded application to keep the analysis focused and comparable. This isolates allocation costs without introducing contention or synchronization effects. In multi-threaded workloads, allocator behavior becomes more complex, and additional overheads — such as thread-local cache management, cross-thread frees, and synchronization — can introduce further runtime penalties. As a result, the single-threaded results presented here should be viewed as a lower bound on allocation cost.


---

# Results

The choice of allocation strategy has a measurable impact on performance — even for relatively simple computations.

In our benchmark, per-loan allocation using `malloc` is up to **2.5x slower** than reusing memory, while stack-based approaches (VLA) remain close to the optimal baseline. When using "simple" allocators (musl) the cost of using malloc can be as high as **6x slower**.

## Throughput

The following table summarize relative throughput, when running the simulation on a portfolio of 1000 loans with durations between 3000 and 12000 days (approxiatley 9 and 33.5 years). Average result, as reported by the program over 10 runs each.

To replicate `./alloc-bench 0 1000 12000`

*Normalized to static = 100% (higher is better)*

| Strategy (relative) | glibc | clang | musl  | mimalloc | jemalloc | tcmalloc |
|---------------------|-------|-------|-------|----------|----------|----------|
| static              | 100%  | 101%  |  99%  | 100%     | 100%     | 100%     |
| vla                 | 100%  | 103%  |  99%  |  99%     |  99%     |  99%     |
| heap (reuse)        |  95%  |  98%  |  94%  |  88%     | 100%     |  87%     |
| heap (per loan)     |  37%  | 100%  |  16%  |  87%     |  95%     |  86%     |
| bulk                | 100%  | 102%  |  18%  |  99%     |  98%     | 100%     |


---

## Key Observations

Several patterns stand out from the results:

- **VLA remains consistently close to the theoretical limit**  
  Across all configurations, stack allocation (Fixed size and VLA) performs very close to the static baseline — effectively the theoretical speed limit where allocation cost is eliminated. It also shows minimal variability, making it both fast and predictable.

- **Allocators are tuned, not universal**  
  Each allocator performs well under certain allocation patterns, but it is easy to fall outside its “comfort zone.” - even it "reasonable" implementations of code. Repeated allocation/free in tight loops can expose slow paths and lead to significant performance penalties.

- **Simple allocators can struggle under pressure**  
  The musl allocator, while lightweight and predictable, shows the weakest performance in allocation-heavy scenarios. Its simplicity becomes a disadvantage when faced with frequent, repeated allocations. This may be important with cloud deployment (K8S, etc) - which often opt for light weight libraries (On Alpine, etc.)

- **Optimized allocators still have weak spots**  
  More sophisticated allocators (mimalloc, jemalloc, tcmalloc) generally perform better, but not uniformly. Some allocation patterns are handled almost for free, while others incur noticeable overhead. Performance is highly pattern-dependent.

- **The compiler matters too**  
  Even with the same allocator (glibc), compiler choice has an impact. Clang shows slightly better and more stable performance compared to GCC, suggesting that code generation and optimization influence how allocation costs are expressed.

Overall, allocation cost is not just about the allocator — it is the interaction between allocator, allocation pattern, and compiler.

---

## Caveats

- Results are from a single-threaded benchmark; multi-threaded workloads may introduce additional allocator overhead  
- Stack-based approaches (VLA) are limited by available stack size and may require safeguards for large inputs  
- The model materializes intermediate arrays for clarity and auditability; other designs may reduce allocation at the cost of complexity  

---

## Conclusion

## Practical Takeaways

The performance upside is not academic.

In simulation-heavy workloads — financial models, risk scenarios, Monte Carlo, or any system that repeatedly builds temporary state — allocation sits directly in the hot path. Small per-call overheads accumulate quickly, and differences of 2–3× at the allocation level can translate into meaningful end-to-end impact.

Stack-based allocation (VLA) offers a simple way to approach the lower bound: allocation cost that is effectively constant and close to zero. In our results, VLA consistently tracks the static baseline, providing both speed and stability.

Used responsibly, VLA does not have to be an all-or-nothing choice. A common pattern is to use a **conditional approach**:

- allocate on the stack for small, bounded sizes  
- fall back to heap allocation for larger inputs  

This provides predictable performance while avoiding stack overflow risks.

It is certainly possible to tune allocator behavior, adjust parameters, or carefully shape allocation patterns. However, these approaches add complexity and are often allocator-specific. In many cases, a simple conditional VLA/heap strategy achieves comparable or better results with significantly less effort.

The takeaway is straightforward:

> If temporary memory sits in your hot path, how you allocate it matters — and simple strategies can go a long way.