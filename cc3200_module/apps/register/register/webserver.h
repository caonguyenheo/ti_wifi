#ifndef __WEBSERVER_H_
#define __WEBSERVER_H_

/*******************************************************************************
* @brief Function to parse get request url
*
* @param[int]  reqBuf:    request url
* @param[int]  reqBufLen: request url len
* @param[out]  *response:  http respone to client 
* @param[out]  *fileResp: flag to respone as a xml file
* @return:   
*/
int HandleCommand(char *reqBuf, int reqBufLen, char *response, int *fileResp);

/*******************************************************************************
* Description.: Send error messages and headers.
* Input Value.: * iSocketFd.....: is the filedescriptor to send the strMessage to
*              * which..: HTTP error code, most popular is 404
*              * strMessage: append this string to the displayed response
* Return Value: -
*/
void SendResponse(
                  UINT16    uConnection, 
                  int       iRetCode, 
                  char     *strMessage,
                  int      fileResponse
                    );

extern char router_list[];

#endif
