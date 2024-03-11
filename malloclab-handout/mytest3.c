#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
int main()
{
    dup2(2, 10);
    const char* ch = "hhhh\n";
    const char* p = "2222\n";

    write(10, ch, sizeof(ch));
    write(2, p, sizeof(p));
    
    exit(0);
}