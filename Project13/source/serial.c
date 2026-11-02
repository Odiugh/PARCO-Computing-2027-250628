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
#include <time.h>
#include <sys/time.h>

// ----- Definitions ----- //

// Game logic constants 
#define CELL_TYPE unsigned char
#define BUFFER_SIZE (N * N * sizeof(CELL_TYPE))

// Game simulation constants
#define N 5000 // world size, >=4
#define T 100// computed steps
#define DEBUG 0

// Count neighbours for cell at row,col
static inline size_t neighbours(CELL_TYPE* data, size_t row, size_t col);
static inline size_t neighbours_outer(CELL_TYPE* world, size_t row, size_t col);
static inline size_t neighbours_inner(CELL_TYPE* data, size_t row, size_t col);

// Determine if cell at row,col is alive
static inline _Bool is_alive(CELL_TYPE* world, size_t row, size_t col);
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
struct timeval start, end;
// Timer functions for code cleanness
static inline int start_timer();
static inline double stop_timer();

// ----- Main ----- //

int main(){
    // Matrix buffers
    CELL_TYPE** m0 = alloc_world();
    CELL_TYPE** m1 = alloc_world();

    load_world(m0, "RANDOM");
    //load_world(m0, "RANDOM");

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

static inline size_t neighbours(CELL_TYPE* data, size_t row, size_t col)
{
    size_t neighbours = 0;
    for(long long int i = (long long)row-1; i <= (long long)row+1; ++i){
        if(i < 0 || i >= N) continue; // branch prediction makes it faster to check bounds after for
        for(long long int j = (long long)col-1; j <= (long long)col+1; ++j){
            if(
                j >= 0 && // j in bounds, upper column bound and row bounds can be checked in for condition
                j < N &&
                data[i*N + j] // neighbour is alive
            ) ++neighbours;
        }
    }

    return neighbours - data[row*N + col]; // the cell itself is counted as a neighbour
}

static inline size_t neighbours_outer(CELL_TYPE* data, size_t row, size_t col)
{
    size_t index = row*N + col;
    
    // Unraveld loop, skips the use of i,j and the checks on them
    // Assuming row == 0 || col == 0
    if(row == 0){
        if(col == 0){ // top left edge
            return /* the cell itself*/ data[index + 1] +
                data[index + N] + data[index + N + 1];
        } else if (col == N){ // top right
            return data[index - 1] + /* the cell itself */
                data[index + N - 1] + data[index + N];
        } else {
            return data[index - 1] + /* the cell itself */ data[index + 1] +
                data[index + N - 1] + data[index + N] + data[index + N + 1];
        }
    } else if (row == N){
        if(col == 0){ // bottom right
            return data[index - N] + data[index - N + 1] +
                /* the cell itself */ data[index + 1];
        } else if (col == N){ // bottom left
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
    } else { // col == N -> outer is used only in this 4 cases
        return data[index - N - 1] + data[index - N] +
            data[index - 1] + /* the cell itself */
            data[index + N - 1] + data[index + N];
    }
}

static inline size_t neighbours_inner(CELL_TYPE* data, size_t row, size_t col)
{
    size_t index = row*N + col;

    return data[index - N - 1] + data[index - N] + data[index - N + 1] +
        data[index - 1] + /* the cell itself */ + data[index + 1] +
        data[index + N - 1] + data[index + N] + data[index + N + 1];
}

static inline _Bool is_alive(CELL_TYPE* data, size_t row, size_t col)
{
    size_t n; 
    if(row == 0 || row == N-1 || col == 0 || col == N-1) // row and col bound checks get skipped in not on border
        n = neighbours(data, row, col);
    else
        n = neighbours_inner(data, row, col);

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

int simulate(CELL_TYPE** m0, CELL_TYPE** m1, size_t cycles)
{
    if(!m0 || !m1) return 1;

    CELL_TYPE *d0 = m0[0], *d1 = m1[0]; 
    typeof(d0) tmp;

#if N < 4
    for(size_t t = 0; t < cycles; ++t){
        for(size_t i = 0; i < N; ++i){
            for(size_t j = 0; j < N; ++j){
                d1[i*N + j] = is_alive(d0, i, j);
            }
        }
#else // Assuming N>=4 borders are computed manually and the for loop is only used for inner data, skipping checks
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
#endif

        // Swamp buffers to avoid rewriting
        tmp = d0;
        d0 = d1;
        d1 = tmp;

#if DEBUG > 1
        print_world(m1, m0, t, 0);
        usleep(100000);
        typeof(m0) mtmp = m0;
        m0 = m1;
        m1 = mtmp;
#endif        
    }

    return 0;
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

int load_world(CELL_TYPE **m, char *filename)
{
    if(!m || !filename) return 1;

    if(!strcmp(filename, "RANDOM")){
        srand(time(NULL));
        for (size_t i = 0; i < N; ++i) {
            for (size_t j = 0; j < N; ++j) {
                m[i][j] = rand()%2;
            }
        }
    }

    FILE *f = fopen(filename, "r");
    if (!f) return 1;

    char ch;
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j < N; ) {
            // " %1c" automatically skips all whitespace/newlines
            if (fscanf(f, "%1c", &ch) != 1) break;

            if(ch == '\n') break;
            else m[i][j++] = ch == '1' ? 1 : 0;
        }
    }

    fclose(f);

    return 0;
}

static inline int start_timer()
{
    return gettimeofday(&start, NULL);
}

static inline double stop_timer()
{
    gettimeofday(&end, NULL);

	return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}


int print_world(CELL_TYPE **world, CELL_TYPE **old_world, size_t iteration, _Bool color)
{
    if(!world || !old_world) return 1;

    // Clear screen;
    //system("clear");

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