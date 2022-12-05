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
    "team7",
    /* First member's full name */
    "Hyeji Yeom",
    /* First member's email address */
    "kristi8041@gmail.com",
    /* Second member's full name (leave blank if none) */
    "Yongsik Im",
    /* Second member's email address (leave blank if none) */
    "hamsik2rang@gmail.com"
};

// #define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/**************************constants & useful macros***************************/
/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12) 

#define MAX(x, y) ((x) > (y)? (x) : (y))
/* pack a size and allocation bit */
#define PACK(size, alloc) ((size)|(alloc))

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define FIRST_FIT
/*************************************************************************/

/**********************global variable***************************/
static char *heap_listp; // points to prologue block footer
/***************************************************************/

/* Coalesce adjacent block under several conditions */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    /* Case 1*/
    if (prev_alloc && next_alloc) return bp;
    /* Case 2 */
    else if (prev_alloc && !next_alloc){
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    /* Case 3 */
    else if (!prev_alloc && next_alloc){
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    /* Case 4 */
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

/* Extend the heap */
static void *extend_heap(size_t size) // size bytes
{
    char *bp;
    size_t asize = ALIGN(size);

    if ((long)(bp = mem_sbrk(asize)) == -1) return NULL;

    PUT(HDRP(bp), PACK(asize, 0)); 
    PUT(FTRP(bp), PACK(asize, 0)); 
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //epilogue block

    return coalesce(bp);
}

/* 
 * mm_init - initialize the malloc package.
   returns 0 on success, -1 on error
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) return -1;

    PUT(heap_listp, 0); // padding 
    PUT(heap_listp+(1*WSIZE), PACK(DSIZE, 1)); // prologue header
    PUT(heap_listp+(2*WSIZE), PACK(DSIZE, 1)); // prologue footer
    PUT(heap_listp+(3*WSIZE), PACK(0, 1)); // epilogue header

    heap_listp += (2*WSIZE); 

    if (extend_heap(CHUNKSIZE) == NULL) return -1;

    return 0;
}

/* Search the suitable free list in the heap */
static void *find_fit(size_t asize)
{
    /*** First Fit ***/
    void *bp;

    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
       if ((!GET_ALLOC(HDRP(bp))) && (asize <= GET_SIZE(HDRP(bp)))) return bp;
    }
    return NULL;

    /*** Next Fit ***/

    /*** Best Fit ***/
}

/* Allocate a new block */
static void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));

    // if (previous block size - new size) is greater than 16 bytes, divide block
    if ((csize - asize)>= 2*DSIZE) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK((csize-asize), 0));
        PUT(FTRP(bp), PACK((csize-asize), 0));
    }

    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 *     if alloc size is 0 or there's no more space to extend, it returns NULL
 */
void *mm_malloc(size_t size) // size byte
{
    size_t asize = ALIGN(size) + DSIZE;
    size_t extendsize;
    char *bp;

    if (size == 0) return NULL;

    // Search the free list for a fit
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    // No fit found
    extendsize = MAX(asize, CHUNKSIZE);

    if ((bp = extend_heap(extendsize)) == NULL) return NULL;

    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 * returns a pointer to an allocated region
 */
void *mm_realloc(void *ptr, size_t size) // size bytes
{
    // if ptr is NULL the call is equivalent to mm_malloc(size)
    if (ptr == NULL) mm_malloc(size);
    // if size is 0 the call is equivalent to mm_free(ptr)
    if (size == 0) mm_free(ptr);

    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    size = ALIGN(size);

    newptr = mm_malloc(size);
    if (newptr == NULL) return NULL;

    copySize = GET_SIZE(HDRP(oldptr)) - DSIZE; // subtract overhead size
   
    if (size < copySize) copySize = size;

    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);

    return newptr;
}














