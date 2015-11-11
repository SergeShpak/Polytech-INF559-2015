/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, tmp, idx_diag;
    int str, str_curr, str_step;
    int col, col_curr, col_step; 
     
   /* Code for the first test case  */
    if (32 == M && 32 == N) {
        str_curr = 0;
        col_curr = 0;
        str_step = 0;
        col_step = 0;
        /* Loop for 4 columns 32 * 8  */
        for (i = 0; i < 4; i++) {
            /* Loop for 2 blocks 16 * 8  */
            for (j = 0; j < 2; j++) {
                /* Loop to iterate through 16 lines */
                for (str = str_curr; str < str_curr + 16; str++) {
                    /* Loop to iterate through 8 columns */
                    for (col = col_curr; col < col_curr + 8; col++) {
                        if (col == str) {
                            tmp = A[str][col];
                            idx_diag = col; 
                        }
                        B[col][str] = A[str][col];
                    }
                    if (-1 != idx_diag) {
                        B[idx_diag][idx_diag] = tmp;
                        idx_diag = -1;
                    }
                }
                str_curr += 16;
            }
            str_curr = 0;
            col_curr += 8;
        }
        return;        
    }
    
    
    /* Code for all other cases */
    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
    
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

char trans_first_desc[] = "First try";
void trans_first(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, tmp, idx_diag;
    int str, str_curr, str_step;
    int col, col_curr, col_step; 
     
   /* Code for the first test case  */
    if (32 == M && 32 == N) {
        str_curr = 0;
        col_curr = 0;
        str_step = 0;
        col_step = 0;
        /* Loop for 4 columns 32 * 8  */
        for (i = 0; i < 2; i++) {
            /* Loop for 2 blocks 16 * 8  */
            for (j = 0; j < 4; j++) {
                /* Loop to iterate through 16 lines */
                for (str = str_curr; str < str_curr + 16; str++) {
                    /* Loop to iterate through 8 columns */
                    for (col = col_curr; col < col_curr + 8; col++) {
                        if (col == str) {
                            tmp = A[str][col];
                            idx_diag = col;
                            continue;
                        }
                        B[col][str] = A[str][col];
                    }
                    B[idx_diag][idx_diag] = tmp;
                }
                col_curr += 8;
            }
            str_curr += 16;
            col_curr = 0;
        }
        return;        
    }
    
    /* Code for all other cases */
    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
    
}

char trans_second_desc[] = "Second try";
void trans_second(int M, int N, int A[N][M], int B[M][N]) {
    int block_row, block_col, idx1, idx2;
    int *current_row;
    int i, j, tmp;

    /* The idea is to process block after block. The problem is that the same rows
        in matrices A and B are cached to the same sets. Therefore when working with
        rows woth the same indexes we get cache misses. 
        We can avoid that by storing diagonal elements in a temporary varibale 
        and after finishing with a row in A writing it 
        from the temporary variable to B. */
    if (32 == M && 32 == N) {
        for (block_row = 0; block_row < N; block_row += 8) {
            for (block_col = 0; block_col < M; block_col += 8) {
                for (idx1 = block_row; idx1 < block_row + 8; idx1++) {
                    if (block_row == block_col) {
                        /* Diagonal element  */
                        tmp = A[idx1][idx1];
                    }
                    for (idx2 = block_col; idx2 < block_col + 8; idx2++) {
                        if (idx1 != idx2) {
                            B[idx2][idx1] = A[idx1][idx2];
                        }
                        
                    }
                    if (block_row == block_col) {
                        B[idx1][idx1] = tmp;
                    }
                }
            }
        }
    }

    if (64 == M && 64 == N) {
        /* The data is still divided into 8x8 blocks  */
        for (block_row = 0; block_row < M; block_row += 8) {
            for (block_col = 0; block_col < N; block_col += 8) {
                for (idx1 = block_row; idx1 < block_row + 4; idx1++) {
                    if (block_row == block_col) {
                        tmp = A[idx1][idx1];  // diagonal element
                    }
                    for (idx2 = block_col; idx2 < block_col + 4; idx2++) {
                        if (idx1 != idx2) {
                            B[idx2][idx1] = A[idx1][idx2];
                        }
                    }
                    for (idx2 = block_col + 4; idx2 block_col + 8; idx2++) {
                        if (idx1 != idx2) {
                            
                        }
                    }
                }
                if (block_row == block_col) {
                }
                /* We further divide the 8x8 block into 4 4x4 blocks  */    
            }
        }
    }
    
    /* A trivial case, no problems with cache misses while working on same rows, etc...  */
    if (61 == M && 67 == N) {
        for (block_row = 0; block_row < N; block_row += 8) {
            for (block_col = 0; block_col < M; block_col += 8) {
                for (idx2 = block_col; idx2 < block_col + 8 && idx2 < M; idx2++) {
                    for (idx1 = block_row; idx1 < block_row + 8 && idx1 < N; idx1++) {
                        B[idx2][idx1] = A[idx1][idx2];
                    }
                }
            }
        }
    }
}
 
/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans_first, trans_first_desc); 
    registerTransFunction(trans_second, trans_second_desc);

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

