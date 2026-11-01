/*
===============================================================================
Vector Addition with Prefetching (AVX2)

This program implements:
- Scalar baseline
- AVX2 vectorized addition with explicit prefetch
- AVX2 vectorized addition with streaming (non-temporal) stores

It also measures performance and memory bandwidth.

-------------------------------------------------------------------------------
How to compile (Linux, GCC or Clang):

# Scalar + Prefetch + Streaming (AVX2)
gcc -O3 -mavx2 -mfma -fopenmp -o vec_prefetch vec_prefetch.c

# Optional: enable AVX512F if your CPU supports it (not used here)
# gcc -O3 -mavx512f -mfma -fopenmp -o vec_prefetch vec_prefetch.c

-------------------------------------------------------------------------------
How to run:

# Run with default size (16,777,216 elements) and 5 iterations:
./vec_prefetch

# Run with custom array size and iterations:
./vec_prefetch <N> <iters>

Example: 67 million elements, 3 iterations
./vec_prefetch 67108864 3

===============================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <immintrin.h>

// ================= Timing =================
static inline double now_sec(void) {
#ifdef _OPENMP
    return omp_get_wtime();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
#endif
}

// ================= Data init =================
static void init_arrays(float *A, float *B, size_t N) {
    for (size_t i = 0; i < N; ++i) {
        A[i] = (float)i * 0.5f;
        B[i] = (float)(N - i) * 0.25f;
    }
}

// ================= Scalar =================
void vec_add_scalar(const float *A, const float *B, float *C, size_t N) {
    for (size_t i = 0; i < N; ++i) C[i] = A[i] + B[i];
}

// ================= Prefetch (AVX2) =================
#ifndef CL_SIZE
#define CL_SIZE 64
#endif
#ifndef PFDIST_CL
#define PFDIST_CL 8
#endif
#define PFDIST_ELEMS ((PFDIST_CL*CL_SIZE)/sizeof(float))

void vec_add_avx_prefetch(const float *A, const float *B, float *C, size_t N) {
#if defined(__AVX__) || defined(__AVX2__)
    size_t i = 0;
    const size_t V = 8;
    for (; i + V <= N; i += V) {
        size_t pf = i + PFDIST_ELEMS;
        if (pf < N) {
            _mm_prefetch((const char*)(A + pf), _MM_HINT_T0);
            _mm_prefetch((const char*)(B + pf), _MM_HINT_T0);
            _mm_prefetch((const char*)(C + pf), _MM_HINT_T0);
        }
        __m256 a = _mm256_load_ps(A + i);
        __m256 b = _mm256_load_ps(B + i);
        __m256 c = _mm256_add_ps(a, b);
        _mm256_store_ps(C + i, c);
    }
    for (; i < N; ++i) C[i] = A[i] + B[i];
#else
    vec_add_scalar(A, B, C, N);
#endif
}

void vec_add_avx_stream(const float *A, const float *B, float *C, size_t N) {
#if defined(__AVX__) || defined(__AVX2__)
    size_t i = 0;
    const size_t V = 8;
    for (; i + V <= N; i += V) {
        size_t pf = i + PFDIST_ELEMS;
        if (pf < N) {
            _mm_prefetch((const char*)(A + pf), _MM_HINT_NTA);
            _mm_prefetch((const char*)(B + pf), _MM_HINT_NTA);
        }
        __m256 a = _mm256_load_ps(A + i);
        __m256 b = _mm256_load_ps(B + i);
        __m256 c = _mm256_add_ps(a, b);
        _mm256_stream_ps(C + i, c);
    }
    for (; i < N; ++i) C[i] = A[i] + B[i];
    _mm_sfence();
#else
    vec_add_scalar(A, B, C, N);
#endif
}

// ================= Verification =================
float max_abs_err(const float *X, const float *Y, size_t N) {
    float e = 0.0f;
    for (size_t i = 0; i < N; ++i) {
        float d = fabsf(X[i] - Y[i]);
        if (d > e) e = d;
    }
    return e;
}

// ================= Benchmark =================
double bench_bytes(const char* name,
                   void (*kernel)(const float*, const float*, float*, size_t),
                   const float *A, const float *B, float *C, const float *Ref,
                   size_t N, int iters, size_t traffic_bytes)
{
    kernel(A, B, C, N);
    double best = 1e9;
    for (int t = 0; t < iters; ++t) {
        double t0 = now_sec();
        kernel(A, B, C, N);
        double t1 = now_sec();
        if (t1-t0 < best) best = t1-t0;
    }
    double gb = (double)traffic_bytes / 1e9;
    double gbps = gb / best;
    float err = max_abs_err(C, Ref, N);
    printf("%-20s: %8.4f s  |  %6.1f GB/s  |  max |err| = %.3g\n",
           name, best, gbps, err);
    return best;
}

// ================= Main =================
int main(int argc, char** argv) {
    size_t N = (1u<<24);
    int iters = 5;
    if (argc > 1) N = strtoull(argv[1], NULL, 10);
    if (argc > 2) iters = atoi(argv[2]);

    const size_t bytes = N*sizeof(float);
    printf("N=%zu (%.2f MB per array), iterations=%d\n", N, (double)bytes/1e6, iters);

    float *A=NULL,*B=NULL,*C=NULL,*Ref=NULL;
    if (posix_memalign((void**)&A,64,bytes) ||
        posix_memalign((void**)&B,64,bytes) ||
        posix_memalign((void**)&C,64,bytes) ||
        posix_memalign((void**)&Ref,64,bytes)) {
        fprintf(stderr,"alloc failed\n"); return 1;
    }

    init_arrays(A,B,N);
    vec_add_scalar(A,B,Ref,N); // reference

    const size_t traffic_cached = 4*bytes;
    const size_t traffic_stream = 3*bytes;

    double t_scalar = bench_bytes("scalar",       vec_add_scalar,       A,B,C,Ref,N,iters,traffic_cached);
    double t_pf     = bench_bytes("avx+prefetch", vec_add_avx_prefetch, A,B,C,Ref,N,iters,traffic_cached);
    double t_stream = bench_bytes("avx+stream",   vec_add_avx_stream,   A,B,C,Ref,N,iters,traffic_stream);

    printf("Speedup avx+prefetch vs scalar: %.2fx\n", t_scalar/t_pf);
    printf("Speedup avx+stream   vs scalar: %.2fx\n", t_scalar/t_stream);

    free(A); free(B); free(C); free(Ref);
    return 0;
}
