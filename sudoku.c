// Sudoku puzzle verifier and solver

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Structure for passing data to threads
typedef struct {
  int id;           // Thread ID
  int psize;        // Puzzle size
  int **grid;       // The Sudoku grid
  int *result_arr;  // Shared array for results
} parameters;

// Worker function to check all columns
void *check_cols(void *params) {
  // Your logic here
  return NULL;
}

// Worker function to check all rows
void *check_rows(void *params) {
  // Your logic here
  return NULL;
}

// Worker function to check a subgrid
void *check_subgrid(void *params) {
  // Your logic here
  return NULL;
}

// takes puzzle size and grid[][] representing sudoku puzzle
// and tow booleans to be assigned: complete and valid.
// row-0 and column-0 is ignored for convenience, so a 9x9 puzzle
// has grid[1][1] as the top-left element and grid[9]9] as bottom right
// A puzzle is complete if it can be completed with no 0s in it
// If complete, a puzzle is valid if all rows/columns/boxes have numbers from 1
// to psize For incomplete puzzles, we cannot say anything about validity
// to psize. For incomplete puzzles, we cannot say anything about validity.
void checkPuzzle(int psize, int **grid, bool *complete, bool *valid) {
  // YOUR CODE GOES HERE and in HELPER FUNCTIONS
  *valid = true;
  // For a 9x9 puzzle, this would be 11 threads (9 subgrids, 1 for rows, 1 for cols)
  // For NxN, it's N subgrids, 1 for rows, 1 for cols = N+2 threads
  int num_threads = psize + 2;
  pthread_t threads[num_threads];
  int thread_results[num_threads];

  // TODO: Implement the puzzle solver loop here.
  // You should repeatedly try to fill in '0's until no more can be filled.
  // The logic below is for validation of a complete puzzle.

  // Check if the puzzle is complete (no zeros)
  *complete = true;
  for (int i = 1; i <= psize; i++) {
    for (int j = 1; j <= psize; j++) {
      if (grid[i][j] == 0) {
        *complete = false;
        break;
      }
    }
    if (!*complete) break;
  }

  // If the puzzle is not complete, it cannot be valid yet.
  if (!*complete) {
    *valid = false;
    // Here you would run your solver logic.
    // After solving, you would re-check for completeness and then validity.
    printf("Puzzle is not complete. Solver not yet implemented.\n");
    return;
  }

  // --- Multi-threaded Validation ---
  // TODO: Create and launch threads.
  // Example for one thread:
  // parameters *data = (parameters *) malloc(sizeof(parameters));
  // data->id = 0; ...
  // pthread_create(&threads[0], NULL, check_rows, data);

  // TODO: Join all threads

  // TODO: Check the thread_results array. If any are 0, set *valid = false.
  *valid = true; // Placeholder
}

// takes filename and pointer to grid[][]
// returns size of Sudoku puzzle and fills grid
int readSudokuPuzzle(char *filename, int ***grid) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Could not open file %s\n", filename);
    exit(EXIT_FAILURE);
  }
  int psize;
  fscanf(fp, "%d", &psize);
  int **agrid = (int **)malloc((psize + 1) * sizeof(int *));
  for (int row = 1; row <= psize; row++) {
    agrid[row] = (int *)malloc((psize + 1) * sizeof(int));
    for (int col = 1; col <= psize; col++) {
      fscanf(fp, "%d", &agrid[row][col]);
    }
  }
  fclose(fp);
  *grid = agrid;
  return psize;
}

// takes puzzle size and grid[][]
// prints the puzzle
void printSudokuPuzzle(int psize, int **grid) {
  printf("%d\n", psize);
  for (int row = 1; row <= psize; row++) {
    for (int col = 1; col <= psize; col++) {
      printf("%d ", grid[row][col]);
    }
    printf("\n");
  }
  printf("\n");
}

// takes puzzle size and grid[][]
// frees the memory allocated
void deleteSudokuPuzzle(int psize, int **grid) {
  for (int row = 1; row <= psize; row++) {
    free(grid[row]);
  }
  free(grid);
}

// expects file name of the puzzle as argument in command line
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("usage: ./sudoku puzzle.txt\n");
    return EXIT_FAILURE;
  }
  // grid is a 2D array
  int **grid = NULL;
  // find grid size and fill grid
  int sudokuSize = readSudokuPuzzle(argv[1], &grid);
  bool valid = false;
  bool complete = false;
  checkPuzzle(sudokuSize, grid, &complete, &valid);
  printf("Complete puzzle? ");
  printf(complete ? "true\n" : "false\n");
  if (complete) {
    printf("Valid puzzle? ");
    printf(valid ? "true\n" : "false\n");
  }
  printSudokuPuzzle(sudokuSize, grid);
  deleteSudokuPuzzle(sudokuSize, grid);
  return EXIT_SUCCESS;
}
