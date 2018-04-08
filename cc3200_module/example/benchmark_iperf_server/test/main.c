#include <stdio.h>
#include "cvhttpd.h"

#define PORT    3333
int main(int argc, char *argv[])
{
    if (httpd_start(PORT) <0) {
        printf("Fail to start cvhttpd\n");
    }
    return 0;
}