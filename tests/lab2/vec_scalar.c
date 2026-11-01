/*
vec_scalar.c
Scalar baseline for vector add.

Compile:
  gcc -O3 -march=native -Wall -Wextra -o vec_scalar vec_scalar.c -lrt

Run:
  ./vec_scalar [N] [iters]
  Example: ./vec_scalar 134217728 5
    - N default ~134M elements (adjustable)
    - iters default 5

Notes:
  - Uses posix_memalign for 64-byte alignment.
  - Measures best of `iters` runs (wall time).
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <immintrin.h>

static inline double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
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

float max_abs_err(const float *X, const float *Y, size_t N) {
    float e = 0.0f;
    for (size_t i = 0; i < N; ++i) {
        float d = fabsf(X[i] - Y[i]);
        if (d > e) e = d;
    }
    return e;
}

int main(int argc, char **argv) {
    size_t N = (1<<27); // default ~134M
    int iters = 5;
    if (argc > 1) N = strtoull(argv[1], NULL, 10);
    if (argc > 2) iters = atoi(argv[2]);
    printf("vec_scalar: N=%zu (%.2f MB per array), iterations=%d\n",
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

    // warm-up
    vec_add_scalar(A,B,C,N);
    double best = 1e9;
    for (int t=0;t<iters;++t) {
        double t0 = now_sec();
        vec_add_scalar(A,B,C,N);
        double t1 = now_sec();
        if (t1-t0 < best) best = t1-t0;
    }

    double gb = (3.0 * bytes) / 1e9;
    double gbps = gb / best;
    float err = max_abs_err(C, Ref, N);
    printf("%-18s: %8.4f s  |  %6.1f GB/s  |  max |err| = %.3g\n",
           "scalar", best, gbps, err);

    free(A); free(B); free(C); free(Ref);
    return 0;
}
