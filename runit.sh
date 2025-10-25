#!/bin/bash

# Script to compile and run sudoku program
rm -f sudoku
gcc -Wall -Wextra -pthread -lm -std=c99 sudoku.c -o sudoku
./sudoku puzzle9-valid.txt
echo "________________________________"
./sudoku puzzle2-invalid.txt
echo "________________________________"
./sudoku puzzle2-fill-valid.txt
echo "________________________________"
./sudoku puzzle2-valid.txt

# to check for memory leaks, use
# valgrind ./sudoku puzzle9-good.txt

# to fix formating use
# clang-format -i main.c

# if clang-format does not work 
# use 'source scl_source enable llvm-toolset-7.0' and try again

# if using GitHub, you can run the program on GitHub servers and see
# the result. Repository > Actions > Run Workflow
