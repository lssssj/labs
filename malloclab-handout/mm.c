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


#define SIZE_T_SIZE (ALIGN(sizeof(uint64_t)))

#define MIN_BLOCK_SIZE 24

#define HEADER_SIZE sizeof(uint64_t)

#define GET_BLOCK(b)       ((unsigned char*)b - HEADER_SIZE)
#define GET_BLOCK_SIZE(b)       (*(uint64_t*)(b))
#define SET_BLOCK_SIZE(b, s)    (*(uint64_t*)b = s)
#define FIND_NEXT_FREE(b)       ((unsigned char*)(*(uintptr_t*)(b + 8)))
#define FIND_PREV_FREE(b)       ((unsigned char*)(*(uintptr_t*)(b + 16)))

/* 
 * mm_init - initialize the malloc package.
 */
static unsigned char **head;
static unsigned char *fit;

static void write_free_block(unsigned char *b, uint64_t s, unsigned char *prev, unsigned char *next) {
   *(uint64_t*)b = s;
   *(uintptr_t*)(b + 8) = (uintptr_t)next;
   *(uintptr_t*)(b + 16) = (uintptr_t)prev;
}


static void insert_free_block(unsigned char *b, unsigned char *prev, unsigned char *next) {
    *(uintptr_t*)(b + 16) = (uintptr_t)prev;
    *(uintptr_t*)(b + 8) = (uintptr_t)next;
    if (prev != NULL) {
        *(uintptr_t*)(prev + 8) = (uintptr_t)b;
    }
    if (next != NULL) {
        *(uintptr_t*)(next + 16) = (uintptr_t)b;
    }
}

static void link_block(unsigned char *a, unsigned char *b) {
    *(uintptr_t*)(a + 8) = (uintptr_t)b;
    *(uintptr_t*)(b + 16) = (uintptr_t)a;  
}

static void merge(unsigned char *a, unsigned char *b, bool anext) {
    unsigned char *next;
    if (anext) {
        next = FIND_NEXT_FREE(a);
    } else {
        next = FIND_NEXT_FREE(b);
    }
    uint64_t asize = GET_BLOCK_SIZE(a), bsize = GET_BLOCK_SIZE(b);
    SET_BLOCK_SIZE(a, asize + bsize);
    *(uintptr_t*)(a + 8) = (uintptr_t)next;
    if (next != NULL) {
        *(uintptr_t*)(next + 16) = (uintptr_t)a;
    }
}

int mm_init(void)
{
    mem_init();
    if ((head = (unsigned char**)mem_sbrk(HEADER_SIZE)) == NULL) {
        fprintf(stderr, "mem_init_vm: malloc error\n");
	    exit(1);
    }
    *head = NULL;
    fit = NULL;
    return 0;
}

// 划分空闲块，其中因为规定每个块大小必须24字节，那么访问prev和next指针也就不会越界
// 所以必须得维护空闲链表中每个块的正确的prev和next
static unsigned char* split_block(unsigned char *b, size_t asize) {
    size_t bsize = GET_BLOCK_SIZE(b);
    unsigned char *prev = FIND_PREV_FREE(b);
    unsigned char *next = FIND_NEXT_FREE(b);
    if (bsize - asize >= MIN_BLOCK_SIZE) {
        SET_BLOCK_SIZE(b, asize);
        write_free_block(b + asize, bsize - asize, prev, next);
        if (*head == b) {   
            *head = b + asize;
            if (next != NULL) {
                 *(uintptr_t*)(next + 16) = (uintptr_t)(b + asize);
            }
        } else {
            insert_free_block(b + asize, prev, next);
        }
        fit = b + asize;
    } else {
        // 不划分block了，直接把改block返回
        if (next == NULL && prev == NULL) {
            *head = NULL;
        } else if (next == NULL) {
            *(uintptr_t*)(prev + 8) = NULL;
        } else if (prev == NULL) {
            *head = next;
            if (next != NULL) {
                *(uintptr_t*)(next + 16) = NULL;
            }
        } else {
            link_block(prev, next);
        }
        fit = next;
    }
    return b;
}

static unsigned char* first_fit(size_t asize) {
    unsigned char *block;
    for (block = *head; block != NULL; block = FIND_NEXT_FREE(block)) {
        if (GET_BLOCK_SIZE(block) >= asize) {
            return split_block(block, asize);
        }
    }
    return NULL;
}

static unsigned char* next_fit(size_t asize) {
    unsigned char *block;
    for (block = fit; block != NULL; block = FIND_NEXT_FREE(block)) {
        if (GET_BLOCK_SIZE(block) >= asize) {
            return split_block(block, asize);
        }
    }
    for (block = *head; block != NULL; block = FIND_NEXT_FREE(block)) {
        if (GET_BLOCK_SIZE(block) >= asize) {
            return split_block(block, asize);
        }
    }
    return NULL;
}

static unsigned char* extend_heap(size_t sz) {
    unsigned char *p;
    if ((p = (unsigned char*)mem_sbrk(sz)) == NULL) {
        fprintf(stderr, "extend_heap: malloc error\n");
	    exit(1);
    }
    SET_BLOCK_SIZE(p, sz);
    return p;
}

// 按地址排序插入free的block
// 1。 插入的block在所有的free_block前面
static void coalesce_block(unsigned char *b) {
    unsigned char *prev = *head, *next;
    // 没有free block
    uint64_t sz = GET_BLOCK_SIZE(b);
    if (prev == NULL) {
       *head = b;
       write_free_block(b, sz, NULL, NULL);
       return;
    }
    do {
        next = FIND_NEXT_FREE(prev);
        if (next > b || next == NULL) {
            break;
        }
        prev = next;
    } while (1);
    uint64_t prev_sz = GET_BLOCK_SIZE(prev);
    if (prev + prev_sz == b) {
        merge(prev, b, true);
        b = prev;
    } else {
        insert_free_block(b, prev, next);
    }
    fit = prev;
    if (next == NULL) {
        *(uintptr_t*)(b + 8) = (uintptr_t)next;
    } else if (b + GET_BLOCK_SIZE(b) == next) {
        merge(b, next, false);
    } else {
        link_block(b, next);
    }
    fit = b;
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t newsize = ALIGN(size + SIZE_T_SIZE);
    if (newsize < MIN_BLOCK_SIZE) {
        newsize = MIN_BLOCK_SIZE;
    }
    unsigned char *block = first_fit(newsize);
    if (block == NULL) {
        block = extend_heap(newsize);
    }
    if (block == NULL) {
        fprintf(stderr, "memory out\n");
	    return NULL;
    }
    //printf("need %ld, alloced %ld, ptr = %p\n", size, newsize, block);
    return (void*)(block + HEADER_SIZE);
}
/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if (ptr == NULL) {
        return;
    }
    coalesce_block(GET_BLOCK(ptr));
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

static unsigned char* next_(unsigned char* b) {
    uintptr_t u = *(uintptr_t*)(b + 8);
    return (unsigned char*)u;
}

static unsigned char* prev_(unsigned char* b) {
    uintptr_t u = *(uintptr_t*)(b + 16);
    return (unsigned char*)u;
}

bool check_free(char *msg) {
    unsigned char *b = *head;
    bool error = false;
    for (b; b != NULL; b = next_(b)) {
        uint64_t header = *(uint64_t*)b;
        if (next_(b) != NULL) {
            unsigned char* l = next_(b);
            if (prev_(l) != b) {
                printf("in %p 's next %p' prev  %p is not equal self\n", b, l, prev_(l));
                error = true;
            }
        } 
    }
    if (error) {
        b = *head;
        printf("%s\n", msg);
        printf("start %p\n", b);
        for (b; b != NULL; b = next_(b)) {
            printf("addr = %p, next = %p, prev = %p, sz = %ld\n", b, FIND_NEXT_FREE(b), FIND_PREV_FREE(b), GET_BLOCK_SIZE(b));
        }
        return true;
    }
    return false;
}

void debug_free() {
    static int cnt = 0;
    // printf("         debug freee     \n");
    printf("============%d===========\n", cnt++);
    unsigned char *b = *head;
    for (b; b != NULL; b = next_(b)) {
        uint64_t header = *(uint64_t*)b;
        unsigned char* e = b + header;
        printf("start_addr = %p, end_addr = %p\n", b, e);
        printf("next = %p, prev = %p\n", next_(b), prev_(b));
        printf("size = %ld\n", GET_BLOCK_SIZE(b));
        if (next_(b) != NULL) {
            unsigned char* l = next_(b);
            if (prev_(l) != b) {
                printf("in %p 's next %p' prev  %p is not equal self\n", b, l, prev_(l));
            }
            printf("-----------\n");
        } 
    }
}
















