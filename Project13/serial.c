/*
 * Serial Implementation of Conway's Game of Life
 *
 * Guido Betti 2026
 */

// ----- Libraries ----- //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ----- Definitions ----- //

// Game logic constants 
#define UNDERPOPULATION_THRESHOLD 1 // max number of neighbors to count as underpopulation
#define OVERPOPULATION_THRESHOLD 4 // min number of neighbors to count as overpopulation
#define REPRODUCTION_VALUE 3 // number of neighbors to spawn a new cell
#define CELL_TYPE unsigned char
#define BUFFER_SIZE (N * N * sizeof(CELL_TYPE))

// Game simulation constants
#define N 100 // world size
#define T 1000 // computed steps
#define DEBUG true

// Count neighbors for cell at row,col
size_t neighbors(CELL_TYPE** world, size_t row, size_t col);

// Determine if cell at row,col is alive
static inline _Bool is_alive(CELL_TYPE** world, size_t row, size_t col);

// Allocate in memory a NxN matrix and initialize all values to 0
CELL_TYPE** alloc_world();

// Free the NxN world buffer
int free_world(CELL_TYPE** world);

// Print the world for debug mode
int print_world(CELL_TYPE** world, CELL_TYPE** old_world, size_t iteration);

// ----- Global Variables ----- //



// ----- Main ----- //

int main(){
    // Matrix buffers
    CELL_TYPE** m0 = alloc_world();
    CELL_TYPE** m1 = alloc_world();

    if(!m0 || !m1) return 1;

    for(int t = 0; t < T; ++t){
        for(size_t i = 0; i < N; ++i){
            for(size_t j = 0; j < N; ++j){
                m1[i][j] = is_alive(m0, i, j);
            }
        }

#if DEBUG
        print_world(m1, m0, t);
#endif        

        // swamp buffers to avoid rewriting
        CELL_TYPE** tmp = m0;
        m0 = m1;
        m1 = tmp;
    }
}

// ----- Implementations ----- //

size_t neighbors(CELL_TYPE** world, size_t row, size_t col)
{
    if(!world) return 0;
    
    size_t neighbors = 0;
    for(long long int i = row-1; i < row+1; ++i){
        for(long long int j = col-1; i < col+1; ++i){
            if(
                i >= 0 && // i in bounds
                i < N &&
                j >= 0 && // j in bounds
                j <= N &&
                !(i == row && j == col) && // not the cell itself
                world[i][j] // neighbour is alive
            ) ++neighbors;
        }
    }

    return neighbors;
}

static inline _Bool is_alive(CELL_TYPE** world, size_t row, size_t col)
{
    size_t n = neighbors(world, row, col);
    
    return (
        n == REPRODUCTION_VALUE ? 1 : (
        n <= UNDERPOPULATION_THRESHOLD ? 0 : (
        n >= OVERPOPULATION_THRESHOLD ? 0 : 1 ))
    );
}

CELL_TYPE** alloc_world()
{
    // Allocate row index array
    CELL_TYPE** world = malloc(N * sizeof(CELL_TYPE*));
    if (!world) return NULL;

    // Allocate continuos data for the buffer
    CELL_TYPE* data = malloc(BUFFER_SIZE);
    if (!data) {
        free(world);
        return NULL;
    }

    // Set values of the index array
    for (size_t i = 0; i < N; i++) {
        world[i] = data + (i * N);
    }

    // Set initial value of the buffer
    memset(data, 0, N * N * sizeof(CELL_TYPE));

    return world;
}

int free_world(CELL_TYPE** world)
{
    if(!world) return 1;
    if(!world[0]) return 2;
    
    free(world[0]);
    free(world);
    
    return 0;
}

int print_world(CELL_TYPE **world, CELL_TYPE **old_world, size_t iteration)
{
    if(!world || !old_world) return 1;

    printf("Iteration %li ", iteration);
    for(size_t i = 0; i < N/2; ++i)
        printf("--");
    printf("\n");

    for(size_t i = 0; i < N; ++i){
        for(size_t j = 0; j < N; ++j){
            char* color = ( 
                world[i][j] && old_world[i][j] ? "\033[43m" : (
                !world[i][j] && !old_world[i][j] ? "\033[0m": (
                world[i][j] ? "\033[42m" : "\033[41m"))
            );
            
            printf("%s[]", color);
        }
        printf("\033[0m\n");
    }

    for(size_t i = 0; i < N; ++i)
        printf("--");
    printf("\n");

    return 0;
}
