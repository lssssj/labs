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
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>

#include "config.h"
#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(2 * sizeof(uint64_t)))

#define MIN_BLOCK_SIZE 32

#define HEADER_SIZE sizeof(uint64_t)

/* 
 * mm_init - initialize the malloc package.
 */
static void write_header(unsigned char *b, size_t s, bool allocated) {
    *(uint64_t*)b = s | allocated;
}

static void write_footer(unsigned char *b, size_t s, bool allocated) {
    *(uint64_t*)(b + s - HEADER_SIZE ) = s | allocated;
}

static void write_meta(unsigned char *b, size_t s, bool allocated) {
    write_header(b, s, allocated);
    write_footer(b, s, allocated);
}

unsigned char *heap_start, *heap_end; 
unsigned char **root;

static void add_free_block(unsigned char *b);
int mm_init(void)
{
    mem_init();
    if ((heap_start = (unsigned char*)mem_sbrk(MAX_HEAP)) == NULL) {
        fprintf(stderr, "mem_init_vm: malloc error\n");
	    exit(1);
    }
    root = (unsigned char**)heap_start;
    printf("root = %p, hs = %p\n", root, heap_start);
    heap_start = heap_start + 16;
    printf("root = %p, hs = %p\n", root, heap_start);
    heap_end = heap_start + MAX_HEAP;
    write_meta(heap_start, MAX_HEAP-16, false);
    add_free_block(heap_start);
    return 0;
}

static size_t block_alocated(unsigned char *b) {
    uint64_t header = *(uint64_t*)b;
    return header & 0x1;
}

static size_t get_block_size(unsigned char *b) {
    if (b == NULL) {
        return 0;
    }
    if (b == heap_end) {
        return 0;
    }
    uint64_t header = *(uint64_t*)b;
    return header & ~0x1L;
}

static unsigned char* get_block(void *bp) {
    return (unsigned char*)bp - HEADER_SIZE;
}

static unsigned char* find_next(unsigned char* b) {
    return b + get_block_size(b);
}

static unsigned char* find_next_free(unsigned char* b) {
    uintptr_t u = *(uintptr_t*)(b + 8);
    return (unsigned char*)u;
}

static unsigned char* find_prev_free(unsigned char* b) {
    uintptr_t u = *(uintptr_t*)(b + 16);
    return (unsigned char*)u;
}

static void add_free_block(unsigned char *b) {
    write_meta(b, get_block_size(b), false);
    if (*root == NULL) {
        *root = b;
        *(root + 8) = NULL;
        *(root + 16) = NULL;
        return;
    }
    unsigned char *p = *root, *n = find_next_free(p); 
    for (; p != heap_end; p = n, n = find_next_free(p)) {
        if (p < b && b < n) {
            *(uintptr_t*)(p + 8) = (uintptr_t) b;
            *(uintptr_t*)(b + 8) = (uintptr_t) n;
            *(uintptr_t*)(b + 16) = (uintptr_t) p;
            if (n != heap_end) {
                *(uintptr_t*)(n + 16) = (uintptr_t) b;
            }
            return;
        } 
    }
}

static unsigned char* split_block(unsigned char* b, size_t asize) {
    size_t bsize = get_block_size(b);
    if (bsize - asize >= MIN_BLOCK_SIZE) {
        if (b == *root) {
            *root = find_next_free(*root);
        }
        write_meta(b, asize, true);
        unsigned char *new_block = find_next(b);
        write_meta(new_block, bsize - asize, false);
        add_free_block(new_block);
    } else {
        write_meta(b, bsize, true);
    }
    return b;
}

static unsigned char* find_fit(size_t asize) {
    unsigned char *block;
    for (block = *root; block != heap_end; block = find_next_free(block)) {
        if (!block_alocated(block) && get_block_size(block) >= asize) {
            return split_block(block, asize);
        }
    }
    return NULL;
}

static uint64_t find_prev_footer(unsigned char *b) {
    return *(uint64_t*)(b - HEADER_SIZE);
}

static unsigned char* find_prev(unsigned char *block) {
    if (block == heap_start) {
        return NULL;
    }
    uint64_t footer = find_prev_footer(block);
    int sz = footer & ~0x1;
    return block - sz;
}

static void coalesce_block(unsigned char *b) {
    unsigned char *p = find_prev(b);
    size_t asize = get_block_size(b);   
    if (p != NULL && !block_alocated(p)) {
        unsigned char *n = find_next(b);
        size_t psize = get_block_size(p);
        if (n != heap_end && !block_alocated(n)) {
            size_t nsize = get_block_size(n);
            write_meta(p, asize + psize + nsize, false);
        } else {
            write_meta(p, asize + psize, false);
        }
    } else {
        unsigned char *n = find_next(b);
        if (n != heap_end && !block_alocated(n)) {
            size_t nsize = get_block_size(n);
            write_meta(b, asize + nsize, false);
        } else {
            write_meta(b, asize, false);
        }
    }
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    unsigned char *block = find_fit(newsize);
    if (block == NULL) {
        return NULL;
    }
    return (void*)(block + HEADER_SIZE);
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    unsigned char *block = get_block(ptr);
    coalesce_block(block);
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


void debug_free() {
    static int cnt = 0;
    printf("         debug freee     \n");
    printf("============%d===========\n", cnt++);
    unsigned char *b = *root;
    for (b; b != heap_end && b != NULL; b = find_next_free(b)) {
        uint64_t header = *(uint64_t*)b;
        unsigned char* e1 = b + (header & ~0x1);
        printf("start_addr = %p, content_addr = %p, end_addr = %p\n", b, b + HEADER_SIZE, e1);
        unsigned char* e = b + (header & ~0x1) - HEADER_SIZE;
        uint64_t footer = *(uint64_t*)e;
        printf("header = %lld, footer= %lld\n", header, footer);
        printf("size = %d, alloced=%d\n", get_block_size(b), block_alocated(b));
        if (find_next_free(b) != heap_end && find_next_free(b) != NULL) {
            printf("-----------\n");
        }
    }
}

void debug2() {
    printf("%p %p\n", heap_start, heap_end);
    unsigned char *p = find_next_free(*root);
    printf("%p %p\n", p, heap_end);
}














