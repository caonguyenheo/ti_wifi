#ifndef __P2P_H__
#define __P2P_H__
#include <stdint.h>

//*****************************************************************************
//                 GLOBAL DEFINE -- Start
//*****************************************************************************
#define PL_PORT		50007
#define PS_PORT		50107
#define PR_PORT		50207

#define SERVER_PORT 9080 
//*****************************************************************************
//                 GLOBAL DEFINE -- End
//*****************************************************************************

//*****************************************************************************
//                  FUNCTION PROTOTYPES
//*****************************************************************************
int get_pl_socket(char *my_pl_ip, uint32_t *my_pl_port);
int new_pl_socket(char *my_pl_ip, uint32_t *my_pl_port);
int get_pr_socket(char *my_pr_ip, uint32_t *my_pr_port);
int new_pr_socket(char *my_pr_ip, uint32_t *my_pr_port);
int new_ps_socket(void);
void PS_open(int ps_socket);
void PR_open(int pr_socket);
#endif //  __P2P__
