


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include "mm.h"
#include "memlib.h"

/*
    a 0 2040
    a 1 2040
    f 1
    a 2 48
    a 3 4072
    f 3
    a 4 4072
    f 0
    f 2
    a 5 4072
    f 4
    f 5
 1000 1*2^12 4 * 1024

  ===========6===========
start_addr = 0xf41bd018, end_addr = 0xf55bd010
next = 0xf55bd010, prev = (nil)
*/

void func() {
     void*p0=mm_malloc(2040);
    void*p1=mm_malloc(2040);
   // no
    mm_free(p1);
    // no
    void *p2 = mm_malloc(48);
    void *p3 = mm_malloc(4072);
    mm_free(p3);
    void *p4 = mm_malloc(4072);
    mm_free(p0);
    mm_free(p2);
    void *p5 = mm_malloc(4072);
    mm_free(p4);
    mm_free(p5);
    debug_free();
}

int main() {
    mm_init();
    debug_header();
   func();
   func();
    return 1;
}