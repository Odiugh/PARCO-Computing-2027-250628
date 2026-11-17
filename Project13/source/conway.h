/*
 * Shared Functions and Constants for Conway's Game of Life
 *
 * Guido Betti 2026
 */

// ----- Libraries ----- //

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

// ----- Definitions ----- //

// Game logic constants 
#define CELL_TYPE unsigned char
#define BUFFER_SIZE ((size_t)(N) * (N) * sizeof(CELL_TYPE))

// Game simulation constants
#ifndef N
#define N 100
#endif

#ifndef T
#define T 100
#endif

#ifndef PRESET
#define PRESET "RANDOM"
#endif

#ifndef DEBUG
#define DEBUG 0
#endif

#ifndef RANDSEED
#define RANDSEED 42
#endif

// Count neighbours for cell at row,col
static inline size_t neighbours(CELL_TYPE* data, size_t row, size_t col);
static inline size_t neighbours_outer(CELL_TYPE* world, size_t row, size_t col);
static inline size_t neighbours_inner(CELL_TYPE* data, size_t row, size_t col);

// Determine if cell at row,col is alive
static inline _Bool is_alive_inner(CELL_TYPE* world, size_t row, size_t col);
static inline _Bool is_alive_outer(CELL_TYPE* world, size_t row, size_t col);

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

// Global structs for timekeeping
static struct timespec start, end;
// Timer functions for code cleanness
static inline int start_timer();
static inline double stop_timer();

// ----- Implementations ----- //

static inline size_t neighbours(CELL_TYPE* data, size_t row, size_t col)
{
    size_t index = row*N + col;

    // Unraveld loop, skips the use of i,j and the checks on them
    if(row == 0){
        if(col == 0){ // top left
            return /* the cell itself */ data[index + 1] +
                data[index + N] + data[index + N + 1];
        } else if(col == N-1){ // top right
            return data[index - 1] + /* the cell itself */
                data[index + N - 1] + data[index + N];
        } else { // top edge
            return data[index - 1] + /* the cell itself */ data[index + 1] +
                data[index + N - 1] + data[index + N] + data[index + N + 1];
        }
    } else if(row == N-1){
        if(col == 0){ // bottom left
            return data[index - N] + data[index - N + 1] +
                /* the cell itself */ data[index + 1];
        } else if(col == N-1){ // bottom right
            return data[index - N - 1] + data[index - N] +
                data[index - 1]; /* the cell itself */
        } else { // bottom edge
            return data[index - N - 1] + data[index - N] + data[index - N + 1] +
                data[index - 1] + /* the cell itself */ data[index + 1];
        }
    } else {
        if(col == 0){ // left edge
            return data[index - N] + data[index - N + 1] +
                /* the cell itself */ data[index + 1] +
                data[index + N] + data[index + N + 1];
        } else if(col == N-1){ // right edge
            return data[index - N - 1] + data[index - N] +
                data[index - 1] + /* the cell itself */
                data[index + N - 1] + data[index + N];
        } else { // interior
            return data[index - N - 1] + data[index - N] + data[index - N + 1] +
                data[index - 1] + /* the cell itself */ data[index + 1] +
                data[index + N - 1] + data[index + N] + data[index + N + 1];
        }
    }
}

static inline size_t neighbours_inner(CELL_TYPE* data, size_t row, size_t col)
{
    size_t index = row*N + col;

    return data[index - N - 1] + data[index - N] + data[index - N + 1] +
        data[index - 1] + /* the cell itself */  data[index + 1] +
        data[index + N - 1] + data[index + N] + data[index + N + 1];
}

static inline size_t neighbours_outer(CELL_TYPE* data, size_t row, size_t col)
{
    size_t index = row*N + col;
    
    // Unraveld loop, skips the use of i,j and the checks on them
    // Only called for row == 0 || col == 0
    if(row == 0){
        if(col == 0){ // top left edge
            return /* the cell itself*/ data[index + 1] +
                data[index + N] + data[index + N + 1];
        } else if (col == N-1){ // top right
            return data[index - 1] + /* the cell itself */
                data[index + N - 1] + data[index + N];
        } else {
            return data[index - 1] + /* the cell itself */ data[index + 1] +
                data[index + N - 1] + data[index + N] + data[index + N + 1];
        }
    } else if (row == N-1){
        if(col == 0){ // bottom left
            return data[index - N] + data[index - N + 1] +
                /* the cell itself */ data[index + 1];
        } else if (col == N-1){ // bottom right
            return data[index - N - 1] + data[index - N] +
                data[index - 1]; /* the cell itself */
        } else {
            return data[index - N - 1] + data[index - N] + data[index - N + 1] +
                data[index - 1] + /* the cell itself */ data[index + 1];
        }
    } else if (col == 0){ // corners are skipped because they are handled by row == 0,N
        return data[index - N] + data[index - N + 1] +
            /* the cell itself */ data[index + 1] +
            data[index + N] + data[index + N + 1];
    } else { // col == N-1 -> outer is used only in this 4 cases
        return data[index - N - 1] + data[index - N] +
            data[index - 1] + /* the cell itself */
            data[index + N - 1] + data[index + N];
    }
}

static inline _Bool is_alive(CELL_TYPE* data, size_t row, size_t col)
{
    size_t n = neighbours(data, row, col);

    return n == 3 || (n == 2 && data[row*N + col]);
}

static inline _Bool is_alive_inner(CELL_TYPE* data, size_t row, size_t col)
{
    size_t n = neighbours_inner(data, row, col);

    return n == 3 || (n == 2 && data[row*N + col]);
}

static inline _Bool is_alive_outer(CELL_TYPE* data, size_t row, size_t col)
{
    size_t n = neighbours_outer(data, row, col);

    return n == 3 || (n == 2 && data[row*N + col]);
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
    memset(data, 0, BUFFER_SIZE);

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

int load_world(CELL_TYPE **m, char *filename)
{
    if(!m || !filename) return 1;

    if(!strcmp(filename, "RANDOM")){
        srand(RANDSEED);
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = 0; j < N; ++j) {
                m[i][j] = rand()%2;
            }
        }
        return 0;
    }

    FILE *f = fopen(filename, "r");
    if (!f) { perror(filename); return 1; }

    size_t row = 0, col = 0;    // position inside the file's matrix
    int ch;

    while ((ch = fgetc(f)) != EOF) {
        if (ch == '0' || ch == '1') {
            if (row < N && col < N)          // skip cells that don't fit
                m[row][col] = (CELL_TYPE)(ch - '0');
            ++col;
        } else if (ch == '\n') {
            if (col > 0) {                   // empty lines are not rows
                ++row;
            }
            col = 0;
        }
    }

    fclose(f);

    return 0;
}

static inline int start_timer()
{
    return clock_gettime(CLOCK_MONOTONIC, &start);
}

static inline double stop_timer()
{
    clock_gettime(CLOCK_MONOTONIC, &end);

    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}


int print_world(CELL_TYPE **world, CELL_TYPE **old_world, size_t iteration, _Bool color)
{
    if(!world || !old_world) return 1;

    // Clear screen;
    //system("clear");

    // Iteration number logging
    printf("Iteration %zu ", iteration);
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