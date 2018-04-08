#include "cvhttpd.h"
#include <string.h>

#define BACKLOG     2
static int web(int);

httpd_status_t httpd_status = HTTPD_IDLE;

int32_t httpd_start(uint16_t usPort)
{
    int             iStatus;
    int             iAddrSize;
    int             iSockID;
    fd_set  master;    // master file descriptor list
    fd_set  read_fds;  // temp file descriptor list for reading
    int     fdmax;        // maximum file descriptor number

    struct sockaddr_in  sLocalAddr;
    struct sockaddr_in  sAddr;

    // set status
    httpd_status = HTTPD_RUNNING;

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

    // clear the master and temp sets
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(iSockID, &master);

    // keep track of the biggest file descriptor
    fdmax = iSockID;

    for (;;) {
        read_fds = master; // copy it
        if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) < 0) {
            return SELECT_ERROR;
        }

        // run through the existing connections looking for data to read
        for (int i = MIN_SOCKET; i <= fdmax; i++) {
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
                    printf("handle data from %d\n", i);
                    if (web(i) >= 0 && httpd_status == HTTPD_EXIT)
                        goto close_http;

                    sleep(2);   /* allow socket to drain before signalling the socket is closed */

                    close(i);   // bye
                    FD_CLR(i, &master); // remove from master set
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    }

close_http:
    // clean opened socket
    for (int i = MIN_SOCKET; i <= fdmax; i++)
        if (FD_ISSET(i, &read_fds))
            close(i);

    httpd_status = HTTPD_IDLE;

    return 0;
}

#define CHECK(type, s1, s2, fd) do { \
        if (logger(type, s1, s2, fd) < 0) \
            return -1; \
    } while(0)


int
logger(int type, const char *s1, const char *s2, int socket_fd)
{
#ifdef __linux__
    char logbuffer[BUFSIZE * 2];
#endif

    switch (type) {
    case HTTPERROR:
#ifdef __linux__
        (void)sprintf(logbuffer, "HTTPERROR: %s:%s Errno=%d exiting pid=%d", s1, s2, errno, getpid());
#endif
        break;

    case HTTPFORBIDDEN:
        (void)send(socket_fd, FORBIDDEN_MSG, strlen(FORBIDDEN_MSG), 0);
#ifdef __linux__
        (void)sprintf(logbuffer, "HTTPFORBIDDEN: %s:%s", s1, s2);
#endif
        break;

    case HTTPNOTFOUND:
        (void)send(socket_fd, NOTFOUND_MSG, strlen(NOTFOUND_MSG), 0);
#ifdef __linux__
        (void)sprintf(logbuffer, "NOT FOUND: %s:%s", s1, s2);
#endif
        break;

    case HTTPLOG:
#ifdef __linux__
        (void)sprintf(logbuffer, " INFO: %s:%s:%d", s1, s2, socket_fd);
#endif
        break;
    }

#ifdef __linux__
    int fd ;
    /* No checks here, nothing can be done with a failure anyway */
    if ((fd = open("cvhttpd.log", O_CREAT | O_WRONLY | O_APPEND, 0644)) >= 0) {
        (void)write(fd, logbuffer, strlen(logbuffer));
        (void)write(fd, "\n", 1);
        (void)close(fd);
    }
#endif

    if (type == HTTPERROR || type == HTTPNOTFOUND || type == HTTPFORBIDDEN)
        return -1;

    return 0;
}


/* this is a child web server process, so we can exit on errors */
static int
web(int fd)
{
    long ret;
    long i;
    int hit = 0;
    char buffer[BUFSIZE + 1]; /* static so zero filled */

    ret = recv(fd, buffer, BUFSIZE, 0);   /* read Web request in one go */
    if (ret == 0 || ret == -1) { /* read failure stop now */
        CHECK(HTTPFORBIDDEN, "failed to read browser request", "", fd);
    }

    if (ret > 0 && ret < BUFSIZE)    /* return code is valid chars */
        buffer[ret] = 0;    /* terminate the buffer */
    else
        buffer[0] = 0;

    for (i = 0; i < ret; i++) /* remove CF and LF characters */
        if (buffer[i] == '\r' || buffer[i] == '\n')
            buffer[i] = '*';

    CHECK(HTTPLOG, "request", buffer, hit);


    /* we only support GET/get request */
    if (strncmp(buffer, "GET ", 4) && strncmp(buffer, "get ", 4)) {
        CHECK(HTTPFORBIDDEN, "Only simple GET operation supported", buffer, fd);
    }

    /* null terminate after the second space to ignore extra stuff */
    for (i = 4; i < BUFSIZE; i++) {
        /* string is "GET URL " +lots of other stuff */
        if (buffer[i] == ' ') {
            buffer[i] = 0;
            break;
        }
    }

    /* then we have buffer contain url */
    return handle_request(fd, buffer, strlen(buffer));
}

