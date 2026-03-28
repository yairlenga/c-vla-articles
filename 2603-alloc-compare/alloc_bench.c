/*
 * alloc_bench.c
 *
 * Benchmark temporary-array allocation strategies for a synthetic
 * portfolio PV-style workload.
 *
 * Strategies:
 *   0 = heap per loan      : malloc/free P,I,L,DF on every loan
 *   1 = heap per portfolio : malloc/free once, reused across loans
 *   2 = static reusable    : fixed-capacity reusable arrays
 *   3 = VLA                : stack arrays per loan (if supported)
 *
 * Synthetic workload:
 *   For each loan and each day:
 *     S[t] = Scheduled principal payments
 *     U[t] = Unscheduled principal payment
 *     I[t] = interest payment
 *     L[t] = losses
 *     DF[t] = discount factor
 *   Then aggregate PV over the horizon.
 *
 * Compile examples:
 *   gcc -O3 -march=native -std=c11 -Wall -Wextra alloc_bench.c -lm -o alloc_bench
 *   clang -O3 -march=native -std=c11 -Wall -Wextra alloc_bench.c -lm -o alloc_bench
 *
 * For mimalloc (Linux example):
 *   gcc -O3 -std=c11 alloc_bench.c -lm -o alloc_bench_mi -lmimalloc
 *
 * For musl:
 *   musl-gcc -O3 -std=c11 alloc_bench.c -lm -o alloc_bench_musl
 *
 * Usage:
 *   ./alloc_bench [strategy] [loans] [days] [passes] [seed]
 *
 * Example:
 *   ./alloc_bench 0 20000 3600 5 12345
 *   ./alloc_bench 1 20000 3600 5 12345
 *   ./alloc_bench 2 20000 3600 5 12345
 *   ./alloc_bench 3 20000 3600 5 12345
 *
 * Strategies:
 *    Static - use static arrays defined to MAX
 *    VLA - allocate all array on stack
 *    HEAP - Allocate all array with malloc
 *    HEAP/REUSE - allocate all array at max size with malloc - reuse array
 *    HEAP/BULK - Allocate big structure with all arrays in one call.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <errno.h>


#ifndef STATIC_MAX_DAYS
#define STATIC_MAX_DAYS (50*12*30)
#endif

#ifndef DEFAULT_LOANS
#define DEFAULT_LOANS 1000
#endif

#ifndef DEFAULT_DAYS
#define DEFAULT_DAYS 3600
#endif

#ifndef DEFAULT_PASSES
#define DEFAULT_PASSES 10
#endif

#define INT_RATE (5.0/100.0)

enum alloc_strategy {
    STRAT_ALL = 0,
    STRAT_STATIC_REUSE,
    STRAT_VLA,
    STRAT_HEAP,
    STRAT_HEAP_REUSE,
    STRAT_HEAP_BULK,
};

static const char *strategy_name[] = {
    [STRAT_HEAP] = "heap/loan",
    [STRAT_HEAP_REUSE] = "heap/reuse",
    [STRAT_VLA] = "vla",
    [STRAT_STATIC_REUSE] = "static",
    [STRAT_HEAP_BULK] = "heap/bulk",
} ;

static double now_seconds(void) {
    struct timespec ts;
#if defined(CLOCK_MONOTONIC)
    clock_gettime(CLOCK_MONOTONIC, &ts);
#else
    clock_gettime(CLOCK_REALTIME, &ts);
#endif
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* Small deterministic PRNG: fast, no libc rand() noise */
static inline uint64_t splitmix64_next(uint64_t *state) {
    uint64_t z = (*state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static uint64_t rng_state ;
static inline double rand_val(double low, double high)
{
    static uint64_t rand_cycle = ((uint64_t) 1) <<48 ;
    double rand01 = (splitmix64_next(&rng_state) & (rand_cycle-1)) / (double) rand_cycle ;
    return low + (high-low) * rand01 ;
}

static inline double clamp_nonneg(double x) {
    return x < 0.0 ? 0.0 : x;
}

static void calc_DF(int max_t, double *DF, double rate)
{
    for(int t=0 ; t<max_t ; t++ ) {
        DF[t] = pow(1+rate/100.0, t/360.0) ;
    }
}

struct loan_info {
    int id ;
    int days ;
    double orig_bal ;
    double coupon ;
} ;

struct portfolio_result {
    double pv ;
} ;

static struct loan_info get_loan_info(int loanid, int sim_days)
{
    struct loan_info info = {} ;
    info.id = loanid ;
    info.days = (int) rand_val(sim_days*0.25, sim_days) ;
    info.orig_bal = rand_val(50000.0, 500000.0) ;
    info.coupon = rand_val(0.02, 0.10) ;
    return info ;
}

/*
 * Fill the arrays with synthetic loan-like dynamics and return a PV-like sum.
 * This intentionally uses all arrays so the optimizer has less room to
 * collapse the work.
 */
static void model_loan(
    const struct loan_info *loan,
    double *S,
    double *P,
    double *I,
    double *L
) {
    double base_balance = loan->orig_bal ;
    double coupon = loan->coupon ;
    double hazard = rand_val(0.00001, 0.0002) ;
    double daily_rate = coupon / 360.0;
    double prepay_rate = rand_val(0.00005, 0.00025) ;

    double bal = base_balance;
    double inv_days_p50 = 1.0/(loan->days + 50) ;

    /* First pass: generate time series */
    for (int t = 0; t < loan->days; ++t) {

        double loss = bal * hazard ;
        if ( loss <= bal ) {
            bal -= loss ;
        } else {
            loss = bal ;
            bal = 0 ;
        }
        double interest = bal * daily_rate;
      
        /* Synthetic amortization / prepay behavior */
        double scheduled_principal = base_balance * inv_days_p50 ;
        double prepay = bal * prepay_rate ;

        bal -= scheduled_principal ;

        if ( prepay <= bal ) {
            bal -= prepay ;
        } else {
            prepay = bal ;
            bal = 0 ;
        }

        S[t] = scheduled_principal ;
        P[t] = prepay;
        I[t] = interest;
        L[t] = loss;

    }

}

static double loan_pv(int days, const double *S, const double *P, const double *I, const double *L, const double *DF)
{
    double pv = 0.0 ;
    for (int t=0 ; t<days ; t++) {
        double cf = S[t] + P[t] + I[t] ;
        pv += cf/DF[t] ;
    }
    return pv ;
}

static inline void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "malloc(%zu) failed\n", n);
        exit(2);
    }
    return p;
}

static struct portfolio_result port_pv_heap_per_loan(int loans, int sim_days)
{
    struct portfolio_result res = {} ; 
    double *DF = (double *)xmalloc(STATIC_MAX_DAYS*sizeof(*DF)) ;
    calc_DF(sim_days, DF, 5.0) ;

    for (int loan = 0; loan < loans; ++loan) {
        struct loan_info info = get_loan_info(loan, sim_days) ;

        int loan_days = info.days ;
        double *P  = xmalloc( loan_days * sizeof(*P));
        double *S  = xmalloc( loan_days * sizeof(*S));
        double *I  = xmalloc( loan_days * sizeof(*I));
        double *L  = xmalloc( loan_days * sizeof(*L));

        model_loan(&info, S, P, I, L);
        res.pv += loan_pv(loan_days, S, P, I, L, DF) ;

        free(L);
        free(I);
        free(P);
        free(S) ;
    }

    free(DF) ;
    return res ;
}

static struct portfolio_result port_pv_heap_bulk(int loans, int sim_days)
{
    struct loan_data {
        double P[STATIC_MAX_DAYS] ;
        double I[STATIC_MAX_DAYS] ;
        double L[STATIC_MAX_DAYS] ;
        double S[STATIC_MAX_DAYS] ;
    } ;
    struct portfolio_result res = {} ; 

    double *DF = (double *)xmalloc(STATIC_MAX_DAYS*sizeof(*DF)) ;
    calc_DF(sim_days, DF, 5.0) ;

    for (int loan = 0; loan < loans; ++loan) {
        struct loan_info info = get_loan_info(loan, sim_days) ;

        struct loan_data *ld = malloc(sizeof(*ld)) ;
        double *P  = ld->P ;
        double *S  = ld->S ;
        double *I  = ld->I ;
        double *L  = ld->L ;
        int loan_days = info.days ;

        model_loan(&info, S, P, I, L);
        res.pv += loan_pv(loan_days, S, P, I, L, DF) ;

        free(ld) ;
    }
    free(DF) ;
    return res ;
}

static struct portfolio_result port_pv_heap_reuse(int loans, int sim_days) 
{
    struct portfolio_result res = {} ; 

    double *P  = (double *)xmalloc(STATIC_MAX_DAYS*sim_days);
    double *S  = (double *)xmalloc(STATIC_MAX_DAYS*sim_days);
    double *I  = (double *)xmalloc(STATIC_MAX_DAYS*sim_days);
    double *L  = (double *)xmalloc(STATIC_MAX_DAYS*sim_days);

    double *DF = (double *)xmalloc(STATIC_MAX_DAYS * sizeof(*DF));
    calc_DF(STATIC_MAX_DAYS, DF, 5.0 ) ;

    for (int loan = 0; loan < loans; ++loan) {
        struct loan_info info = get_loan_info(loan, sim_days) ;
        model_loan(&info, S, P, I, L);
        res.pv += loan_pv(info.days, S, P, I, L, DF) ;
    }

    free(L);
    free(I);
    free(P);
    free(S) ;
    free(DF);
    return res ;
}

static struct portfolio_result port_pv_static_mem(int loans, int sim_days) {
    static double P[STATIC_MAX_DAYS];
    static double I[STATIC_MAX_DAYS];
    static double L[STATIC_MAX_DAYS];
    static double S[STATIC_MAX_DAYS];
    static double DF[STATIC_MAX_DAYS];

    if (sim_days > STATIC_MAX_DAYS) {
        fprintf(stderr,
                "days=%d exceeds STATIC_MAX_DAYS=%d\n",
                sim_days, STATIC_MAX_DAYS);
        exit(2);
    }

    struct portfolio_result res = {} ; 
    calc_DF(STATIC_MAX_DAYS, DF, 5.0 ) ;

    for (int loan = 0; loan < loans; ++loan) {
        struct loan_info info = get_loan_info(loan, sim_days) ;
        model_loan(&info, S, P, I, L);
        res.pv += loan_pv(info.days, S, P, I, L, DF) ;
    }
    return res ;
}

static struct portfolio_result port_pv_using_vla(int loans, int sim_days) {
#if defined(__STDC_NO_VLA__)
    (void)loans; (void)days; (void)passes; (void)seed;
    fprintf(stderr, "VLA not supported by this compiler mode\n");
#else
    struct portfolio_result res = {} ;
    double DF[sim_days] ;
    calc_DF(sim_days, DF, 5.0 ) ;

    for (int loan = 0; loan < loans; ++loan) {
        /*
            * Be careful: 4 * days * sizeof(double) on stack.
            * For large days this may overflow the stack.
            */

        struct loan_info info = get_loan_info(loan, sim_days) ;
        int loan_days = info.days ;
        double P[loan_days];
        double I[loan_days];
        double L[loan_days];
        double S[loan_days];
        model_loan(&info, S, P, I, L);
        res.pv += loan_pv(info.days, S, P, I, L, DF) ;
    }
#endif
    return res ;
}

static long parse_long(const char *s, const char *name) {
    char *end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno || !end || *end != '\0') {
        fprintf(stderr, "invalid %s: '%s'\n", name, s);
        exit(2);
    }
    return v;
}

static struct portfolio_result run_sim(enum alloc_strategy strategy, int loans, int days)
{
    switch (strategy) {
        case STRAT_STATIC_REUSE:
            return port_pv_static_mem(loans, days);
        case STRAT_VLA:
            return port_pv_using_vla(loans, days);
        case STRAT_HEAP:
            return port_pv_heap_per_loan(loans, days);
        case STRAT_HEAP_BULK:
            return port_pv_heap_bulk(loans, days);
        case STRAT_HEAP_REUSE:
            return port_pv_heap_reuse(loans, days);
        default:
            fprintf(stderr, "unknown strategy %d\n", strategy);
            abort() ;
    }
}

static struct portfolio_result simulate(enum alloc_strategy strategy, int loans, int days, int passes, uint64_t seed)
{
    rng_state = seed ;
    struct portfolio_result res = {} ;

    double t0 = now_seconds();
    for (int pass = 0 ; pass < passes ; pass++) {
        res = run_sim(strategy, loans, days) ;
    }
    double t1 = now_seconds();
    double elapsed = t1 - t0;
    double loans_per_sec = loans * passes / elapsed ;

    printf("strategy: %-15s elapsed: %7.3f seconds, throughput: %10.2f loans/sec, V=%.3f\n",
        strategy_name[strategy], elapsed, loans_per_sec, res.pv/loans) ;
    return res ;
}

int main(int argc, char **argv) {
    enum alloc_strategy strategy = STRAT_ALL;
    int loans = DEFAULT_LOANS;
    int days = DEFAULT_DAYS;
    int passes = DEFAULT_PASSES;
    uint64_t seed = 123456789ULL;

    if (argc > 1) strategy = (enum alloc_strategy)parse_long(argv[1], "strategy");
    if (argc > 2) loans    = (int)parse_long(argv[2], "loans");
    if (argc > 3) days     = (int)parse_long(argv[3], "days");
    if (argc > 4) passes   = (int)parse_long(argv[4], "passes");
    if (argc > 5) seed     = (uint64_t)strtoull(argv[5], NULL, 10);

    if (loans <= 0 || days <= 0 || passes <= 0) {
        fprintf(stderr, "loans, days, passes must be > 0\n");
        return 2;
    }

    double array_mb = ((double)days * sizeof(double)) / (1024.0 * 1024.0);
    double all_mb = 5 * array_mb;

    printf("loans      : %d\n", loans);
    printf("days       : %d\n", days);
    printf("passes     : %d\n", passes);
    printf("seed       : %llu\n", (unsigned long long)seed);
    printf("array size : %.3f MiB per array\n", array_mb);
    printf("workspace  : %.3f MiB for ALL\n", all_mb);
    printf("\n");

    if ( strategy == STRAT_ALL ) {
        simulate(STRAT_STATIC_REUSE, loans, days, passes, seed) ;
        simulate(STRAT_VLA, loans, days, passes, seed) ;
        simulate(STRAT_HEAP_REUSE, loans, days, passes, seed) ;
        simulate(STRAT_HEAP, loans, days, passes, seed) ;
        simulate(STRAT_HEAP_BULK, loans, days, passes, seed) ;
    } else {
        struct portfolio_result res = simulate(strategy, loans, days, passes, seed) ;
        printf("PV       : %.12f\n", res.pv);
    }
    printf("\n") ;

    return 0;
}