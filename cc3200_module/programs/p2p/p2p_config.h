#ifndef __P2P_CONFIG_H__
#define __P2P_CONFIG_H__


#define MAX_KEY_SIZE       17
#define MAX_RN_SIZE        13

/*******************************************************************************
* @brief: Get MAC address and UID from device factory config
* @param: None
* @ret  : None
*/
void
config_init(void);

/*******************************************************************************
* @brief: Generate random alpha numberical case sensity char array (0-9, a-z, A-Z)
* @param[out]: output  - output buffef
* @param[in]:  len     - length of output
* @ret:        None 
*/
void
random_char(char *output,  int len);

/*******************************************************************************
* @brief: Convert char string to hex string 
* @param[in]:  input   - input buffer
* @param[out]: output  - output buffer
* @param[in]:  inp_len - length of input buffer
* @ret  :      0 if success, others for failure  
*/
int
char_to_hex(char *input, char *output, int inp_len);

/*******************************************************************************
* @brief: P2P init 
* @param: None
* @ret  : None
*/
int
p2p_init(int update_param);

// extern global variables
extern char p2p_key_char[];
extern char p2p_rn_char[];
extern char p2p_key_hex[];
extern char p2p_rn_char_hex[];

#endif   /* end of file */
