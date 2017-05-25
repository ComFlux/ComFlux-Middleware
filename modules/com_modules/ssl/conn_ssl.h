/*
 * conn_ssl.h
 *
 *  Created on: 19Oct 2016
 *      Author: Jie Deng
 */

#ifndef CONN_SSL_H_
#define CONN_SSL_H_
#include <openssl/ssl.h>

char* tcp_receive_message(int conn);

/*
 *ssl parts: set cert, key and ca cert; 
 */
 void conn_ssl_set_cli_cacert(const char* cacert);
void conn_ssl_set_cli_certfile(const char* cert);
void conn_ssl_set_cli_keyfile(const char* key);

 void conn_ssl_set_srv_cacert(const char* cacert);
void conn_ssl_set_srv_certfile(const char* cert);
void conn_ssl_set_srv_keyfile(const char* key);


char* conn_ssl_get_ssl_by_fd(int conn);
X509*  conn_ssl_get_ssl_cert(SSL* ssl);
char* conn_ssl_get_cert_subject(X509* cert); //get the cert subject, for authentication with 


//common functions
int   get_port(const char * full_address);
char *get_addr(const char * full_address);

#endif /* COMM_H_ */
