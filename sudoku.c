// Sudoku puzzle verifier and solver

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Structure for passing data to threads
typedef struct {
  int id;           // Thread ID
  int psize;        // Puzzle size
  int **grid;       // The Sudoku grid
  int *result_arr;  // Shared array for results
} parameters;

// Worker function to check all columns
void *check_cols(void *params) {
  parameters *p = (parameters *)params;
  p->result_arr[p->id] = 1; // Assume valid
  for (int i = 1; i <= p->psize; i++) {
    if (!is_col_valid(i, p->psize, p->grid)) {
      p->result_arr[p->id] = 0; // Found invalid column
      break;
    }
  }
  free(p);
  return NULL;
}

// Worker function to check all rows
void *check_rows(void *params) {
  parameters *p = (parameters *)params;
  p->result_arr[p->id] = 1; // Assume valid
  for (int i = 1; i <= p->psize; i++) {
    if (!is_row_valid(i, p->psize, p->grid)) {
      p->result_arr[p->id] = 0; // Found invalid row
      break;
    }
  }
  free(p);
  return NULL;
}

// Worker function to check a subgrid
void *check_subgrid(void *params) {
  parameters *p = (parameters *)params;
  int subgrid_size = sqrt(p->psize);
  // Thread IDs for subgrids are 2 to psize+1. Convert to 0-based index.
  int subgrid_idx = p->id - 2;

  // Calculate starting row and column from the 0-based index
  int start_row = (subgrid_idx / subgrid_size) * subgrid_size + 1;
  int start_col = (subgrid_idx % subgrid_size) * subgrid_size + 1;

  if (is_subgrid_valid(start_row, start_col, p->psize, p->grid)) {
    p->result_arr[p->id] = 1;
  } else {
    p->result_arr[p->id] = 0;
  }
  free(p);
  return NULL;
}

// --- Single-threaded validation helpers ---

// Checks if a given row is valid
bool is_row_valid(int row, int psize, int **grid) {
  bool seen[psize + 1];
  for (int i = 0; i <= psize; i++) {
    seen[i] = false;
  }

  for (int col = 1; col <= psize; col++) {
    int num = grid[row][col];
    if (num < 1 || num > psize || seen[num]) {
      return false;
    }
    seen[num] = true;
  }
  return true;
}

// Checks if a given column is valid
bool is_col_valid(int col, int psize, int **grid) {
  bool seen[psize + 1];
  for (int i = 0; i <= psize; i++) {
    seen[i] = false;
  }

  for (int row = 1; row <= psize; row++) {
    int num = grid[row][col];
    if (num < 1 || num > psize || seen[num]) {
      return false;
    }
    seen[num] = true;
  }
  return true;
}

// Checks if a given subgrid is valid
bool is_subgrid_valid(int start_row, int start_col, int psize, int **grid) {
  int subgrid_size = sqrt(psize);
  bool seen[psize + 1];
  for (int i = 0; i <= psize; i++) { seen[i] = false; }

  for (int row = 0; row < subgrid_size; row++) {
    for (int col = 0; col < subgrid_size; col++) {
      int num = grid[start_row + row][start_col + col];
      if (num < 1 || num > psize || seen[num]) { return false; }
      seen[num] = true;
    }
  }
  return true;
}

// takes puzzle size and grid[][] representing sudoku puzzle
// and tow booleans to be assigned: complete and valid.
// row-0 and column-0 is ignored for convenience, so a 9x9 puzzle
// has grid[1][1] as the top-left element and grid[9]9] as bottom right
// A puzzle is complete if it can be completed with no 0s in it
// If complete, a puzzle is valid if all rows/columns/boxes have numbers from 1
// to psize. For incomplete puzzles, we cannot say anything about validity.
void checkPuzzle(int psize, int **grid, bool *complete, bool *valid) {
  // For a 9x9 puzzle, this would be 11 threads (9 subgrids, 1 for rows, 1 for cols)
  // For NxN, it's N subgrids, 1 for rows, 1 for cols = N+2 threads
  int num_threads = psize + 2;
  pthread_t threads[num_threads];

  // TODO: Implement the puzzle solver loop here.
  // You should repeatedly try to fill in '0's until no more can be filled.
  // The logic below is for validation of a complete puzzle.
  // The logic below is for validation of a complete puzzle. We will add
  // threads in a later step.

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
  int thread_results[num_threads];

  // Create and launch threads
  for (int i = 0; i < num_threads; i++) {
    parameters *data = (parameters *)malloc(sizeof(parameters));
    data->id = i;
    data->psize = psize;
    data->grid = grid;
    data->result_arr = thread_results;

    if (i == 0) { // Thread 0 for rows
      pthread_create(&threads[i], NULL, check_rows, data);
    } else if (i == 1) { // Thread 1 for columns
      pthread_create(&threads[i], NULL, check_cols, data);
    } else { // Threads 2 to N+1 for subgrids
      pthread_create(&threads[i], NULL, check_subgrid, data);
    }
  }

  // Wait for all threads to finish
  for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
  }

  // Check the results from all threads
  *valid = true;
  for (int i = 0; i < num_threads; i++) {
    if (thread_results[i] == 0) {
      *valid = false;
      break;
    }
  }
}

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

void check_rows_worker(void *args) {
  parameters *params = (parameters *)args;
  for (int i = 1; i <= params->psize; i++) {
    if (!is_row_valid(i, params->psize, params->grid)) {
      params->result_arr[params->id] = 0;
      return;
    }
    params->result_arr[params->id] = 1;
  }
}

void check_cols_worker(void *args) {
  parameters *params = (parameters *)args;
  for (int i = 1; i <= params->psize; i++) {
    if (!is_col_valid(i, params->psize, params->grid)) {
      params->result_arr[params->id] = 0;
      return;
    }
    params->result_arr[params->id] = 1;
  }
}

void check_subgrid_worker(void *args) {
  