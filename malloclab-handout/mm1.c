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

/* 
 * mm_init - initialize the malloc package.
 */
static unsigned char *heap_start, *heap_end; 
static unsigned char **root, **tail;
static unsigned char *last_find;

static void write_free_block(unsigned char *b, uint64_t s, unsigned char *prev, unsigned char *next) {
   *(uint64_t*)b = s;
   *(uintptr_t*)(b + 8) = (uintptr_t)next;
   *(uintptr_t*)(b + 16) = (uintptr_t)prev;
   if (next != heap_end) {
    *(uintptr_t*)(next + 16) = (uintptr_t)b;
   }
}


#define set_block_size(b, s) (*(uint64_t*)b = s)
#define get_block(b)      ((unsigned char*)b - HEADER_SIZE)
#define get_block_size(b) ((b == heap_end) ? 0 : (*(uint64_t*)(b)))
#define find_next_free(b) ((unsigned char*)(*(uintptr_t*)(b + 8)))
#define find_prev_free(b) ((unsigned char*)(*(uintptr_t*)(b + 16)))
#define link_block(a, b) do { \
    *(uintptr_t*)((a) + 8) = (uintptr_t)(b); \
    *(uintptr_t*)((b) + 16) = (uintptr_t)(a); \
} while (0)

static void insert_free_block(unsigned char *b, unsigned char *prev, unsigned char *next) {
    *(uintptr_t*)(prev + 8) = (uintptr_t)b;
    *(uintptr_t*)(b + 16) = (uintptr_t)prev;
    *(uintptr_t*)(b + 8) = (uintptr_t)next;
    if (next != heap_end) {
        *(uintptr_t*)(next + 16) = (uintptr_t)b;
    }
}

// static void link_block(unsigned char *a, unsigned char *b) {
//     *(uintptr_t*)(a + 8) = (uintptr_t)b;
//     *(uintptr_t*)(b + 16) = (uintptr_t)a;  
// }

static void merge(unsigned char *a, unsigned char *b, bool anext) {
    unsigned char *next;
    if (anext) {
        next = find_next_free(a);
    } else {
        next = find_next_free(b);
    }
    uint64_t asize = get_block_size(a), bsize = get_block_size(b);
    set_block_size(a, asize + bsize);
    *(uintptr_t*)(a + 8) = (uintptr_t)next;
    if (next != heap_end) {
        *(uintptr_t*)(next + 16) = (uintptr_t)a;
    }
}


int mm_init(void)
{
    mem_init();
    if ((heap_start = (unsigned char*)mem_sbrk(2 * HEADER_SIZE)) == NULL) {
        fprintf(stderr, "mem_init_vm: malloc error\n");
	    exit(1);
    }
    root = (unsigned char**)heap_start;
    tail = (unsigned char**)(heap_start + 8);
    heap_start = heap_start + 16;
    heap_end = heap_start;
    *root = heap_end;
    *tail = heap_end;
    last_find = heap_end;
    return 0;
}

// 划分空闲块，其中因为规定每个块大小必须24字节，那么访问prev和next指针也就不会越界
// 所以必须得维护空闲链表中每个块的正确的prev和next
static unsigned char* split_block(unsigned char *b, size_t asize) {
    size_t bsize = get_block_size(b);
    unsigned char *prev = find_prev_free(b);
    unsigned char *next = find_next_free(b);
    if (bsize - asize >= MIN_BLOCK_SIZE) {
        set_block_size(b, asize);
        write_free_block(b + asize, bsize - asize, prev, next);
        if (*tail == b) {
            *tail = b + asize;
        }
        if (*root == b) {   
            *root = b + asize;
        } else {
            insert_free_block(b + asize, prev, next);
        }
        last_find = b + asize;
    } else {
        // 不划分block了，直接把改block返回
        if (*tail == b) {
            if (next == heap_end && prev != (unsigned char*)root) {
                *tail = prev;
            } else {
                *tail = next;
            }
        }
        if (*root == b) {
            *root = next;
            //  把下一个free block的prev设为root
            if (next != heap_end) {
                *(uintptr_t*)(next + 16) = (uintptr_t)root;
            }
        } else {
            link_block(prev, next);
        }
        last_find = next;
    }
    return b;
}

static unsigned char* first_fit(size_t asize) {
    unsigned char *block;
    for (block = *root; block != heap_end; block = find_next_free(block)) {
        if (get_block_size(block) >= asize) {
            return split_block(block, asize);
        }
    }
    return NULL;
}

static unsigned char* next_fit(size_t asize) {
    unsigned char *block;
    for (block = last_find; block != heap_end; block = find_next_free(block)) {
        if (get_block_size(block) >= asize) {
            return split_block(block, asize);
        }
    }
    for (block = *root; block != heap_end; block = find_next_free(block)) {
        if (get_block_size(block) >= asize) {
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
    if (last_find == heap_end) {
        last_find = p + sz;
    }
    if (*root == heap_end) {
        *root = p + sz;
    }
    if (*tail == heap_end) {
        *tail = p + sz;
    }
    heap_end = p + sz;
    if (*tail != heap_end) {
        unsigned char *last_block = *tail;
        *(uintptr_t*)(last_block + 8) = (uintptr_t)heap_end;
    }
    set_block_size(p, sz);
    return p;
}

// 按地址排序插入free的block
// 1。 插入的block在所有的free_block前面
static void coalesce_block(unsigned char *b) {
    unsigned char *prev = *root, *next;
    if (b < prev) {
        *root = b;
        *(uintptr_t*)(b + 16) = (uintptr_t)root;
        // 没有free block
        if (prev == heap_end) {
            *tail = b;
            *(uintptr_t*)(b + 8) = (uintptr_t)heap_end;
        } else if (b + get_block_size(b) == prev) {  // 释放的block刚好可以和第一个 block merge
            // 刚好只有一个block
            if (*tail == prev) {
                *tail = b;
            }
            merge(b, prev, false);
        } else {
            link_block(b, prev);
        }
        last_find = b;
        return;
    }
    do {
        next = find_next_free(prev);
        if (next > b) {
            break;
        }
        prev = next;
    } while (1);
    if (prev + get_block_size(prev) == b) {
        merge(prev, b, true);
        b = prev;
    } else {
        if (*tail == prev) {
            *tail = b;
        }
        insert_free_block(b, prev, next);
    }
    last_find = prev;
    if (next == heap_end) {
        *(uintptr_t*)(b + 8) = (uintptr_t)next;
    } else if (b + get_block_size(b) == next) {
        if (*tail == next) {
            *tail = b;
        }
        merge(b, next, false);
    } else {
        link_block(b, next);
    }
    last_find = b;
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
    unsigned char *block = next_fit(newsize);
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

static unsigned char* next_(unsigned char* b) {
    uintptr_t u = *(uintptr_t*)(b + 8);
    return (unsigned char*)u;
}

static unsigned char* prev_(unsigned char* b) {
    uintptr_t u = *(uintptr_t*)(b + 16);
    return (unsigned char*)u;
}

void debug_free() {
    static int cnt = 0;
    // printf("         debug freee     \n");
    printf("============%d===========\n", cnt++);
    unsigned char *b = *root;
    for (b; b != NULL && b != heap_end; b = next_(b)) {
        uint64_t header = *(uint64_t*)b;
        unsigned char* e = b + header;
        printf("start_addr = %p, end_addr = %p\n", b, e);
        printf("next = %p, prev = %p heap_end = %p\n", next_(b), prev_(b), heap_end);
        printf("size = %ld\n", get_block_size(b));
        if (next_(b) != heap_end) {
            unsigned char* l = next_(b);
            if (prev_(l) != b) {
                printf("in %p 's next %p' prev  %p is not equal self\n", b, l, prev_(l));
            }
            printf("-----------\n");
        } else {
            if (*tail != b) {
                printf("error *tail = %p, b = %p\n", *tail, b);
            }
        }
    }
}

void debug_header() {
    printf("root = %p, heap_start = %p, heap_end %p\n", root, heap_start, heap_end);
}














