/*
******************************
* Sergey SHPAK, sergey.shpak *
******************************
*/

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
    int block_row, block_col, idx1, idx2;
    int tmp, tmp1, tmp2, tmp3, tmp4;

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
        /* Pattern:
            1) Divide data into 8x8 blocks
            2) Divide each block further, into 4x4 blocks: 
                a | b
                -----
                c | d
            3) Transpose A's 'a' block and store it in the B's 'a' block
            4) Transpose A's 'b' block and store it in the B's 'b' block
            5) Store first row of B's 'b' block in temporary variables
            6) Transpose A's 'c' block and store it in the B's 'b' block
            7) Copy B's 'b' block, stored in the temporary variables 
                into the B's 'c' block (row by row)
            8) Transpose A's 'd' block and store it in the B's 'd' block

            Local variables idx1 and idx2 are used as block offsets
            (for rows and columns respectively)
         */
        /* The data is still divided into 8x8 blocks  */
        for (block_row = 0; block_row < M; block_row += 8) {
            for (block_col = 0; block_col < N; block_col += 8) {
                /* Processing "a" sub-block */
                for (idx1 = 0; idx1 < 4; idx1++) {
                    /* Getting a diagonal element */
                    if (block_row == block_col) {
                        tmp1 = A[block_row + idx1][block_row + idx1];
                    }
                    /* Transposing "a" sub-block */
                    for (idx2 = 0; idx2 < 4; idx2++) {
                        if (block_col != block_row || idx1 != idx2) {
                            B[block_col + idx2][block_row + idx1] = A[block_row + idx1][block_col + idx2];
                        }
                    }
                    /* Transposing "b" sub-block 
                        and temporarily storing it in the "b" sub-block of B */
                    for (idx2 = 0; idx2 < 4; idx2++) {
                        B[block_col + idx2][block_row + 4 + idx1] = A[block_row + idx1][block_col + 4 + idx2];
                    }
                    /* Storing a diagonal element previously stored in a temp variabe */
                    if (block_col == block_row) {
                        B[block_col + idx1][block_row + idx1] = tmp1;
                    }
                }
                /* Transposing 'c' sub-block and exchanging B's 'b' and 'c' blocks */
                for (idx1 = 0; idx1 < 4; idx1++) {
                    tmp1 = B[block_col + idx1][block_row + 4];
                    tmp2 = B[block_col + idx1][block_row + 5];
                    tmp3 = B[block_col + idx1][block_row + 6];
                    tmp4 = B[block_col + idx1][block_row + 7];
                    for (idx2 = 0; idx2 < 4; idx2++) {
                        B[block_col + idx1][block_row + 4 + idx2] = A[block_row + 4 + idx2][block_col + idx1];
                    }
                    B[block_col + 4 + idx1][block_row] = tmp1;
                    B[block_col + 4 + idx1][block_row + 1] = tmp2;
                    B[block_col + 4 + idx1][block_row + 2] = tmp3;
                    B[block_col + 4 + idx1][block_row + 3] = tmp4;
                }
                /*  Transposing "d" sub-block */
                for (idx1 = 0; idx1 < 4; idx1++) {
                    /* Getting a diagonal element */
                    if (block_row == block_col) {
                        tmp1 = A[block_row + idx1 + 4][block_row + idx1 + 4];
                    }
                    for (idx2 = 0; idx2 < 4; idx2++) {
                        if (block_col != block_row || idx1 != idx2) {
                            B[block_col + 4 + idx2][block_row + 4 + idx1] = A[block_row + 4 + idx1][block_col + 4 + idx2];
                        }
                    }
                    /* Storing a diagonal element */
                    if (block_col == block_row) {
                        B[block_col + 4 + idx1][block_col + 4 + idx1] = tmp1;
                    }
                }
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
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 


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

