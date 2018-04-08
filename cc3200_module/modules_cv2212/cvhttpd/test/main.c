#include <stdio.h>
#include "cvhttpd.h"

#define PORT    3333

int main(int argc, char *argv[])
{
    printf("starting a static http server at port %d\n", PORT);

    int rc = httpd_start(PORT);

    if (rc < 0) {
        printf("Fail to start cvhttpd %d\n", rc);
    }
    return 0;
}
