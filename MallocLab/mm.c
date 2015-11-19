/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Meh",
    /* First member's full name */
    "Sergey Shpak",
    /* First member's email address */
    "sergey.shpak@polytechnique.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};


/* BASIC CONSTANTS AND MACROS */


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* single word size in bytes */
#define WSIZE   4

/* double word size in bytes */
#define DSIZE   8

/* we extend heap by this amount of bytes */
#define CHUNKSIZE       4096

/* Value to indicate that the block is allocated */
#define ALLOC_BIT       1

/* Value defining until which size a free blog can be splitted */
#define MIN_FREE_BLOCK_SIZE     16  // Currently equals to the min block size


/* Implicit free list macros */

#define MAX(x, y)       ((x) > (y) ? (x) : (y))

/* Get the word stored at the address pointed by p */
#define GET(p)          (*(unsigned int *)(p))

/* Write value of val (word) to the address pointed by p */
#define PUT(p, val)     (*(unsigned int *)(p) = (val))

/* Pack size and is_allocated (is_alloc) bit together */
#define PACK(size, is_alloc)    ((size) | (is_alloc))

/* Get size stored at the address, which bp points to.
    Should be used with the header or footer blocks.
    ~0x7 is a mask with all ones as bits except three LSBs. */
#define GET_SIZE(p)    (GET(p) & ~0x7)

/* Get allocated bit stored at the address, which bp points at.
    Should be used with the header of footer blocks */

#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Compute address of the block pointer (bp) header */
#define HDRP(bp)        ((char *)(bp) - WSIZE)

/* Compute address of the block pointer (bp) footer */
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Compute address of the next block after the one
    that bp points at */
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE((char *)(bp)- WSIZE))

/* Compute address of the previous block before the one
    that bp points at */
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

/* End of implicit free list macros */


/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))


/* END OF BASIC CONSTANTS AND MACROS */


/* GLOBAL VARIABLES */

void *heap_listp;  // points to the heap starting block

/* END OF GLOBAL VARIABLES */


/* FUNCTION PROTOTYPES */

static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void exit_with_error(char *message);
static void allocate_free_block(void *bp, size_t size);

/* END OF FUNCTION PROTOTYPES */


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* heap_listp is the pointer to the heap starting block.
        It is a global variable  */
    heap_listp = mem_sbrk(4 * WSIZE);
    if (heap_listp == (void *)-1) {
        /* Case if there's no enough memory */
        return -1;
    }
    PUT(heap_listp, 0);
    /* Writing the prologue header */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    /* Writing the prologue footer */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    /* Writing the epilogue header */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    /* We point heap_listp at the heap part
        right after the prologue */
    heap_listp += (2 * WSIZE);

    if (NULL == extend_heap(CHUNKSIZE / WSIZE)) {
        /* Case if there's not enough memory to extend the heap
            by the chunksize */
        return -1;
    }

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;  // adjusted size
    size_t extend_size;  // amount to extend heap if no fit
    void *bp;
    if (0 == size) {
        return NULL;
    }
    /* Aligning size to allocate to 8 */
    if (size <= DSIZE) {
        asize = 2 * DSIZE;  // to align to 8
    }
    if (size > DSIZE) {
        asize = DSIZE * ((size / DSIZE) + 1);  // ??
    }

    bp = find_fit(asize);

    /* A suitable free block was found */
    if (NULL != bp) {
        allocate_free_block(bp, asize);
        return bp;
    }
    
    /* No free block of a suitale size was found,
        extending heap */
    extend_size = MAX(asize, CHUNKSIZE);
    bp = extend_heap(extend_size / WSIZE);

    /* Cannot extend heap further */
    if (NULL == bp) {
        return NULL;
    }

    allocate_free_block(bp, asize);
    return bp;
}

/*
 * Free an allocated block of memory.
 * If an unallocated block of memory is tried to be freed,
 *      the program execution is interrupted.
 *
 *
 * ptr - pointer to the first byte of the allocated block
 */
void mm_free(void *ptr)
{
    int alloc_bit = GET_ALLOC(ptr);
    size_t block_size = GET_SIZE(ptr);
    /* Check if trying to free an unallocated block */
    if (0 == alloc_bit) {
        exit_with_error("ERROR: tried to free an unallocated block. The execution will be interrupted.");
    }
    PUT(HDRP(ptr), PACK(block_size, 0));
    PUT(FTRP(ptr), PACK(block_size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copy_size;
  
    newptr = mm_malloc(size);
    /* No block of proper size can be allocated */
    if (newptr == NULL)
      return NULL;
    copy_size = GET_SIZE(HDRP(ptr));
    /* We will copy the amount of bytes stored in the 'size' varialbe */
    if (size < copy_size)
      copy_size = size;
    memcpy(newptr, oldptr, copy_size);
    mm_free(oldptr);
    return newptr;
}

/*
*  extend_heap - Extends heap by the given size (amount of words)
*/
void *extend_heap(size_t words) {
    /* Allocate an even number of words to maintain alignment */
    size_t size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    volatile char *bp = ((char *)(mem_sbrk(size)));

    if (-1 == (long)(bp = mem_sbrk(size))) {
        return NULL;
    }
    
    /* Add header, footer to the allocated block
        and add a new epilogue to the heap */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // new epilogue block
    
    bp = coalesce(bp);
    return bp;
}

void *coalesce(void *bp) {
    size_t combined_blocks_size = 0;
    /* Get the allocation byte of the previous block */
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    /* Get the allocation byte of the next block */
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    /* Get size of the block that we coalesce */
    size_t current_block_size = GET_SIZE(HDRP(bp));

    /* Previous and next block are allocated */
    if (0 != prev_alloc && 0 != next_alloc) {
        return bp;
    }

    /* Previous block is free, next block is allocated */
    if (0 == prev_alloc && 0 != next_alloc) {
        combined_blocks_size = current_block_size + 
            GET_SIZE(HDRP(PREV_BLKP(bp)));  // previous block size
        PUT(HDRP(PREV_BLKP(bp)), PACK(combined_blocks_size, 0));
        /* As the new size is stored in the header of the previous block
            the FTRP macros treats the two blocks as one */
        PUT(FTRP(bp), PACK(combined_blocks_size, 0));
        bp = PREV_BLKP(bp);
        return bp;
    }

    /* Previous block is allocated, next block is free */
    if (0 != prev_alloc && 0 == next_alloc) {
        combined_blocks_size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        /* After the following operation macros process bp 
            as a block of size combined_blocks_size */
        PUT(HDRP(bp), PACK(combined_blocks_size, 0));
        /* We don't use PUT(NEXT_BLKP(bp)) here as after changing
            bp header macros treat the footer of the next block as
            bp's footer */
        PUT(FTRP(bp), PACK(combined_blocks_size, 0));
        return bp;    
    }
    
    /* Previous block is free, next block is free */
    if (0 == prev_alloc && 0 == next_alloc) {
        combined_blocks_size = GET_SIZE(HDRP(NEXT_BLKP(bp))) + 
                                GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(combined_blocks_size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(combined_blocks_size, 0));
        bp = PREV_BLKP(bp);
        return bp;
    }
    
    exit_with_error("Error in the coalesce function. Status of neighbour blocks is undefined.");
    return NULL;  // "default" return
}

/*
 *  Find and return a free block with enough size.
 *
 *  size - the value is the amount of bytes 
 *          that the free block should be able to store
 */
void *find_fit(size_t size) {
    int alloc_bit;
    size_t size_of_current_block;
    void *curr_block_pointer = heap_listp;
    /* Iterate over all blocks, trying to find a fit */
    do {
        alloc_bit = GET_ALLOC(curr_block_pointer);
        size_of_current_block = GET_SIZE(curr_block_pointer);
        /* the following condition means we got to the heap epilogue,
            which means there's no suitable free block */
        if (1 == alloc_bit && 0 == size_of_current_block) {
            return NULL;
        }
        curr_block_pointer = NEXT_BLKP(curr_block_pointer);
    } while (0 != alloc_bit && size_of_current_block < size);
    return curr_block_pointer;    
}

void exit_with_error(char *message) {
    printf("%s\n", message);
    exit(EXIT_FAILURE);
}

/* Function to allocate a free block. 
    Decision whether to split or not takes place here.

    bp - pointer with the address of the allocated free block
    size - size needed to be allocated */
void allocate_free_block(void *bp, size_t size) {
    size_t free_block_size = GET_SIZE(bp);
    /* amount of storage in the block after potential splitting */
    size_t splitted_free_block_size = free_block_size - size;
    size_t size_to_store = size;  // value to write to the header and footer
    if (splitted_free_block_size < MIN_FREE_BLOCK_SIZE) {
    /* If the potentially splitted free block has smaller amount
        of space than MIN_FREE_BLOCK_SIZE we do not split and occupy
        the whole free block */
        size_to_store = free_block_size;
    }
    PUT(HDRP(bp), PACK(size_to_store, ALLOC_BIT));
    PUT(FTRP(bp), PACK(size_to_store, ALLOC_BIT));
}







