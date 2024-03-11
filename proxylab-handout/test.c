#include <arpa/inet.h>


#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netdb.h>

int main() {
    char ch[299];
    char *uri = "www.cmu.cu.edu";
    strcpy(ch, "hh");
    // strcpy(ch, "yy");
    
    // strcat(ch, uri);
    // strcat(ch, "\n");
    // strcat(ch, "test\n");
    // strcat(ch, "hh:hhh\n");
    // ch[10] = '\0';
    printf("%s %d\n", ch, strlen(ch));
    
    return 0;
}
