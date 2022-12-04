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
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

 /*********************************************************
  * NOTE TO STUDENTS: Before you do anything else, please
  * provide your team information in the following struct.
  ********************************************************/
team_t team = {
    /* Team name */
    "Team 7",
    /* First member's full name */
    "Im Yongsik",
    /* First member's email address */
    "Hamsik2rang",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

typedef uint32_t WORD;
typedef uint64_t DWORD;

/* single word (4) or double word (8) alignment */
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define ALIGNMENT 8
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(WORD*)(p))
#define PUT(p, value) (*(WORD*)(p) = (value))

#define GET_SIZE_FROM_HEADER(p) (GET(p) & ~(ALIGNMENT-1))
#define GET_ALLOC_FROM_HEADER(p) (GET(p) & 0x1)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define HEADER_PTR(bp) ((char*)(bp) - WSIZE)
#define FOOTER_PTR(bp) ((char*)(bp) + GET_SIZE_FROM_HEADER(HEADER_PTR(bp)) - DSIZE)

#define NEXT_BLOCK_PTR(bp) ((char*)(bp) + GET_SIZE_FROM_HEADER(((char*)(bp) - WSIZE)))
#define PREV_BLOCK_PTR(bp) ((char*)(bp) - GET_SIZE_FROM_HEADER(((char*)(bp) - DSIZE)))

// function declarations
static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* find_fit(size_t size);
static void place(void* bp, size_t size);

// static variables
static char* heap_list_ptr = NULL;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_list_ptr = mem_sbrk(4 * WSIZE)) == (void*)-1)
    {
        return -1;
    }
    PUT(heap_list_ptr, 0);
    PUT(heap_list_ptr + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_list_ptr + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_list_ptr + (3 * WSIZE), PACK(0, 1));
    heap_list_ptr += (2 * WSIZE);
    if (!extend_heap(CHUNKSIZE / WSIZE))
    {
        return -1;
    }
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size)
{
    char* bp = NULL;
    if (size == 0)
    {
        return NULL;
    }

    if (size <= DSIZE)
    {
        size = 2 * DSIZE;
    }
    else
    {
        size = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    }

    if ((bp = find_fit(size)) != NULL)
    {
        place(bp, size);
        return bp;
    }

    size_t extend_size = MAX(size, CHUNKSIZE);
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL)
    {
        return NULL;
    }
    place(bp, size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void* ptr)
{
    size_t size = GET_SIZE_FROM_HEADER(HEADER_PTR(ptr));

    PUT(HEADER_PTR(ptr), PACK(size, 0));
    PUT(FOOTER_PTR(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
 // void* mm_realloc(void* ptr, size_t size)
 // {
 //     void* old_ptr = ptr;
 //     void* new_ptr;
 //     size_t copy_size;

 //     new_ptr = mm_malloc(size);
 //     if (new_ptr == NULL)
 //     {
 //         return NULL;
 //     }

 //     copy_size = *(size_t*)((char*)old_ptr - SIZE_T_SIZE);
 //     if (size < copy_size)
 //     {
 //         copy_size = size;
 //     }

 //     memcpy(new_ptr, old_ptr, copy_size);
 //     mm_free(old_ptr);
 //     return new_ptr;
 // }

void* mm_realloc(void* ptr, size_t size)
{
    void* old_ptr = ptr;
    void* new_ptr = NULL;
    size_t copy_size = 0;

    new_ptr = mm_malloc(size);
    if (!new_ptr)
    {
        return NULL;
    }

    copy_size = (size_t)GET_SIZE_FROM_HEADER(HEADER_PTR(old_ptr));
    if (size < copy_size)
    {
        copy_size = size;
    }
    memcpy(new_ptr, old_ptr, copy_size);
    mm_free(old_ptr);

    return new_ptr;
}

static void* extend_heap(size_t words)
{
    char* bp = NULL;
    size_t size = 0;

    size = ((words % 2) ? (words + 1) : words) * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
    {
        return NULL;
    }

    PUT(HEADER_PTR(bp), PACK(size, 0));
    PUT(FOOTER_PTR(bp), PACK(size, 0));
    PUT(HEADER_PTR(NEXT_BLOCK_PTR(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void* coalesce(void* bp)
{
    size_t prev_alloc = GET_ALLOC_FROM_HEADER(FOOTER_PTR(PREV_BLOCK_PTR(bp)));
    size_t next_alloc = GET_ALLOC_FROM_HEADER(HEADER_PTR(NEXT_BLOCK_PTR(bp)));
    size_t size = GET_SIZE_FROM_HEADER(HEADER_PTR(bp));

    if (prev_alloc && next_alloc)
    {
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE_FROM_HEADER(HEADER_PTR(NEXT_BLOCK_PTR(bp)));
        PUT(HEADER_PTR(bp), PACK(size, 0));
        PUT(FOOTER_PTR(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE_FROM_HEADER(HEADER_PTR(PREV_BLOCK_PTR(bp)));
        PUT(FOOTER_PTR(bp), PACK(size, 0));
        PUT(HEADER_PTR(PREV_BLOCK_PTR(bp)), PACK(size, 0));
        bp = PREV_BLOCK_PTR(bp);
    }
    else
    {
        size += GET_SIZE_FROM_HEADER(HEADER_PTR(PREV_BLOCK_PTR(bp))) +
            GET_SIZE_FROM_HEADER(FOOTER_PTR(NEXT_BLOCK_PTR(bp)));
        PUT(HEADER_PTR(PREV_BLOCK_PTR(bp)), PACK(size, 0));
        PUT(FOOTER_PTR(NEXT_BLOCK_PTR(bp)), PACK(size, 0));
        bp = PREV_BLOCK_PTR(bp);
    }

    return bp;
}

static void* find_fit(size_t size)
{
    char* cur = heap_list_ptr;

    while (GET_SIZE_FROM_HEADER(HEADER_PTR(cur)))
    {
        WORD* cur_header = (WORD*)HEADER_PTR(cur);

        if (GET_ALLOC_FROM_HEADER(cur_header) ||
            GET_SIZE_FROM_HEADER(cur_header) < size)
        {
            cur = NEXT_BLOCK_PTR(cur);
            continue;
        }

        return cur;
    }

    return NULL;
}

static void place(void* bp, size_t size)
{
    size_t block_size = GET_SIZE_FROM_HEADER(HEADER_PTR(bp));
    if (block_size > size)
    {
        // set header/footer for new allocated block
        PUT(HEADER_PTR(bp), PACK(size, 1));
        PUT(FOOTER_PTR(bp), PACK(size, 1));

        bp = NEXT_BLOCK_PTR(bp);
        PUT(HEADER_PTR(bp), PACK(block_size - size, 0));
        PUT(FOOTER_PTR(bp), PACK(block_size - size, 0));
    }
    else
    {
        PUT(HEADER_PTR(bp), PACK(size, 1));
        PUT(FOOTER_PTR(bp), PACK(size, 1));
    }
}