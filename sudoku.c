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
  int *filled_count; // Shared counter for filled zeros
  pthread_mutex_t *lock; // Mutex for the shared counter
} parameters;

// --- Validation Worker Functions ---

/*
 * The functions check_rows, check_cols, and check_subgrid are worker functions
 * for pthreads. They are used to validate a completed Sudoku puzzle.
 */

// Worker function to validate all columns
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

// Worker function to validate all rows
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

// Worker function to validate a subgrid
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

// --- Solver Worker Functions ---

/*
 * The functions solve_row, solve_col, and solve_subgrid are helper functions
 * that attempt to fill a single '0' in a given unit (row, col, or subgrid).
 * They return 1 if they fill a zero, and 0 otherwise.
 */

int solve_row(int row, int psize, int **grid) {
  bool seen[psize + 1];
  int zero_count = 0;
  int zero_col = -1;
  long sum = 0;
  for (int i = 0; i <= psize; i++) { seen[i] = false; }

  for (int col = 1; col <= psize; col++) {
    int num = grid[row][col];
    if (num == 0) {
      zero_count++;
      zero_col = col;
    } else if (num > 0 && num <= psize) {
      seen[num] = true;
      sum += num;
    }
  }

  if (zero_count == 1) {
    long expected_sum = (long)psize * (psize + 1) / 2;
    int missing_num = expected_sum - sum;
    if (missing_num > 0 && missing_num <= psize && !seen[missing_num]) {
      grid[row][zero_col] = missing_num;
      return 1; // We filled a zero
    }
  }
  return 0;
}

int solve_col(int col, int psize, int **grid) {
  bool seen[psize + 1];
  int zero_count = 0;
  int zero_row = -1;
  long sum = 0;
  for (int i = 0; i <= psize; i++) { seen[i] = false; }

  for (int row = 1; row <= psize; row++) {
    int num = grid[row][col];
    if (num == 0) {
      zero_count++;
      zero_row = row;
    } else if (num > 0 && num <= psize) {
      seen[num] = true;
      sum += num;
    }
  }

  if (zero_count == 1) {
    long expected_sum = (long)psize * (psize + 1) / 2;
    int missing_num = expected_sum - sum;
    if (missing_num > 0 && missing_num <= psize && !seen[missing_num]) {
      grid[zero_row][col] = missing_num;
      return 1;
    }
  }
  return 0;
}

int solve_subgrid(int start_row, int start_col, int psize, int **grid) {
  int subgrid_size = sqrt(psize);
  bool seen[psize + 1];
  int zero_count = 0;
  int zero_row = -1, zero_col = -1;
  long sum = 0;
  for (int i = 0; i <= psize; i++) { seen[i] = false; }

  for (int r_off = 0; r_off < subgrid_size; r_off++) {
    for (int c_off = 0; c_off < subgrid_size; c_off++) {
      int num = grid[start_row + r_off][start_col + c_off];
      if (num == 0) {
        zero_count++;
        zero_row = start_row + r_off;
        zero_col = start_col + c_off;
      } else if (num > 0 && num <= psize) {
        seen[num] = true;
        sum += num;
      }
    }
  }

  if (zero_count == 1) {
    long expected_sum = (long)psize * (psize + 1) / 2;
    int missing_num = expected_sum - sum;
    if (missing_num > 0 && missing_num <= psize && !seen[missing_num]) {
      grid[zero_row][zero_col] = missing_num;
      return 1;
    }
  }
  return 0;
}

// --- Validation Helper Functions ---

/*
 * The functions is_row_valid, is_col_valid, and is_subgrid_valid are helper
 * functions that check for correctness (no duplicates, all numbers 1-psize).
 * They are used by the validation worker threads.
 */

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

// --- Solver Thread Entry Points ---

void *solve_rows_worker(void *params) {
  parameters *p = (parameters *)params;
  int filled_this_thread = 0;
  for (int i = 1; i <= p->psize; i++) {
    filled_this_thread += solve_row(i, p->psize, p->grid);
  }
  if (filled_this_thread > 0) {
    pthread_mutex_lock(p->lock);
    *(p->filled_count) += filled_this_thread;
    pthread_mutex_unlock(p->lock);
  }
  free(p);
  return NULL;
}

void *solve_cols_worker(void *params) {
  parameters *p = (parameters *)params;
  int filled_this_thread = 0;
  for (int i = 1; i <= p->psize; i++) {
    filled_this_thread += solve_col(i, p->psize, p->grid);
  }
  if (filled_this_thread > 0) {
    pthread_mutex_lock(p->lock);
    *(p->filled_count) += filled_this_thread;
    pthread_mutex_unlock(p->lock);
  }
  free(p);
  return NULL;
}

void *solve_subgrid_worker(void *params) {
  parameters *p = (parameters *)params;
  int subgrid_size = sqrt(p->psize);
  int subgrid_idx = p->id - 2; // 0-based index
  int start_row = (subgrid_idx / subgrid_size) * subgrid_size + 1;
  int start_col = (subgrid_idx % subgrid_size) * subgrid_size + 1;

  if (solve_subgrid(start_row, start_col, p->psize, p->grid) > 0) {
    pthread_mutex_lock(p->lock);
    *(p->filled_count) += 1;
    pthread_mutex_unlock(p->lock);
  }
  free(p);
  return NULL;
}

// takes puzzle size and grid[][] representing sudoku puzzle
// and tow booleans to be assigned: complete and valid.
// row-0 and column-0 is ignored for convenience, so a 9x9 puzzle
// has grid[1][1] as the top-left element and grid[9]9] as bottom right
// A puzzle is complete if it can be completed with no 0s in it
// If complete, a puzzle is valid if all rows/columns/boxes have numbers from 1
// to psize. For incomplete puzzles, validity is only checked after solving.
void checkPuzzle(int psize, int **grid, bool *complete, bool *valid) {
  // For a 9x9 puzzle, this would be 11 threads (9 subgrids, 1 for rows, 1 for cols)
  // For NxN, it's N subgrids, 1 for rows, 1 for cols = N+2 threads
  int num_threads = psize + 2;
  pthread_t threads[num_threads];

  // TODO: Implement the puzzle solver loop here.
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

  // If the puzzle is not complete, try to solve it.
  if (!*complete) {
    *valid = false;
    int zeros_filled_in_pass;
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);

    do {
      zeros_filled_in_pass = 0;
      // Launch solver threads for rows, cols, and subgrids
      for (int i = 0; i < num_threads; i++) {
        parameters *data = (parameters *)malloc(sizeof(parameters));
        data->id = i;
        data->psize = psize;
        data->grid = grid;
        data->filled_count = &zeros_filled_in_pass;
        data->lock = &lock;

        if (i == 0) { // Thread 0 for rows
          pthread_create(&threads[i], NULL, solve_rows_worker, data);
        } else if (i == 1) { // Thread 1 for columns
          pthread_create(&threads[i], NULL, solve_cols_worker, data);
        } else { // Threads 2 to N+1 for subgrids
          pthread_create(&threads[i], NULL, solve_subgrid_worker, data);
        }
      }
      // Wait for all solver threads to finish this pass
      for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
      }
    } while (zeros_filled_in_pass > 0); // Repeat if we made progress

    pthread_mutex_destroy(&lock);

    // After solving, re-check if the puzzle is now complete
    *complete = true;
    for (int i = 1; i <= psize; i++) {
      for (int j = 1; j <= psize; j++) {
        if (grid[i][j] == 0) { *complete = false; break; }
      }
      if (!*complete) break;
    }
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
    data->filled_count = NULL; // Not used for validation
    data->lock = NULL;         // Not used for validation

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