#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "whatismyaddress.h"


#define SERVER_PORT 9080 
extern char SERVER_URL[];
int main(void)
{
    int client_sock;
    char server_url[32] = {0}, client_ip[32] = {0};
    int  serverPort = SERVER_PORT, client_port=0;

    if ((client_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
        return -1;
    }
    
    strcpy(server_url, SERVER_URL);

    // Note, socket timeout is set to 50ms internally
    if (wima_nonos(client_sock, server_url, serverPort, client_ip, &client_port) < 0) {
        perror("cannot query for my public address\n");
        return -1;
    }

    printf("ip:%s, port: %d\n", client_ip, client_port);
    
    return 0;
}
