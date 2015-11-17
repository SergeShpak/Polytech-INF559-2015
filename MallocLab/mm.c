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

/* Implicit free list macros */


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
#define FTRP(bp)        ((char *))


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
    /* We point heap_listp at the heap epilogue 
        starting block */
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
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
	return NULL;
    else {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/*
*  extend_heap - Extends heap by the given size (amount of words)
*/
void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if (-1 == (long)(bp = mem_sbrk(size))) {
        return NULL;
    }

    
}











