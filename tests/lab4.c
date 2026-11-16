#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
#include <pthread.h>

#define N 1000
#define T 10


_Bool isPrime(unsigned int n) {
    if (n <= 1) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;


    for (unsigned int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

typedef struct {
    int start;
    int end;
} start_end;

void* countPrimes(void* arg){
    start_end* se = (start_end*)arg;

    int* n = (int*)malloc(sizeof(int));
    *n = 0;

    for (int i = se->start; i <= se->end; ++i){
        if(isPrime(i)) ++(*n);
    }

    return (void*) n;
}

int main(){

    pthread_t th[T];
    //task_t tk[T];
    start_end SE[T];

    int count = 0;

    for (int t = 0; t < T; t++) {
        //tk[t].id = t;

        SE[t].start = N/T * t;
        SE[t].end = (N/T * (t+1)) - 1;

        pthread_create(&th[t], NULL, countPrimes, &SE[t]);

        
    }
    
    for (int t = 0; t < T; t++){
        int* c;

        pthread_join(th[t], (void**)&c); /* wait for all */

        count += *c;

        free(c);
    }

    printf("Cunt: %d", count);

}