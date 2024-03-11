

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mm.h"
#include <stdbool.h>



/*
a 0 2040
a 1 4010
a 2 48
a 3 4072
a 4 4072
a 5 4072
*/
int main() {
    mm_init();
    debug_header();
    void *p1 = mm_malloc(2040);
    
    void *p2 = mm_malloc(4010);
    void *p3 = mm_malloc(48);
    void *p4 = mm_malloc(4072);
    void *p5 = mm_malloc(4072);
    void *p6 = mm_malloc(4072);
    printf("1\n");
    debug_free();
    printf("2\n");
    mm_free(p1);
   
    mm_free(p2);
    mm_free(p3);
   
    mm_free(p4);
    mm_free(p5);
    mm_free(p6);
    debug_free();
    
    return 1;
}
