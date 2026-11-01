/*
 * Serial Implementation of Conway's Game of Life
 *
 * Guido Betti 2026
 */

// ----- Libraries ----- //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ----- Definitions ----- //

// Game logic constants 
#define UNDERPOPULATION_THRESHOLD 1 // max number of neighbors to count as underpopulation
#define OVERPOPULATION_THRESHOLD 4 // min number of neighbors to count as overpopulation
#define REPRODUCTION_VALUE 3 // number of neighbors to spawn a new cell
#define CELL_TYPE unsigned char
#define BUFFER_SIZE (N * N * sizeof(CELL_TYPE))

// Game simulation constants
#define N 50 // world size
#define T 50// computed steps
#define DEBUG 1

// Count neighbors for cell at row,col
size_t neighbors(CELL_TYPE** world, size_t row, size_t col);

// Determine if cell at row,col is alive
static inline _Bool is_alive(CELL_TYPE** world, size_t row, size_t col);

// Allocate in memory a NxN matrix and initialize all values to 0
CELL_TYPE** alloc_world();

// Free the NxN world buffer
int free_world(CELL_TYPE** world);

// Print the world for debug mode
int print_world(CELL_TYPE** world, CELL_TYPE** old_world, size_t iteration, _Bool color);

// Simulate n steps
int simulate(CELL_TYPE** m0, CELL_TYPE** m1, size_t cycles);

// Load world from txt file
int load_world(CELL_TYPE** m, char* filename);

// ----- Global Variables ----- //



// ----- Main ----- //

int main(){
    // Matrix buffers
    CELL_TYPE** m0 = alloc_world();
    CELL_TYPE** m1 = alloc_world();

    load_world(m0, "start.txt");

    simulate(m0, m1, T);

    return 0;
}

// ----- Implementations ----- //

size_t neighbors(CELL_TYPE** world, size_t row, size_t col)
{
    if(!world) return 0;
    
    size_t neighbors = 0;
    for(long long int i = (long long)row-1; i <= (long long)row+1; ++i){
        for(long long int j = (long long)col-1; j <= (long long)col+1; ++j){
            if(
                i >= 0 && // i in bounds
                i < N &&
                j >= 0 && // j in bounds
                j < N &&
                !(i == (long long)row && j == (long long)col) && // not the cell itself
                world[i][j] // neighbour is alive
            ) ++neighbors;
        }
    }

    return neighbors;
}

static inline _Bool is_alive(CELL_TYPE** world, size_t row, size_t col)
{
    if(!world || row >= N || col >= N) return 0;

    size_t n = neighbors(world, row, col);

    return (
        world[row][col] ? (
            n > UNDERPOPULATION_THRESHOLD && n < OVERPOPULATION_THRESHOLD ? 1 : 0
        ) : n == REPRODUCTION_VALUE ? 1 : 0
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
    for (size_t i = 0; i < N; ++i) {
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
    
    free(world[0]); // free buffer
    free(world); // free index array
    
    return 0;
}

int print_world(CELL_TYPE **world, CELL_TYPE **old_world, size_t iteration, _Bool color)
{
    if(!world || !old_world) return 1;

    // Clear screen;
    system("clear");

    // Iteration number logging
    printf("Iteration %li ", iteration);
    for(size_t i = 0; i < N/2; ++i)
        printf("--");
    printf("\n");

    for(size_t i = 0; i < N; ++i){
        for(size_t j = 0; j < N; ++j){
            char* color_code;
            if(color){
                color_code = ( 
                    world[i][j] && old_world[i][j] ? "\033[43m" : ( // alive, no changes
                    !world[i][j] && !old_world[i][j] ? "\033[0m": ( // dead, no changes
                    world[i][j] ? "\033[42m" : "\033[41m")) // alive, just born : dead, was alive
                );
            } else {
                color_code = world[i][j] ? "\033[47m" : "\033[0m";
            }
            
            printf("%s[]", color_code);
        }
        printf("\033[0m\n");
    }

    for(size_t i = 0; i < N; ++i)
        printf("--");
    printf("\n");

    return 0;
}

int simulate(CELL_TYPE **m0, CELL_TYPE **m1, size_t cycles)
{
    if(!m0 || !m1) return 1;

    for(size_t t = 0; t < cycles; ++t){
        for(size_t i = 0; i < N; ++i){
            for(size_t j = 0; j < N; ++j){
                m1[i][j] = is_alive(m0, i, j);
            }
        }

#if DEBUG
        print_world(m1, m0, t, 0);
        sleep(1);
#endif        

        // Swamp buffers to avoid rewriting
        CELL_TYPE** tmp = m0;
        m0 = m1;
        m1 = tmp;
    }

    // Reswap the buffers if cycles are not even
    if(!(cycles%2)){
        CELL_TYPE** tmp = m0;
        m0 = m1;
        m1 = tmp;
    }

    return 0;
}

int load_world(CELL_TYPE **m, char *filename)
{
    if(!m || !filename) return 1;

    FILE *f = fopen(filename, "r");
    if (!f) return 1;

    char ch;
    for (size_t row = 0; row < N; ++row) {
        for (size_t col = 0; col < N; ) {
            // " %1c" automatically skips all whitespace/newlines
            if (fscanf(f, "%1c", &ch) != 1) break;

            if(ch == '\n') break;
            else m[row][col++] = ch == '1' ? 1 : 0;
        }
    }

    fclose(f);

    return 0;
}
