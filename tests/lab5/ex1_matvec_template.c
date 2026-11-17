/*
 * Lab 4 - TEMPLATE - Exercise 1: matrix-vector product y = A * x with OpenMP
 *
 * A is an n x n matrix, x and y are vectors of length n.
 *     y[i] = A[i][0]*x[0] + A[i][1]*x[1] + ... + A[i][n-1]*x[n-1]
 *
 * Every y[i] only needs row i of A (and x), so different rows can be
 * computed by different threads at the same time.
 *
 * What you have to do:
 *   TODO 1: parallelize the loop in matvec_parallel()
 *   TODO 2: parallelize the loop in init_matrix()   (first touch)
 *
 * Compile:  gcc -O2 -Wall -fopenmp ex1_matvec.c -o ex1_matvec
 * Run:      ./ex1_matvec 8192 4          (n = 8192, 4 threads)
 */
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/* The matrix is stored row by row in one big array:
   element A[i][j] is at position i*n + j. */

void init_matrix(double *A, long n)
{
    #pragma omp parallel for schedule(static)
    for (long i = 0; i < n; i++)
        for (long j = 0; j < n; j++)
            A[i*n + j] = (double)((i + 2*j) % 17) / 17.0;
}

void matvec_serial(double *A, double *x, double *y, long n)
{
    for (long i = 0; i < n; i++) {
        double sum = 0.0;
        for (long j = 0; j < n; j++)
            sum += A[i*n + j] * x[j];
        y[i] = sum;
    }
}

void matvec_parallel(double *A, double *x, double *y, long n)
{
    #pragma omp parallel for schedule(static)
    for (long i = 0; i < n; i++) {
        double sum = 0.0;
        for (long j = 0; j < n; j++)
            sum += A[i*n + j] * x[j];
        y[i] = sum;
    }
}

/* ------------- you do not need to change anything below ------------- */

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("usage: %s n threads\n", argv[0]);
        return 1;
    }
    long n = atol(argv[1]);
    int threads = atoi(argv[2]);
    if (n < 1 || threads < 1) {
        printf("n and threads must be at least 1\n");
        return 1;
    }
    omp_set_num_threads(threads);

    double *A = malloc(n * n * sizeof(double));
    double *x = malloc(n * sizeof(double));
    double *y_serial = malloc(n * sizeof(double));
    double *y = malloc(n * sizeof(double));
    if (A == NULL || x == NULL || y_serial == NULL || y == NULL) {
        printf("not enough memory, try a smaller n\n");
        return 1;
    }

    init_matrix(A, n);
    for (long j = 0; j < n; j++)
        x[j] = 1.0 + j % 5;
    for (long i = 0; i < n; i++)
        y[i] = -1.0;           /* real results are never negative */

    /* run each version 3 times and keep the fastest time */
    double t_serial = 1e9, t_parallel = 1e9;
    for (int rep = 0; rep < 3; rep++) {
        double t = omp_get_wtime();
        matvec_serial(A, x, y_serial, n);
        t = omp_get_wtime() - t;
        if (t < t_serial) t_serial = t;

        t = omp_get_wtime();
        matvec_parallel(A, x, y, n);
        t = omp_get_wtime() - t;
        if (t < t_parallel) t_parallel = t;
    }

    /* compare the parallel result with the serial one */
    int errors = 0, not_done = 1;
    for (long i = 0; i < n; i++) {
        if (y[i] != -1.0) not_done = 0;
        if (y[i] != y_serial[i]) errors++;   /* same order of additions -> exactly equal */
    }

    char *bind = getenv("OMP_PROC_BIND");
    printf("n = %ld  threads = %d  bind = %s\n", n, threads, bind ? bind : "not set");
    printf("serial   : %.4f s\n", t_serial);
    if (not_done)
        printf("parallel : not implemented yet (TODO 1)\n");
    else if (errors > 0)
        printf("parallel : WRONG result in %d of %ld elements\n", errors, n);
    else
        printf("parallel : %.4f s   speedup %.2f\n", t_parallel, t_serial / t_parallel);

    free(A); free(x); free(y_serial); free(y);
    return 0;
}
