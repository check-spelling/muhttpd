#include "ssl.h"

/** Accept a client. */
SSL *ssl_accept_client(int fd, SSL_CTX *ctx) {
	SSL *ssl = SSL_new(ctx);
	if(!ssl) return NULL;

	if(!SSL_set_fd(ssl, fd)) {
		SSL_free(ssl);
		return NULL;
	}

	if(SSL_accept(ssl) != 1) {
		SSL_free(ssl);
		return NULL;
	}

	return ssl;
}

/** Initialize SSL support and create SSL context. */
SSL_CTX *ssl_init() {
	SSL_library_init();

	return SSL_CTX_new(SSLv23_server_method());
}
	
/** Set SSL certificate file. */
int ssl_set_cert_file(SSL_CTX *ctx, const char *certfile) {
	if(SSL_CTX_use_certificate_file(ctx, certfile,
		SSL_FILETYPE_PEM) != 1) {
		return -1;
	}
	return 0;
}

/** Set SSL key file. */
int ssl_set_key_file(SSL_CTX *ctx, const char *keyfile) {
	if(SSL_CTX_use_PrivateKey_file(ctx, keyfile,
		SSL_FILETYPE_PEM) != 1) {
		return -1;
	}
	return 0;
}

/** Print errors that occurred in the SSL code. */
void ssl_perror(const char *prefix) {
	SSL_load_error_strings();
	fprintf(stderr, "%s: ", prefix);
	ERR_print_errors_fp(stderr);
	fflush(stderr);
}
