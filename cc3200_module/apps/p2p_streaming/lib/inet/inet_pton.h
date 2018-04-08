#ifndef __INET_PTON_H__
#define __INET_PTON_H__

#define u_char          unsigned char
#define u_int           unsigned int
#define NS_INADDRSZ     4

int inet_pton4 (const char *src, u_char *dst);
void unit_test_inet(void *arg);

#endif // __INET_PTON_H__
