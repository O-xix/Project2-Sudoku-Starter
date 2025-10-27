#!/bin/bash

# Script to compile and run sudoku program
rm -f sudoku
gcc -Wall -Wextra -pthread -lm -std=c99 sudoku.c -o sudoku
./sudoku puzzle2-fill-valid.txt
echo "________________________________puzzle2-fill-valid.txt"
./sudoku puzzle2-invalid.txt
echo "________________________________puzzle2-invalid.txt"
./sudoku puzzle2-valid.txt
echo "________________________________puzzle2-valid.txt"
./sudoku puzzle4-complete-invalid.txt
echo "________________________________puzzle4-complete-invalid.txt"
./sudoku puzzle4-solvable.txt
echo "________________________________puzzle4-solvable.txt"
./sudoku puzzle9-invalid-start.txt
echo "________________________________puzzle9-invalid-start.txt"
./sudoku puzzle9-simple-solve.txt
echo "________________________________puzzle9-simple-solve.txt"
./sudoku puzzle9-unsolvable.txt
echo "________________________________puzzle9-unsolvable.txt"
./sudoku puzzle9-valid.txt
echo "________________________________puzzle9-valid.txt"


# to check for memory leaks, use
# valgrind ./sudoku puzzle9-good.txt

# to fix formating use
# clang-format -i main.c

# if clang-format does not work 
# use 'source scl_source enable llvm-toolset-7.0' and try again

# if using GitHub, you can run the program on GitHub servers and see
# the result. Repository > Actions > Run Workflow
