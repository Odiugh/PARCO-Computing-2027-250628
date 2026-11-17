/*
 * Serial Implementation of Conway's Game of Life
 *
 * Guido Betti 2026
 */

// ----- Libraries ----- //

#include "conway.h"


// ----- Definitions ----- //

// Simulate n steps
int simulate(CELL_TYPE** m0, CELL_TYPE** m1, size_t cycles);


// ----- Main ----- //

int main(){
    // Matrix buffers
    CELL_TYPE** m0 = alloc_world();
    CELL_TYPE** m1 = alloc_world();

    if(!m0 || !m1) return 1;

    if(load_world(m0, PRESET)) return 2;

    //warmup
    simulate(m0,m1, 100);

    start_timer();

    simulate(m0, m1, T);
	
    double dt = stop_timer();
	printf("%.6f\n", dt);

    free_world(m0);
    free_world(m1);

    return 0;
}


// ----- Implementations ----- //

int simulate(CELL_TYPE** m0, CELL_TYPE** m1, size_t cycles)
{
    if(!m0 || !m1) return 1;

    CELL_TYPE *d0 = m0[0], *d1 = m1[0]; 
    typeof(d0) tmp;

    // Assuming N>=4 borders are computed manually and the for loop is only used for inner data, skipping checks
    for(size_t t = 0; t < cycles; ++t){

        for(size_t j = 0; j < N; ++j){
            d1[j] = is_alive_outer(d0, 0, j);
        }

        for(size_t i = 1; i < N-1; ++i){

            d1[i*N] = is_alive_outer(d0, i, 0);

            for(size_t j = 1; j < N-1; ++j){
                d1[i*N + j] = is_alive_inner(d0, i, j);
            }

            d1[i*N + N - 1] = is_alive_outer(d0, i, N-1);
        }

        for(size_t j = 0; j < N; ++j){
            d1[(N-1) * N + j] = is_alive_outer(d0, N-1, j);
        }

        // Swamp buffers to avoid rewriting
        tmp = d0;
        d0 = d1;
        d1 = tmp;

#if DEBUG > 1
        print_world(m1, m0, t, 1);
        //usleep(100000);
        typeof(m0) mtmp = m0;
        m0 = m1;
        m1 = mtmp;
#endif        
    }

    return 0;
}