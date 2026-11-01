/*
vec_avx512.c
AVX-512 intrinsics version.

Compile:
  gcc -O3 -mavx512f -mfma -Wall -Wextra -o vec_avx512 vec_avx512.c -lrt
  or:
  gcc -O3 -march=native -mfma -Wall -Wextra -o vec_avx512 vec_avx512.c -lrt
  (only if native arch has AVX-512)

Run:
  ./vec_avx512 [N] [iters]
  Example: ./vec_avx512 134217728 5

Notes:
  - Requires both compiler and CPU support for AVX-512.
  - If not compiled with AVX-512, this program falls back to the AVX2 implementation at compile time.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <immintrin.h>

static inline double now_sec(void) {
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void init_arrays(float *A, float *B, size_t N) {
    for (size_t i = 0; i < N; ++i) {
        A[i] = (float)i * 0.5f;
        B[i] = (float)(N - i) * 0.25f;
    }
}

void vec_add_scalar(const float *A, const float *B, float *C, size_t N) {
    for (size_t i = 0; i < N; ++i) C[i] = A[i] + B[i];
}

void vec_add_avx2(const float *A, const float *B, float *C, size_t N) {
#if defined(__AVX2__)
    size_t i = 0; const size_t V = 8;
    for (; i + V <= N; i += V) {
        __m256 a = _mm256_load_ps(A+i);
        __m256 b = _mm256_load_ps(B+i);
        _mm256_store_ps(C+i, _mm256_add_ps(a,b));
    }
    for (; i < N; ++i) C[i] = A[i] + B[i];
#else
    vec_add_scalar(A,B,C,N);
#endif
}

void vec_add_avx512(const float *A, const float *B, float *C, size_t N) {
#if defined(__AVX512F__)
    size_t i = 0;
    const size_t V = 16; // 16 floats per 512-bit
    for (; i + V <= N; i += V) {
        __m512 a = _mm512_load_ps(A + i);
        __m512 b = _mm512_load_ps(B + i);
        __m512 c = _mm512_add_ps(a, b);
        _mm512_store_ps(C + i, c);
    }
    for (; i < N; ++i) C[i] = A[i] + B[i];
#else
    // fallback to AVX2 or scalar depending on compile-time flags
    vec_add_avx2(A,B,C,N);
#endif
}

float max_abs_err(const float *X, const float *Y, size_t N) {
    float e=0.0f;
    for (size_t i=0;i<N;++i){ float d=fabsf(X[i]-Y[i]); if (d>e) e=d; }
    return e;
}

int main(int argc, char **argv) {
    size_t N = (1u<<27);
    int iters = 5;
    if (argc > 1) N = strtoull(argv[1], NULL, 10);
    if (argc > 2) iters = atoi(argv[2]);
    printf("vec_avx512: N=%zu (%.2f MB per array), iterations=%d\n",
           N, N * sizeof(float) / 1e6, iters);

    size_t bytes = N * sizeof(float);
    float *A=NULL,*B=NULL,*C=NULL,*Ref=NULL;
    if (posix_memalign((void**)&A, 64, bytes) ||
        posix_memalign((void**)&B, 64, bytes) ||
        posix_memalign((void**)&C, 64, bytes) ||
        posix_memalign((void**)&Ref,64, bytes)) {
        fprintf(stderr, "posix_memalign failed\n"); return 1;
    }

    init_arrays(A,B,N);
    vec_add_scalar(A,B,Ref,N); // reference

    vec_add_avx512(A,B,C,N); // warm-up
    double best = 1e9;
    for (int t=0;t<iters;++t) {
        double t0 = now_sec();
        vec_add_avx512(A,B,C,N);
        double t1 = now_sec();
        if (t1-t0 < best) best = t1-t0;
    }

    double gb = (3.0 * bytes) / 1e9;
    double gbps = gb / best;
    float err = max_abs_err(C, Ref, N);
    printf("%-18s: %8.4f s  |  %6.1f GB/s  |  max |err| = %.3g\n",
           "avx512", best, gbps, err);

    free(A); free(B); free(C); free(Ref);
    return 0;
}
