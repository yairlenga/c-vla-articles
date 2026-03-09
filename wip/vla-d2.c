// hidden variables: var_##_sz (int, size), var_##_vla (char [n] or char [0])
// Better concat.
// Used in Article

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#ifndef VLA_SIZE_MAX
#define VLA_SIZE_MAX 800
#endif

#define FLEX_ARRAY_INIT(var_, type_, count_) \
	const int var_##_count = count_ ; \
	type_ var_##_vla[ (size_t) (var_##_count*sizeof(type_)) > (size_t) VLA_SIZE_MAX ? 0 : var_##_count] ; \
	type_ *var_ = sizeof(var_##_vla) ? var_##_vla : malloc(3*var_##_count * sizeof(type_)) ;

//	type_ (*var_##_ptr)[var_##_count] = sizeof(var_##_vla) ? var_##_vla : malloc(var_##_count * sizeof(type_)) ;


#define FLEX_ARRAY_FREE(var_) \
	if ( !sizeof(var_##_vla) ) { free(var_) ; var_ = NULL ; }

#define FLEX_ARRAY_V(var_) (*var_)
#define FLEX_ARRAY_SIZE(var_) var_##_count ;
#define FLEX_ARRAY_VS(var_) (var_), STR_VIEW_SIZE(var_)

int vla_size_max = VLA_SIZE_MAX ;
#undef VLA_SIZE_MAX
#define VLA_SIZE_MAX vla_size_max

static void test1(bool show, double *x, int n) 
{
	FLEX_ARRAY_INIT(work, double, n) ;
	work[0] = x[0] ; work[n-1] = x[n-1] ;
	for (int i=1 ; i<n-1 ; i++) work[i] = 0.25*x[i-1] + 0.5 * x[i] + 0.25*x[i+1] ;
	if ( show ) { int i=n/2 ; printf("Smooth(%d): x[%d]=%.1f -> %.1f sz=%d\n", n, i, x[i], work[i], (int) sizeof(work_vla)) ;} ;
	FLEX_ARRAY_INIT(a2, double, n) ;
	a2[0] = x[0] ;
	for (int i=1 ; i<n-1 ; i++ ) a2[i] = a2[i-1]+x[i] ;
	memcpy(x, work, n*sizeof(*x)) ;
	FLEX_ARRAY_FREE(work) ;
	FLEX_ARRAY_FREE(a2) ;
}

static double now(void)
{
	struct timespec ts ;
	clock_gettime(CLOCK_MONOTONIC, &ts) ;
	return ts.tv_sec + ts.tv_nsec*1e-9 ;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv)
{
	const int N = argc > 1 ? atoi(argv[1]) : 1000000;

	if ( argc > 2 ) vla_size_max = atoi(argv[2]) ;
	int NZ = VLA_SIZE_MAX/sizeof(double) ;
	double z[NZ*2] ;

	for (int i=0 ;  i<(int) (sizeof z / sizeof z[0]) ; i++) z[i] = i*i ;
	test1(false, z, 40) ;
	test1(false, z, 40) ;

	double s1 = 0, s2 = 0 ;
	for (int i=0 ; i<N ; i++ ) {
		int k = NZ + rand()%NZ ;
		double t0= now() ;
		vla_size_max = sizeof(z) ;
		test1(i<5, z, k) ;
		double t1 = now() ;
		vla_size_max = 0 ;
		test1(i<5, z, k) ;
		double t2= now() ;
		s1 += (t1-t0) ;
		s2 += (t2-t1) ;
	}

	printf("Speed (1M): VLA=%.6f, HEAP=%.6f, malloc(1M)=%.6f\n", 1e6*(s1)/N, 1e6*(s2)/N, 1e6*(s2-s1)/N) ;

	exit(EXIT_SUCCESS) ;
}
