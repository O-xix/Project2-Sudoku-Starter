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

// --- Function Prototypes ---

bool is_row_valid(int row, int psize, int **grid);
bool is_col_valid(int col, int psize, int **grid);
bool is_subgrid_valid(int start_row, int start_col, int psize, int **grid);

// --- Validation Worker Functions ---

/*
 * The functions check_rows, check_cols, and check_subgrid are worker functions
 * for pthreads. They are used to validate a completed Sudoku puzzle.
 */

/**
 * @brief Worker function to validate all columns of a completed Sudoku puzzle.
 * @param params A void pointer to a parameters struct.
 * @return NULL. The result is written to the shared result_arr.
 */
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

/**
 * @brief Worker function to validate all rows of a completed Sudoku puzzle.
 * @param params A void pointer to a parameters struct.
 * @return NULL. The result is written to the shared result_arr.
 */
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

/**
 * @brief Worker function to validate a single subgrid of a completed Sudoku puzzle.
 * @details The specific subgrid is determined by the thread's ID within the
 * parameters struct.
 * @param params A void pointer to a parameters struct.
 * @return NULL. The result is written to the shared result_arr.
 */
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

/**
 * @brief Attempts to solve a single missing number (0) in a given row.
 * @details This function checks if there is exactly one zero in the row. If so,
 * it calculates the missing number and fills it in the grid.
 * @param row The row index (1-based) to check.
 * @param psize The size of the puzzle (e.g., 9 for a 9x9 grid).
 * @param grid The 2D array representing the Sudoku puzzle.
 * @return 1 if a zero was filled, 0 otherwise.
 */
int solve_row(int row, int psize, int **grid) { // NOLINT
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

/**
 * @brief Attempts to solve a single missing number (0) in a given column.
 * @details This function checks if there is exactly one zero in the column. If
 * so, it calculates the missing number and fills it in the grid.
 * @param col The column index (1-based) to check.
 * @param psize The size of the puzzle.
 * @param grid The 2D array representing the Sudoku puzzle.
 * @return 1 if a zero was filled, 0 otherwise.
 */
int solve_col(int col, int psize, int **grid) { // NOLINT
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

/**
 * @brief Attempts to solve a single missing number (0) in a given subgrid.
 * @details This function checks if there is exactly one zero in the subgrid. If
 * so, it calculates the missing number and fills it in the grid.
 * @param start_row The starting row index (1-based) of the subgrid.
 * @param start_col The starting column index (1-based) of the subgrid.
 * @param psize The size of the puzzle.
 * @param grid The 2D array representing the Sudoku puzzle.
 * @return 1 if a zero was filled, 0 otherwise.
 */
int solve_subgrid(int start_row, int start_col, int psize, int **grid) { // NOLINT
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

/**
 * @brief Checks if a given row is valid in a completed Sudoku puzzle.
 * @details A row is valid if it contains all numbers from 1 to psize exactly
 * once.
 * @param row The row index (1-based) to check.
 * @param psize The size of the puzzle.
 * @param grid The 2D array representing the Sudoku puzzle.
 * @return true if the row is valid, false otherwise.
 */
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

/**
 * @brief Checks if a given column is valid in a completed Sudoku puzzle.
 * @details A column is valid if it contains all numbers from 1 to psize
 * exactly once.
 * @param col The column index (1-based) to check.
 * @param psize The size of the puzzle.
 * @param grid The 2D array representing the Sudoku puzzle.
 * @return true if the column is valid, false otherwise.
 */
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

/**
 * @brief Checks if a given subgrid is valid in a completed Sudoku puzzle.
 * @details A subgrid is valid if it contains all numbers from 1 to psize
 * exactly once.
 * @param start_row The starting row index (1-based) of the subgrid.
 * @param start_col The starting column index (1-based) of the subgrid.
 * @param psize The size of the puzzle.
 * @param grid The 2D array representing the Sudoku puzzle.
 * @return true if the subgrid is valid, false otherwise.
 */
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

/**
 * @brief Worker function that attempts to solve all rows in a puzzle.
 * @details Iterates through each row, calling solve_row. Uses a mutex to safely
 * update a shared counter if any zeros are filled.
 * @param params A void pointer to a parameters struct.
 */
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

/**
 * @brief Worker function that attempts to solve all columns in a puzzle.
 * @details Iterates through each column, calling solve_col. Uses a mutex to
 * safely update a shared counter if any zeros are filled.
 * @param params A void pointer to a parameters struct.
 */
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

/**
 * @brief Worker function that attempts to solve a single subgrid in a puzzle.
 * @details Determines its assigned subgrid from its thread ID and calls
 * solve_subgrid. Uses a mutex to safely update a shared counter if a zero is
 * filled.
 * @param params A void pointer to a parameters struct.
 */
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

/**
 * @brief Main logic function to solve and/or validate a Sudoku puzzle.
 * @details This function first checks if the puzzle is complete. If not, it
 * enters an iterative solving phase, repeatedly launching solver threads until
 * no more 'easy' cells can be filled. After attempting to solve, it launches
 * validation threads to check if the final grid is a complete and valid Sudoku
 * solution.
 * @param psize The size of the puzzle (e.g., 9 for a 9x9 grid).
 * @param grid The 2D array representing the Sudoku puzzle.
 * @param complete A pointer to a boolean that will be set to true if the puzzle
 * is complete, false otherwise.
 * @param valid A pointer to a boolean that will be set to true if the puzzle is
 * valid, false otherwise.
 */
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

/**
 * @brief Reads a Sudoku puzzle from a file.
 * @param filename The path to the puzzle file.
 * @param grid A pointer to a 2D int array that will be allocated and filled
 * with the puzzle data.
 * @return The size of the puzzle.
 */
int readSudokuPuzzle(char *filename, int ***grid) { // NOLINT
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

/**
 * @brief Prints the Sudoku puzzle to the console.
 * @param psize The size of the puzzle.
 * @param grid The 2D array representing the puzzle.
 */
void printSudokuPuzzle(int psize, int **grid) { // NOLINT
  printf("%d\n", psize);
  for (int row = 1; row <= psize; row++) {
    for (int col = 1; col <= psize; col++) {
      printf("%d ", grid[row][col]);
    }
    printf("\n");
  }
  printf("\n");
}

/**
 * @brief Frees the memory allocated for the Sudoku grid.
 * @param psize The size of the puzzle.
 * @param grid The 2D array to be freed.
 */
void deleteSudokuPuzzle(int psize, int **grid) { // NOLINT
  for (int row = 1; row <= psize; row++) {
    free(grid[row]);
  }
  free(grid);
}

// expects file name of the puzzle as argument in command line
/**
 * @brief Main entry point of the program.
 * @param argc The number of command-line arguments.
 * @param argv An array of command-line arguments. Expects one argument: the puzzle filename.
 */
int main(int argc, char **argv) { // NOLINT
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