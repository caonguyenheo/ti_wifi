#include "tcpserver.h"

#define BACKLOG     2

int32_t httpd_start(uint16_t usPort)
{
    // int             iCounter;
    // short           sTestBufLen;
    // sockaddr_in  sAddr;
    int             iStatus;
    int             iNewSockID;
    int             iAddrSize;
    int             iSockID;
    fd_set  master;    // master file descriptor list
    fd_set  read_fds;  // temp file descriptor list for reading
    fd_set  write_fds; // temp file descriptor list for writing
    int     fdmax;        // maximum file descriptor number

    struct sockaddr_in  sLocalAddr;
    struct sockaddr_in  sAddr;

    char    buf[256];    // buffer for client data
    int     nbytes;

    //filling the TCP server socket address
    sLocalAddr.sin_family = AF_INET;
    sLocalAddr.sin_port = htons((uint16_t)usPort);
    sLocalAddr.sin_addr.s_addr = 0;

    // creating a TCP socket
    iSockID = socket(AF_INET, SOCK_STREAM, 0);
    if (iSockID < 0) {
        return SOCKET_CREATE_ERROR;
    }

    iAddrSize = sizeof(sLocalAddr);

    // binding the TCP socket to the TCP server address
    iStatus = bind(iSockID, (struct sockaddr *)&sLocalAddr, iAddrSize);
    if (iStatus < 0) {
        // error
        close(iSockID);
        return BIND_ERROR;
    }

    // putting the socket for listening to the incoming TCP connection
    iStatus = listen(iSockID, BACKLOG);
    if (iStatus < 0) {
        close(iSockID);
        return LISTEN_ERROR;
    }
/*
    // setting socket option to make the socket as non blocking
#ifdef TARGET_IS_CC3200
    long lNonBlocking = 1;
    iStatus = setsockopt(iSockID, SOL_SOCKET, O_NONBLOCK, 
                                &lNonBlocking, sizeof(lNonBlocking));
    if (iStatus < 0) {
        close(iSockID);
        return SOCKET_OPT_ERROR;
    }
#else
    int flags = fcntl(iSockID, F_GETFL, 0);
    if (flags < 0) {
        close(iSockID);
        return SOCKET_OPT_ERROR;
    }
    fcntl(iSockID, F_SETFL, flags | O_NONBLOCK);
#endif
*/
    // iNewSockID = EAGAIN;

    // clear the master and temp sets
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(iSockID, &master);

    // keep track of the biggest file descriptor
    fdmax = iSockID;

    for (;;) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) < 0) {
            return SELECT_ERROR;
        }

        // run through the existing connections looking for data to read
        int i;
        for(i = MIN_SOCKET; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {

                if (i == iSockID) {
                    // accepts a connection form a TCP client, if there is any
                    // otherwise returns SL_EAGAIN
                    int iNewSockID = accept(iSockID,
                        (struct sockaddr*)&sAddr,
                        (socklen_t*)&iAddrSize);

                    if (iNewSockID == -1) {
                        printf("err accept\n");
                    } else {
                        printf("accept %d\n", iNewSockID);
                        FD_SET(iNewSockID, &master); // add to master set
                        if (iNewSockID > fdmax) {    // keep track of the max
                            fdmax = iNewSockID;
                        }
                    }
                }
                else {
                    // handle data from a client
                    nbytes = recv(i, buf, sizeof buf, 0);
                    if (nbytes <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("err socket %d hung up\n", i);
                        } else {
                            printf("err recv\n");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    }
                    else {
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    }

    return 0;
}
