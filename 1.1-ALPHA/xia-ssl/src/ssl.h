
#ifndef TUNNEL_H
#define TUNNEL_H

#include <openssl/dh.h>
#include <openssl/ssl.h>

extern DH *get_dh1024();
extern int ssl_init(int, char *, int);
extern void ssl_error_stack();
extern DH *tmp_dh_callback(SSL *, int, int);

#define SSL_SERVER 0x1
#define SSL_CLIENT 0x2

#endif
