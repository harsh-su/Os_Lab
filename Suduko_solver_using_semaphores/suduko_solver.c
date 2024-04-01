#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define N 9

int sudoku[N][N];
sem_t row_semaphores[N];
sem_t col_semaphores[N];
sem_t grid_semaphores[N][N];

typedef struct {
    int row;
    int col;#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define SIZE 9#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define SIZE 9

// Sudoku puzzle to solve (0 represents empty cells)
int sudoku_grid[SIZE][SIZE] = {
    {5, 3, 0, 0, 7, 0, 0, 0, 0},
    {6, 0, 0, 1, 9, 5, 0, 0, 0},
    {0, 9, 8, 0, 0, 0, 0, 6, 0},
    {8, 0, 0, 0, 6, 0, 0, 0, 3},
    {4, 0, 0, 8, 0, 3, 0, 0, 1},
    {7, 0, 0, 0, 2, 0, 0, 0, 6},
    {0, 6, 0, 0, 0, 0, 2, 8, 0},
    {0, 0, 0, 4, 1, 9, 0, 0, 5},
    {0, 0, 0, 0, 8, 0, 0, 7, 9}
};

// Semaphores for each cell in the grid
sem_t semaphores[SIZE][SIZE];

typedef struct {
    int row;
    int col;
} CellPosition;


// Sudoku puzzle to solve (0 represents empty cells)
int sudoku_grid[SIZE][SIZE] = {
    {5, 3, 0, 0, 7, 0, 0, 0, 0},
    {6, 0, 0, 1, 9, 5, 0, 0, 0},
    {0, 9, 8, 0, 0, 0, 0, 6, 0},
    {8, 0, 0, 0, 6, 0, 0, 0, 3},
    {4, 0, 0, 8, 0, 3, 0, 0, 1},
    {7, 0, 0, 0, 2, 0, 0, 0, 6},
    {0, 6, 0, 0, 0, 0, 2, 8, 0},
    {0, 0, 0, 4, 1, 9, 0, 0, 5},
    {0, 0, 0, 0, 8, 0, 0, 7, 9}
};

// Semaphores for each cell in the grid
sem_t semaphores[SIZE][SIZE];

typedef struct {
    int row;
    int col;
} CellPosition;

} Cell;

bool is_valid(int row, int col, int num) {
    // Check if 'num' is valid in the current row, column, and grid
    // Use sem_wait and sem_post to acquire and release semaphores
    // Return true if 'num' is valid, false otherwise

    // Check row
    sem_wait(&row_semaphores[row]);
    for (int i = 0; i < N; i++) {
        if (sudoku[row][i] == num) {
            sem_post(&row_semaphores[row]);
            return false;
        }
    }
    sem_post(&row_semaphores[row]);

    // Check column
    sem_wait(&col_semaphores[col]);
    for (int i = 0; i < N; i++) {
        if (sudoku[i][col] == num) {
            sem_post(&col_semaphores[col]);
            return false;
        }
    }
    sem_post(&col_semaphores[col]);

    // Check grid
    int startRow = row - row % 3;
    int startCol = col - col % 3;
    sem_wait(&grid_semaphores[startRow][startCol]);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (sudoku[i + startRow][j + startCol] == num) {
                sem_post(&grid_semaphores[startRow][startCol]);
                return false;
            }
        }
    }
    sem_post(&grid_semaphores[startRow][startCol]);

    return true;
}

bool find_empty_cell(Cell* cell) {
    // Find the next empty cell in the Sudoku puzzle
    // Return false if no empty cell is found, true otherwise

    for (int row = 0; row < N; row++) {
        for (int col = 0; col < N; col++) {
            if (sudoku[row][col] == 0) {
                cell->row = row;
                cell->col = col;
                return true;
            }
        }
    }
    return false;
}

bool solve_sudoku() {
    Cell cell;
    if (!find_empty_cell(&cell)) {
        // All cells are filled, the puzzle is solved
        return true;
    }

    int row = cell.row;
    int col = cell.col;

    for (int num = 1; num <= 9; num++) {
        if (is_valid(row, col, num)) {
            // Place the number in the cell
            sudoku[row][col] = num;

            // Recursive call to solve the next cell
            if (solve_sudoku()) {
                return true; // If the puzzle is solved, return true
            }

            // Backtrack by resetting the cell
            sudoku[row][col] = 0;
        }
    }

    return false; // No solution found
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    // Open the input file
    FILE* file = fopen(argv[1], "r");
   
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }

    // Read Sudoku puzzle from the file
    for (int i = 0; i < N; i++) {
    int temp;
    fscanf(file, "%d", &temp);
        for (int j = N-1; j >= 0; j--) {
            sudoku[i][j]=temp%10;
            temp/=10;
        }
    }

    fclose(file);

    // Initialize semaphores
    for (int i = 0; i < N; i++) {
        sem_init(&row_semaphores[i], 0, 1);
        sem_init(&col_semaphores[i], 0, 1);
    }
    for (int i = 0; i < N; i += 3) {
        for (int j = 0; j < N; j += 3) {
            sem_init(&grid_semaphores[i][j], 0, 1);
        }
    }

    // Solve the Sudoku puzzle
    if (solve_sudoku()) {
    FILE* file1 = fopen(argv[2], "w");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }
        // Print the solved Sudoku puzzle
        printf("Solved Sudoku stored in output file\n");
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                fprintf(file1,"%d", sudoku[i][j]);
            }
            fprintf(file1,"\n");
        }
        fclose(file1);
    } else {
        printf("No solution found.\n");
    }

    // Clean up semaphores
    for (int i = 0; i < N; i++) {
        sem_destroy(&row_semaphores[i]);
        sem_destroy(&col_semaphores[i]);
    }
    for (int i = 0; i < N; i += 3) {
        for (int j = 0; j < N; j += 3) {
            sem_destroy(&grid_semaphores[i][j]);
        }
    }
