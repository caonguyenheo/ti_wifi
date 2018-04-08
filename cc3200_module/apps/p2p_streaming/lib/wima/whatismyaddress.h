#ifndef __WHATISMYADDRESS_H__
#define __WHATISMYADDRESS_H__

#include <stdint.h>
//#include <arpa/inet.h>
// #include <sys/time.h>


// return -1 if fail, 0 if success
// Set socket timeout to 50ms internally
int32_t wima_nonos(int socket, char* server_ip, int server_port, char* client_ip, int* client_port);

#endif
