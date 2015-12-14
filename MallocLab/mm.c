/*
* mm.c - Malloc implementation using segregated fits with address-ordered
*        explicit linked lists
* 
* The idea of the segregated fits technique with address-ordered explicit 
* linked lists is to separate all free memory blocks into different classes
* according to their sizes. When the new free block is requested we search
* through only one class of free blocks. This technique significantly reduce
* the searching time of the block. In this project we assume twenty classes,
* where the i-th class contains blocks of the size from 2^i to 2^(i+1) - 1
* (the last class contains all blocks of the size greater or equal to the 
* 2^20.
* 
* The technique was chosen as this approach is used in the standard library's
* malloc implementation.
*
* We also use reallocation tags to reduce the internal and external
* fragmentation (the idea of using reallocation tags was found in a reference
* but I cannot provide a link, as I did not save it). The idea is rather
* simple: before reallocating a block we check if we can just use the next
* one (if it is unallocated). If not we reallocate the block, provide some
* "buffer" space, and mark it is a reallocation block, so we know it may be
* used for this purpose.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*
 * Team identification
 */
team_t team = {
    /* Team name */
    "Sergey Shpak",
    /* First member's full name */
    "Sergey Shpak",
    "sergey.shpak@polytechnique.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* CONSTANTS AND MACROS DEFINITION */

#define WSIZE   4       /* Word size in bytes */
#define DSIZE   8       /* Double word size in bytes */
#define CHUNKSIZE       4096    /* Heap is extendede by this amount of bytes */
#define MINSIZE         16      /* Minimal block size to keep free */
#define ALLOCATE_BIT    0x1     /* Bit to indicate the block is allocated */
#define LISTS_COUNT   20      /* Number of free lists */
#define BUFFER  128      /* Reallocation buffer */

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* Pack size and status bit into one word */
#define PACK(size, status) ((size) | (status))

/* 
*    Getting allocation and tag bits of the block 
*    at which header points the p pointer 
*/
#define GET_ALLOC(p)    (GET(p) & 0x1)
#define GET_TAG(p)   (GET(p) & 0x2)

/* Read word (and allocation bits) at which points the p pointer */
#define GET(p)    (*(unsigned int *)(p))
// Preserve reallocation bit
#define PUT(p, val)       (*(unsigned int *)(p) = (val) | GET_TAG(p))
// Clear reallocation bit
#define PUT_NOTAG(p, val) (*(unsigned int *)(p) = (val))

/* Adjust the reallocation tag */
#define SET_TAG(p)   (*(unsigned int *)(p) = GET(p) | 0x2)
#define UNSET_TAG(p) (*(unsigned int *)(p) = GET(p) & ~0x2)

/* Store predecessor or successor pointer for free blocks */
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/* 
*   Read word (no allocation bits) at which points p.
*   Mostly used to get size of the block (p is a header or footer pointer)
*/
#define GET_CLEAN(p)  (GET(p) & ~0x7)

/* 
*   Block's header and footer pointers.
*   Here we use casting to char pointer because the length of
*   word (WSIZE) or dword (DSIZE) is given in bytes (pointer arithmetic thing)
*/
#define HPTR(ptr) ((char *)(ptr) - WSIZE)
#define FPTR(ptr) ((char *)(ptr) + GET_CLEAN(HPTR(ptr)) - DSIZE)

/* Address of next and previous blocks */
#define NEXT(ptr) ((char *)(ptr) + GET_CLEAN((char *)(ptr) - WSIZE))
#define PREV(ptr) ((char *)(ptr) - GET_CLEAN((char *)(ptr) - DSIZE))

/* Address of free block's predecessor and successor entries */
#define PREV_FREE_PTR(ptr) ((char *)(ptr))
#define NEXT_FREE_PTR(ptr) ((char *)(ptr) + WSIZE)

/* Address of free block's predecessor and successor on the segregated list */
#define PRED(ptr) (*(char **)(ptr))
#define SUCC(ptr) (*(char **)(NEXT_FREE_PTR(ptr)))

/* Check for alignment */
#define ALIGN(p) (((size_t)(p) + 7) & ~(0x7))

/* END OF CONSTANTS AND MACROS DEFINITION BLOCK */


/* GLOBAL VARIABLES */

static char *prologue_block;  // pointer to prologue block
static void *free_lists[LISTS_COUNT];  // array of pointers to free lists

/* END OF GLOBAL VARIABLES BLOCK */


/* FUNCTION PROTOTYPES */

static void *extend_heap(size_t size);
static void *coalesce(void *ptr);
static void place(void *ptr, size_t asize);
static void add_to_free_lists(void *ptr, size_t size);
static void remove_block_from_list(void *ptr);

/* END OF FUNCTION PROTOTYPES BLOCK */


/* 
*   Create heap: 
*       1) create prologue, epilogue blocks, empty heap;
*       2) extend empty heap by CHUNKSIZE bytes
*/
int mm_init()
{
  char *new_heap_ptr;  // Type is char ptr due to the pointer arithmetics
  int i;
  
  /* Inintialise free lists for all size classes */
  for (i = 0; i < LISTS_COUNT; i++) {
    free_lists[i] = NULL;
  }

  /* Allocate memory for the initial empty heap */
  new_heap_ptr = mem_sbrk(4 * WSIZE);
  if ((long)new_heap_ptr == -1)
    return -1;
  
  PUT_NOTAG(new_heap_ptr, 0);  // padding byte
  PUT_NOTAG(new_heap_ptr + WSIZE, PACK(DSIZE, 1));  // Prologue header
  PUT_NOTAG(new_heap_ptr + (2 * WSIZE), PACK(DSIZE, 1)); // Prologue footer
  PUT_NOTAG(new_heap_ptr + (3 * WSIZE), PACK(0, 1));     // Epilogue
  prologue_block = new_heap_ptr + 2 * WSIZE;
  
  /* Extend the empty heap */
  if (NULL == extend_heap(CHUNKSIZE)) {
    return -1;
  }
    
  return 0;
}

/*
*   mm_free - Free a block and coalesce it.
*/
void mm_free(void *ptr)
{
  size_t size = GET_CLEAN(HPTR(ptr)); 
  
  /* Remove reallocation tag from the next block header */
  UNSET_TAG(HPTR(NEXT(ptr)));
  
  /* SET header and footer block to "unallocated" status */
  PUT(HPTR(ptr), PACK(size, 0));
  PUT(FPTR(ptr), PACK(size, 0));
  
  /* Add the freed block to it's free list */
  add_to_free_lists(ptr, size);
  
  /* Coalesce the freed block */
  coalesce(ptr);
}

/* 
 * mm_malloc - Allocate a new block by placing it in a free block, extending
 *             heap if necessary. Blocks are padded with boundary tags and
 *             lengths are changed to conform with alignment.
 */
void *mm_malloc(size_t size)
{
  size_t asize;      /* Adjusted block size */
  size_t extendsize; /* Amount to extend heap if no fit */
  void *ptr = NULL;  /* Pointer */
  int list = 0;      /* List counter */
 
  /* Filter invalid block size */
  if (size == 0)
    return NULL;
  
  /* Adjust block size to include boundary tags and alignment requirements */
  if (size <= DSIZE) {
    asize = 2 * DSIZE;
  } else {
    asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  }
  
  /* Select a free block of sufficient size from segregated list */
  size = asize;
  while (list < LISTS_COUNT) {
    if ((list == LISTS_COUNT - 1) || ((size <= 1) && (free_lists[list] != NULL))) {
      ptr = free_lists[list];
      // Ignore blocks that are too small or marked with the reallocation bit
      while ((ptr != NULL)
        && ((asize > GET_CLEAN(HPTR(ptr))) || (GET_TAG(HPTR(ptr)))))
      {
        ptr = PRED(ptr);
      }
      if (ptr != NULL)
        break;
    }
    
    size >>= 1;
    list++;
  }
  
  /* Extend the heap if no free blocks of sufficient size are found */
  if (ptr == NULL) {
    extendsize = MAX(asize, CHUNKSIZE);
    if ((ptr = extend_heap(extendsize)) == NULL)
      return NULL;
  }
  
  /* Place the block */
  place(ptr, asize);
  
  /*
  // Check heap for consistency
  line_count++;
  if (CHECK && CHECK_MALLOC) {
    mm_check('a', ptr, checksize);
  }
  */
  
  /* Return pointer to newly allocated block */
  return ptr;
}



/*
 * mm_realloc - Reallocate a block in place, extending the heap if necessary.
 *              The new block is padded with a buffer to guarantee that the
 *              next reallocation can be done without extending the heap,
 *              assuming that the block is expanded by a constant number of bytes
 *              per reallocation.
 *
 *              If the buffer is not large enough for the next reallocation, 
 *              mark the next block with the reallocation tag. Free blocks
 *              marked with this tag cannot be used for allocation or
 *              coalescing. The tag is cleared when the marked block is
 *              consumed by reallocation, when the heap is extended, or when
 *              the reallocated block is freed.
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *new_ptr = ptr;    /* Pointer to be returned */
  size_t new_size = size; /* Size of new block */
  int remainder;          /* Adequacy of block sizes */
  int extendsize;         /* Size of heap extension */
  int block_buffer;       /* Size of block buffer */
    
    /* Filter invalid block size */
  if (size == 0)
    return NULL;
  
  /* Adjust block size to include boundary tag and alignment requirements */
  if (new_size <= DSIZE) {
    new_size = 2 * DSIZE;
  } else {
    new_size = DSIZE * ((new_size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  }
  
  /* Add overhead requirements to block size */
  new_size += BUFFER;
  
  /* Calculate block buffer */
  block_buffer = GET_CLEAN(HPTR(ptr)) - new_size;
  
  /* Allocate more space if overhead falls below the minimum */
  if (block_buffer < 0) {
    /* Check if next block is a free block or the epilogue block */
    if (!GET_ALLOC(HPTR(NEXT(ptr))) || !GET_CLEAN(HPTR(NEXT(ptr)))) {
      remainder = GET_CLEAN(HPTR(ptr)) + GET_CLEAN(HPTR(NEXT(ptr))) - new_size;
      if (remainder < 0) {
        extendsize = MAX(-remainder, CHUNKSIZE);
        if (extend_heap(extendsize) == NULL)
          return NULL;
        remainder += extendsize;
      }
      
      remove_block_from_list(NEXT(ptr));
      
      // Do not split block
      PUT_NOTAG(HPTR(ptr), PACK(new_size + remainder, 1)); /* Block header */
      PUT_NOTAG(FPTR(ptr), PACK(new_size + remainder, 1)); /* Block footer */
    } else {
      new_ptr = mm_malloc(new_size - DSIZE);
      //line_count--;
      memmove(new_ptr, ptr, MIN(size, new_size));
      mm_free(ptr);
      //line_count--;
    }
    block_buffer = GET_CLEAN(HPTR(new_ptr)) - new_size;
  }  

  /* Tag the next block if block overhead drops below twice the overhead */
  if (block_buffer < 2 * BUFFER)
    SET_TAG(HPTR(NEXT(new_ptr)));

  /*
  // Check heap for consistency
  line_count++;
  if (CHECK && CHECK_REALLOC) {
    mm_check('r', ptr, size);
  }
  */
  
  /* Return reallocated block */
  return new_ptr;
}

/*
 * extend_heap - Extend the heap with a system call. Insert the newly
 *               requested free block into the appropriate list.
 */
static void *extend_heap(size_t size)
{
  void *ptr;                   /* Pointer to newly allocated memory */
  size_t words = size / WSIZE; /* Size of extension in words */
  size_t asize;                /* Adjusted size */
  
  /* Allocate an even number of words to maintain alignment */
  asize = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
  
  /* Extend the heap */
  if ((long)(ptr = mem_sbrk(asize)) == -1)
    return NULL;
  
  /* Set headers and footer */
  PUT_NOTAG(HPTR(ptr), PACK(asize, 0));   /* Free block header */
  PUT_NOTAG(FPTR(ptr), PACK(asize, 0));   /* Free block footer */
  PUT_NOTAG(HPTR(NEXT(ptr)), PACK(0, 1)); /* Epilogue header */
  
  /* Insert new block into appropriate list */
  add_to_free_lists(ptr, asize);
  
  /* Coalesce if the previous block was free */
  return coalesce(ptr);
}

/*
 * add_to_free_lists - Insert a block pointer into a segregated list. Lists are
 *               segregated by byte size, with the n-th list spanning byte
 *               sizes 2^n to 2^(n+1)-1. Each individual list is sorted by
 *               pointer address in ascending order.
 */
static void add_to_free_lists(void *ptr, size_t size) {
  int list = 0;
  void *search_ptr = ptr;
  void *insert_ptr = NULL;
  
  /* Select segregated list */
  while ((list < LISTS_COUNT - 1) && (size > 1)) {
    size >>= 1;
    list++;
  }
  
  /* Select location on list to insert pointer while keeping list
     organized by byte size in ascending order. */
  search_ptr = free_lists[list];
  while ((search_ptr != NULL) && (size > GET_CLEAN(HPTR(search_ptr)))) {
    insert_ptr = search_ptr;
    search_ptr = PRED(search_ptr);
  }
  
  /* Set predecessor and successor */
  if (search_ptr != NULL) {
    if (insert_ptr != NULL) {
      SET_PTR(PREV_FREE_PTR(ptr), search_ptr); 
      SET_PTR(NEXT_FREE_PTR(search_ptr), ptr);
      SET_PTR(NEXT_FREE_PTR(ptr), insert_ptr);
      SET_PTR(PREV_FREE_PTR(insert_ptr), ptr);
    } else {
      SET_PTR(PREV_FREE_PTR(ptr), search_ptr); 
      SET_PTR(NEXT_FREE_PTR(search_ptr), ptr);
      SET_PTR(NEXT_FREE_PTR(ptr), NULL);
      
      /* Add block to appropriate list */
      free_lists[list] = ptr;

    }
  } else {
    if (insert_ptr != NULL) {
      SET_PTR(PREV_FREE_PTR(ptr), NULL);
      SET_PTR(NEXT_FREE_PTR(ptr), insert_ptr);
      SET_PTR(PREV_FREE_PTR(insert_ptr), ptr);
    } else {
      SET_PTR(PREV_FREE_PTR(ptr), NULL);
      SET_PTR(NEXT_FREE_PTR(ptr), NULL);
      
      /* Add block to appropriate list */
      free_lists[list] = ptr;
    }
  }

  return;
}

/*
 * remove_block_from_list: Remove a free block pointer from a segregated list. If
 *              necessary, adjust pointers in predecessor and successor blocks
 *              or reset the list head.
 */
static void remove_block_from_list(void *ptr) {
  int list = 0;
  size_t size = GET_CLEAN(HPTR(ptr));
  
  /* Select segregated list */
  while ((list < LISTS_COUNT - 1) && (size > 1)) {
    size >>= 1;
    list++;
  }
  
  if (PRED(ptr) != NULL) {
    if (SUCC(ptr) != NULL) {
      SET_PTR(NEXT_FREE_PTR(PRED(ptr)), SUCC(ptr));
      SET_PTR(PREV_FREE_PTR(SUCC(ptr)), PRED(ptr));
    } else {
      SET_PTR(NEXT_FREE_PTR(PRED(ptr)), NULL);
      free_lists[list] = PRED(ptr);
    }
  } else {
    if (SUCC(ptr) != NULL) {
      SET_PTR(PREV_FREE_PTR(SUCC(ptr)), NULL);
    } else {
      free_lists[list] = NULL;
    }
  }
  
  return;
}

/*
 * coalesce - Coalesce adjacent free blocks. Sort the new free block into the
 *            appropriate list.
 */
static void *coalesce(void *ptr)
{
  size_t prev_alloc = GET_ALLOC(HPTR(PREV(ptr)));
  size_t next_alloc = GET_ALLOC(HPTR(NEXT(ptr)));
  size_t size = GET_CLEAN(HPTR(ptr));
  
  /* Return if previous and next blocks are allocated */
  if (prev_alloc && next_alloc) {
    return ptr;
  }
  
  /* Do not coalesce with previous block if it is tagged */
  if (GET_TAG(HPTR(PREV(ptr))))
    prev_alloc = 1;
  
  /* Remove old block from list */
  remove_block_from_list(ptr);
  
  /* Detect free blocks and merge, if possible */
  if (prev_alloc && !next_alloc) {
    remove_block_from_list(NEXT(ptr));
    size += GET_CLEAN(HPTR(NEXT(ptr)));
    PUT(HPTR(ptr), PACK(size, 0));
    PUT(FPTR(ptr), PACK(size, 0));
  } else if (!prev_alloc && next_alloc) {
    remove_block_from_list(PREV(ptr));
    size += GET_CLEAN(HPTR(PREV(ptr)));
    PUT(FPTR(ptr), PACK(size, 0));
    PUT(HPTR(PREV(ptr)), PACK(size, 0));
    ptr = PREV(ptr);
  } else {
    remove_block_from_list(PREV(ptr));
    remove_block_from_list(NEXT(ptr));
    size += GET_CLEAN(HPTR(PREV(ptr))) + GET_CLEAN(HPTR(NEXT(ptr)));
    PUT(HPTR(PREV(ptr)), PACK(size, 0));
    PUT(FPTR(NEXT(ptr)), PACK(size, 0));
    ptr = PREV(ptr);
  }
  
  /* Adjust segregated linked lists */
  add_to_free_lists(ptr, size);
  
  return ptr;
}

/*
 * place - Set headers and footers for newly allocated blocks. Split blocks
 *         if enough space is remaining.
 */
static void place(void *ptr, size_t asize)
{
  size_t ptr_size = GET_CLEAN(HPTR(ptr));
  size_t remainder = ptr_size - asize;
  
  /* Remove block from list */
  remove_block_from_list(ptr);
  
  if (remainder >= MINSIZE) {
    /* Split block */
    PUT(HPTR(ptr), PACK(asize, 1)); /* Block header */
    PUT(FPTR(ptr), PACK(asize, 1)); /* Block footer */
    PUT_NOTAG(HPTR(NEXT(ptr)), PACK(remainder, 0)); /* Next header */
    PUT_NOTAG(FPTR(NEXT(ptr)), PACK(remainder, 0)); /* Next footer */  
    add_to_free_lists(NEXT(ptr), remainder);
  } else {
    /* Do not split block */
    PUT(HPTR(ptr), PACK(ptr_size, 1)); /* Block header */
    PUT(FPTR(ptr), PACK(ptr_size, 1)); /* Block footer */
  }
  return;
}
