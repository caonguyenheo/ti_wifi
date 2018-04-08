#include "cvhttpd.h"
#include <string.h>

struct {
    char *ext;
    char *filetype;
} extensions [] = {
    {"gif", "image/gif" },
    {"jpg", "image/jpg" },
    {"jpeg", "image/jpeg"},
    {"png", "image/png" },
    {"ico", "image/ico" },
    {"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {"htm", "text/html" },
    {"html", "text/html" },
    {0, 0}
};

int handle_request(int fd, const char *url, int url_len)
{
    /* check for illegal parent directory use .. */
    for (int j = 0; j < url_len - 1; j++)
        if (url[j] == '.' && url[j + 1] == '.') {
            logger(HTTPFORBIDDEN, "Parent directory (..) path names not supported", url, fd);
            return -1;
        }
    /* convert no filename to index file */
    /*
    if (!strncmp(&url[0], "GET /\0", 6) || !strncmp(&url[0], "get /\0", 6))
        (void)strcpy(url, "GET /index.html");
    */

    /* work out the file type and check we support it */
    char *fstr = NULL;
    int buflen = strlen(url);
    fstr = (char *)0;

    for (long i = 0; extensions[i].ext != 0; i++) {
        long len = strlen(extensions[i].ext);
        if (!strncmp(&url[buflen - len], extensions[i].ext, len)) {
            fstr = extensions[i].filetype;
            break;
        }
    }

#if HAS_EXENSION_CHECK
    if (fstr == 0) {
        logger(HTTPFORBIDDEN, "file extension type not supported", url, fd);
        return -1;
    }
#endif

    logger(HTTPLOG, "SEND", &url[5], fd);

#ifdef __linux__
    int file_fd;
    char send_msg[BUFSIZE + 1]; /* static so zero filled */

    if ((file_fd = open(&url[5], O_RDONLY)) == -1) { /* open the file for reading */
        logger(HTTPNOTFOUND, "failed to open file", &url[5], fd);
        return -1;
    }

    /* send response with header */
    long msg_len = (long)lseek(file_fd, (off_t)0, SEEK_END); /* lseek to the file end to find the length */
    (void)lseek(file_fd, (off_t)0, SEEK_SET); /* lseek back to the file start ready for reading */

    /* Header + a blank line */
    snprintf(send_msg, sizeof(send_msg),
            "HTTP/1.1 200 OK\nServer: cvhttpd/%d.0\nContent-Length: %ld\nConnection: close\nContent-Type: %s\n\n",
            HTTPVERSION,
            msg_len,
            fstr);
    logger(HTTPLOG, "Header", send_msg, fd);
    send(fd, send_msg, strlen(send_msg), 0);

    /* send file in 8KB block - last block may be smaller */
    long ret;
    while ((ret = read(file_fd, send_msg, BUFSIZE)) > 0) {
        (void)send(fd, send_msg, ret, 0);
    }

    // attempt to exit
    httpd_status = HTTPD_EXIT;

#elif defined(TARGET_IS_CC3200)
    send(fd, DEFAULT_MSG, strlen(DEFAULT_MSG), 0);
#endif

    return 0;
}
