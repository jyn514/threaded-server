#include <stdbool.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
SSL *set_up_ssl(int socket) {
    char error_buf[1000];

    // on a new connection
    SSL_CTX *ctx = SSL_CTX_new(TLS_server_method());
    if (ctx == NULL) {  // ?
        ERR_error_string_n(ERR_get_error(), error_buf, 1000);
        puts(error_buf);
    }
    SSL *ssl = SSL_new(ctx);
    if (ssl == NULL) { // ?
        ERR_error_string_n(ERR_get_error(), error_buf, 1000);
        puts(error_buf);
    }
    if (SSL_set_fd(ssl, socket) == 0) {
        ERR_error_string_n(ERR_get_error(), error_buf, 1000);
        puts(error_buf);
    }
    int ret;
    if ((ret = SSL_accept(ssl) != 1)) {
        int err = SSL_get_error(ssl, ret);
        ERR_error_string_n(err, error_buf, 1000);
        puts(error_buf);
    }
    return ssl;
}
