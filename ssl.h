#ifndef SSL_H
#define SSL_H
#include <openssl/err.h>
#include <openssl/ssl.h>

SSL *ssl_accept_client(int fd, SSL_CTX *ctx);
SSL_CTX *ssl_init();
int ssl_set_cert_file(SSL_CTX *ctx, const char *certfile);
int ssl_set_key_file(SSL_CTX *ctx, const char *keyfile);
void ssl_perror(const char *prefix);

#endif /* ndef SSL_H */
