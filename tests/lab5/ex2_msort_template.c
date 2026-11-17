/*
 * Lab 4 - TEMPLATE - Exercise 2: sort an edge list with a task-parallel merge sort
 *
 * A graph is given as a list of m edges (u, v) in random order.
 * For triangle counting we need the edges sorted by u, and for the same u
 * sorted by v. Then the neighbours of every vertex form one sorted block.
 *
 * Trick: store an edge as ONE 64-bit number  key = u * 2^32 + v.
 * Sorting these numbers sorts the edges by u first and then by v.
 *
 * What you have to do:
 *   TODO 1: make merge_sort_tasks() create tasks
 *   TODO 2: start the threads in sort_parallel()
 *
 * Compile:  gcc -O2 -Wall -fopenmp ex2_msort.c -o ex2_msort
 * Run:      ./ex2_msort 20000000 4 65536     (m edges, threads, cutoff)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>

long cutoff;   /* parts smaller than this are sorted without new tasks */

/* merge the two sorted parts a[lo..mid-1] and a[mid..hi-1] using tmp */
void merge(uint64_t *a, uint64_t *tmp, long lo, long mid, long hi)
{
    long i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        if (a[i] <= a[j]) tmp[k++] = a[i++];
        else              tmp[k++] = a[j++];
    }
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi)  tmp[k++] = a[j++];
    for (k = lo; k < hi; k++)
        a[k] = tmp[k];
}

/* normal (serial) merge sort of a[lo..hi-1] */
void merge_sort(uint64_t *a, uint64_t *tmp, long lo, long hi)
{
    if (hi - lo < 2)
        return;
    long mid = (lo + hi) / 2;
    merge_sort(a, tmp, lo, mid);
    merge_sort(a, tmp, mid, hi);
    merge(a, tmp, lo, mid, hi);
}

/* merge sort with OpenMP tasks */
void merge_sort_tasks(uint64_t *a, uint64_t *tmp, long lo, long hi)
{
    /* TODO 1:  */
}

void sort_parallel(uint64_t *a, uint64_t *tmp, long m)
{
    /* TODO 2:  */
}

/* ------------- you do not need to change anything below ------------- */

/* for qsort, used to check your result */
int compare(const void *p, const void *q)
{
    uint64_t a = *(const uint64_t *)p, b = *(const uint64_t *)q;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("usage: %s edges threads cutoff\n", argv[0]);
        return 1;
    }
    long m = atol(argv[1]);
    int threads = atoi(argv[2]);
    cutoff = atol(argv[3]);
    if (m < 2 || threads < 1) {
        printf("need at least 2 edges and 1 thread\n");
        return 1;
    }
    if (cutoff < 2) cutoff = 2;
    omp_set_num_threads(threads);

    long nv = m / 8;                      /* number of vertices: about 8 edges each */
    if (nv < 16) nv = 16;

    uint64_t *edges  = malloc(m * sizeof(uint64_t));   /* the original edge list */
    uint64_t *a      = malloc(m * sizeof(uint64_t));   /* sorted by you          */
    uint64_t *check  = malloc(m * sizeof(uint64_t));   /* sorted by qsort        */
    uint64_t *tmp    = malloc(m * sizeof(uint64_t));
    if (edges == NULL || a == NULL || check == NULL || tmp == NULL) {
        printf("not enough memory, try fewer edges\n");
        return 1;
    }

    /* random edges (u, v) with u < v */
    srand(42);
    for (long k = 0; k < m; k++) {
        uint64_t u = rand() % nv, v = rand() % nv;
        if (u == v) v = (v + 1) % nv;
        if (u > v) { uint64_t t = u; u = v; v = t; }
        edges[k] = (u << 32) | v;
    }

    /* reference: serial merge sort (timed) and qsort (for the check) */
    memcpy(check, edges, m * sizeof(uint64_t));
    double t_serial = omp_get_wtime();
    merge_sort(check, tmp, 0, m);
    t_serial = omp_get_wtime() - t_serial;

    /* your parallel sort */
    memcpy(a, edges, m * sizeof(uint64_t));
    double t_parallel = omp_get_wtime();
    sort_parallel(a, tmp, m);
    t_parallel = omp_get_wtime() - t_parallel;

    memcpy(check, edges, m * sizeof(uint64_t));
    qsort(check, m, sizeof(uint64_t), compare);

    printf("edges = %ld  threads = %d  cutoff = %ld\n", m, threads, cutoff);
    printf("serial   : %.4f s\n", t_serial);
    if (memcmp(a, edges, m * sizeof(uint64_t)) == 0)
        printf("parallel : not implemented yet (TODO 1 and 2)\n");
    else if (memcmp(a, check, m * sizeof(uint64_t)) != 0)
        printf("parallel : WRONG result, the edges are not sorted correctly\n");
    else
        printf("parallel : %.4f s   speedup %.2f\n", t_parallel, t_serial / t_parallel);

    /* after sorting, the neighbours of every vertex are next to each other
       and in increasing order. Example: the first neighbours of vertex 0 */
    if (memcmp(a, check, m * sizeof(uint64_t)) == 0) {
        printf("neighbours of vertex 0:");
        for (long k = 0; k < m && k < 10 && (a[k] >> 32) == 0; k++)
            printf(" %llu", (unsigned long long)(a[k] & 0xFFFFFFFF));
        printf("\n");
    }

    free(edges); free(a); free(check); free(tmp);
    return 0;
}
