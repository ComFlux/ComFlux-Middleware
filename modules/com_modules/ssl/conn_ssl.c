/*
 * conn_ssl.c
 *
 *  Created on: 19 oct 2016
 *      Author: Jie Deng
 * gcc -c -fPIC ../common/conn_ssl.c ../common/conn.c ../utils/slog.c ../utils/hashmap.c -I"../common/" -I"../utils"
 *-I"../../lib/uthash/include/" -I"../../lib/slog/src" -I"../../lib/wjelement/src."
 */

#include <errno.h>

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>

#include <sys/ioctl.h>
#include <netinet/in.h>

#ifdef __unix__
#include <linux/if.h>
#include <linux/if_tun.h>
#else
#include <net/if.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "config.h"
#include "hashmap.h"
#include "json.h"

#include "com.h"
#include "conn_ssl.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

int ssl_initiated = 0; // False

HashMap* skt_ssl;  //<socket,ssl> pair
HashMap* skt_type; //"s"  server or "c"  client

void* thismodule;

const char* cli_keyfile;
const char* cli_cafile;
const char* cli_certfile;

const char* srv_keyfile;
const char* srv_cafile;
const char* srv_certfile;

void ssl_run_accept_thread(int serversock);
void* ssl_accept_function(void* conn);
void ssl_run_receive_thread(int conn);
void* ssl_receive_function(void* conn); // ssl needs to be created inside the thread

SSL* ssl_start_server(int conn);
SSL* ssl_start_client(int conn);

char* com_init(void* module, const char* cfgfile){
	//init module
	thismodule = module;
	JSON* rootfile = json_new(cfgfile);
	// metadata

	// config
	JSON* config = json_get_json(rootfile, "configs");

	conn_ssl_set_cli_cacert(json_get_str(config, "cli_cafile"));

	conn_ssl_set_cli_certfile(json_get_str(config, "cli_certfile"));

	conn_ssl_set_cli_keyfile(json_get_str(config, "cli_keyfile"));

	conn_ssl_set_srv_cacert(json_get_str(config, "srv_cafile"));

	conn_ssl_set_srv_certfile(json_get_str(config, "srv_certfile"));

	conn_ssl_set_srv_keyfile(json_get_str(config, "srv_keyfile"));

	//init ssl
	slog(SLOG_INFO, "CONN_SSL: init ssl funcations;");
	SSL_library_init();
	OpenSSL_add_ssl_algorithms();
	SSL_load_error_strings();
	ERR_load_crypto_strings();

	skt_type = map_new(KEY_TYPE_INT);
	skt_ssl = map_new(KEY_TYPE_INT);
	if (skt_ssl) {
		ssl_initiated = 1; // TRUE;
	}

	//init server
	if (!ssl_initiated)
		ssl_init();
	char* server_addr = get_addr(json_get_str(config, "address"));
	int server_port = get_port(json_get_str(config, "address"));
	if (server_port == -1) {
		slog(SLOG_ERROR, "CONN: Illegal value for server port: %d.",
				server_port);
		return -1;
	}

	int serversock;              //, peersock;
	struct sockaddr_in serverin; //, peerin;

	serversock = socket(AF_INET, SOCK_STREAM, 0);
	if (serversock == -1) {
		slog(SLOG_ERROR, "CONN: Could not create server socket.");
		return -1;
	}

	serverin.sin_family = AF_INET;
	serverin.sin_addr.s_addr = INADDR_ANY;
	serverin.sin_port = htons(server_port);

	if (bind(serversock, (struct sockaddr*) &serverin, sizeof(serverin)) < 0) {
		slog(SLOG_ERROR, "CONN: Bind to %s:%d failed: %s",
				server_addr, server_port, strerror(errno));
		free(server_addr);
		return -1;
	}
	//free(server_addr);
	listen(serversock, 3);

	/*
	 slog(SLOG_INFO, "CONN: Waiting for the app connection... ");
	 int c = sizeof(struct sockaddr_in);
	 peersock = accept(serversock, (struct sockaddr *)&peerin, (socklen_t*)&c);
	 if (peersock < 0)
	 {
	 slog(SLOG_ERROR, "CONN: accept failed."); //, strerror(peersock)); //
	 return 0; // TODO
	 }
	 slog(SLOG_INFO, "CONN: Connected to api ");

	 (*on_connect_handler)(peersock);

	 run_receive_thread(peersock);
	 receivethread = rcvthread;
	 */
	ssl_run_accept_thread(serversock);

	return server_addr;
}

int com_connect(const char *server_address) {
	// init client

	if (!com_is_valid_address(server_address)) {
		slog(SLOG_ERROR, "Invalid address format given: %s",
				server_address);
		return -1;
	}

	char* server_addr = get_addr(server_address);
	int server_port = get_port(server_address);
	slog(SLOG_INFO, "CONN: Connecting to server %s:%d", server_addr,
			server_port);

	int peersock;
	struct sockaddr_in peerin;

	peersock = socket(AF_INET, SOCK_STREAM, 0);
	if (peersock == -1) {
		slog(SLOG_ERROR, "CONN: Could not create client socket.");
		return -1;
	}

	peerin.sin_addr.s_addr = inet_addr(server_addr);
	peerin.sin_family = AF_INET;
	peerin.sin_port = htons(server_port);

	if (connect(peersock, (struct sockaddr*) &peerin, sizeof(peerin)) < 0) {
		slog(SLOG_ERROR, "CONN: Could not connect to server %s:%d",
				server_addr, server_port);
		free(server_addr);
		return -1;
	}
	free(server_addr);
	map_insert(skt_type, &peersock, "c");

	ssl_run_receive_thread(peersock);
	return peersock;
}

int com_connection_close(int conn){
	return close(conn);
}

int com_send(int conn, const void *data, unsigned int size) {
	if (conn <= 0) {
		slog(SLOG_ERROR,
				"CONN: not established with (%d), can't send msg *%s*", conn,
				msg);
		return -1;
	}
	slog(SLOG_INFO, "CONN: send to (%d) *%s*", conn, msg);

	int allBytesSent; /* sum of all sent sizes */
	ssize_t sentSize; /* one shot sent size */

	//sleep(1);
	slog(SLOG_INFO,
			"CONN: About to send message using ssl on socket (%d)", conn);
	SSL* ssl;
	while (!map_contains(skt_ssl, &conn)) {
		sleep(1);
		printf("send ssl not found yet\n");

	}
	ssl = map_get(skt_ssl, &conn);

	allBytesSent = 0;
	while (allBytesSent < size) {
		sentSize = SSL_write(ssl, msg + allBytesSent, size - allBytesSent);
		/*if(varSize - allBytesSent < 512)
		 sentSize = send(conn , msg+allBytesSent , varSize - allBytesSent , 0);
		 else
		 sentSize = send(conn , msg+allBytesSent , 512 , 0);*/
		if (sentSize < 0) {
			slog(SLOG_ERROR, "CONN: error sending msg on sock (%d)",
					conn);
			break;
		}
		allBytesSent += sentSize;
	}

	return (int) allBytesSent;
}

int com_send_data(int conn, const char *msg) {
	if (conn <= 0) {
		slog(SLOG_ERROR,
				"CONN: not established with (%d), can't send msg *%s*", conn,
				msg);
		return -1;
	}
	slog(SLOG_INFO, "CONN: send to (%d) *%s*", conn, msg);

	int allBytesSent; /* sum of all sent sizes */
	ssize_t sentSize; /* one shot sent size */

	//sleep(1);
	slog(SLOG_INFO,
			"CONN: About to send message using ssl on socket (%d)", conn);
	SSL* ssl;
	while (!map_contains(skt_ssl, &conn)) {
		sleep(1);
		printf("send ssl not found yet\n");

	}
	ssl = map_get(skt_ssl, &conn);

	const uint32_t varSize = strlen(msg);
	allBytesSent = 0;
	while (allBytesSent < varSize) {
		sentSize = SSL_write(ssl, msg + allBytesSent, varSize - allBytesSent);
		/*if(varSize - allBytesSent < 512)
		 sentSize = send(conn , msg+allBytesSent , varSize - allBytesSent , 0);
		 else
		 sentSize = send(conn , msg+allBytesSent , 512 , 0);*/
		if (sentSize < 0) {
			slog(SLOG_ERROR, "CONN: error sending msg on sock (%d)",
					conn);
			break;
		}
		allBytesSent += sentSize;
	}

	return (int) allBytesSent;
}

int com_set_on_data(void (*handler)(void*, int, const char*)) {
	on_data_handler = handler;
}
int com_set_on_connect(void (*handler)(void*, int)) {
	on_connect_handler = handler;
}
int com_set_on_disconnect(void (*handler)(void*, int)) {
	on_disconnect_handler = handler;
}
int com_is_valid_address(const char* full_address) {
	char* addr = get_addr(full_address);
	int port = get_port(full_address);

	if (addr == NULL || port < 0)
		return 0;

	/* checks the addr */
	struct sockaddr_in sa;
	int is_addr = inet_pton(AF_INET, addr, &(sa.sin_addr));

	/* checks the port */
	int is_port = (port > 1024 && port <= 65535);

	free(addr);

	return is_addr && is_port;
}

int com_is_bridge(void) {
	return 1;
}

void ssl_run_accept_thread(int serversock) {
	int err;
	int* serversock_ptr = (int*) malloc(sizeof(int));
	pthread_t listenthread;
	*serversock_ptr = serversock;
	slog(SLOG_INFO,
			"CONN: Waiting for incoming connections for server (%d).",
			serversock);
	err = pthread_create(&listenthread, NULL, &ssl_accept_function,
			serversock_ptr);
	if (err != 0) {
		slog(SLOG_ERROR, "CONN: can't create listen thread"); //: %s", strerror(err));
		return;
	}
	err = pthread_detach(listenthread);
	if (err != 0) {
		slog(SLOG_ERROR, "CONN: Could not detach listen thread ");
		return;
	}

	slog(SLOG_INFO, "Listen thread created successfully.");
}

void* ssl_accept_function(void* serversock) {
	int peersock;
	struct sockaddr_in peerin;

	// accept connection from an incoming client
	int c = sizeof(struct sockaddr_in);
	while (1) {
		peersock = accept(*((int*) serversock), (struct sockaddr*) &peerin,
				(socklen_t*) &c);
		if (peersock < 0) {
			slog(SLOG_ERROR, "CONN: accept failed for (%d).", serversock);
			return NULL; // TODO
		}
		slog(SLOG_INFO, "CONN: connection accpeted by socket (%d) ",
				peersock);

		map_insert(skt_type, &peersock, "s");
		// on_connect calls after receive thread. As ssl is build in receive thread, and on_connect may send
		// message(requires ssl)

		ssl_run_receive_thread(peersock);

	}

	return NULL;
}

SSL* ssl_start_server(int fd) {
	const SSL_METHOD* method;
	SSL_CTX* ctx;
	method = SSLv23_server_method();
	ctx = SSL_CTX_new(method);
	if (!ctx) {
		slog(SLOG_ERROR, "Unable to create SSL context\n");
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	slog(SLOG_INFO,
			"CONN: creating server SSL connection on socket (%d) ", fd);
	// SSL_CTX_set_client_CA_list(ctx, SSL_load_client_CA_file("/home/jie/iot/middleware/src/test/cc@123.com.crt"));
	slog(SLOG_INFO, "CONN: ssl ca cert file %s ", srv_cafile);
	SSL_CTX_load_verify_locations(ctx, srv_cafile, ".");
	// SSL_CTX_set_ecdh_auto(ctx, 1);
	SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

	/* Set the key and cert */
	slog(SLOG_INFO, "CONN: ssl server cert file %s ", srv_certfile);
	if (SSL_CTX_use_certificate_file(ctx, srv_certfile, SSL_FILETYPE_PEM) < 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	slog(SLOG_INFO, "CONN: ssl server key file %s ", srv_keyfile);
	if (SSL_CTX_use_RSAPrivateKey_file(ctx, srv_keyfile, SSL_FILETYPE_PEM)
			< 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	if (!SSL_CTX_check_private_key(ctx)) {
		slog(SLOG_ERROR,
				"Server private key does not match the public certificate\n");
	}

	SSL* ssl = SSL_new(ctx);
	SSL_set_fd(ssl, fd);
	if (SSL_accept(ssl) <= 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	// two way auth
	X509* cert = NULL;
	if (cert = SSL_get_peer_certificate(ssl)) {
		if (SSL_get_verify_result(ssl) == X509_V_OK) {
			slog(SLOG_INFO,
					"CONN: SSL created successfuly on socket  (%d) ", fd);
			map_insert(skt_ssl, &fd, ssl);
			return ssl;
		} else { // no pass
			slog(SLOG_ERROR, "SSL verify not OK\n");
		}
	} else { // no cert
		slog(SLOG_ERROR, "No SSL cerf found from peer\n");
	}
	return NULL;
}

void ssl_run_receive_thread(int conn) {
	if (!ssl_initiated)
		ssl_init();
	int* conn_ptr = (int*) malloc(sizeof(int));
	*conn_ptr = conn;
	pthread_t rcvthread;
	int err;

	err = pthread_create(&rcvthread, NULL, &ssl_receive_function,
			(void*) conn_ptr);
	if (err != 0) {
		slog(SLOG_ERROR, "CONN: can't create receive thread  for (%d).",
				conn);
		return;
	}
	err = pthread_detach(rcvthread);
	if (err != 0) {
		slog(SLOG_ERROR, "CONN: Could not detach rcv thread for (%d) ",
				conn);
		return;
	}
	// on_connect calls after receive thread. As ssl is build in receive thread, and on_connect may send
	// message(requires ssl)
	sleep(1);
	if (on_connect_handler != NULL)
		(*on_connect_handler)(thismodule, conn);

	slog(SLOG_INFO, "CONN: Receive thread created successfully for (%d).",
			*conn_ptr);
}

void* ssl_receive_function(void* conn) {
	int _conn = *((int*) conn);
	if (_conn <= 0) {
		slog(SLOG_ERROR,
				"CONN: not established with (%d), can't recv", _conn);
		return NULL;
	}
	SSL* ssl;
	if (map_contains(skt_type, &_conn)) {
		if (strcmp(map_get(skt_type, &_conn), "s") == 0) {
			printf("create ssl server\n");
			ssl = ssl_start_server(_conn);

		} else if (strcmp(map_get(skt_type, &_conn), "c") == 0) {
			printf("create ssl client\n");
			//   printf("%s\n", map_get(skt_type, &_conn));
			//sleep(1);
			ssl = ssl_start_client(_conn);
		}
	} else {
		printf("default create ssl client\n");
		ssl = ssl_start_client(_conn);
	}

	map_insert(skt_ssl, &_conn, ssl);

	char* buf;
	do {
		/* read message */
		buf = tcp_receive_message(_conn);

		/* recv failed or disconnected */
		if (buf == NULL)
			return NULL;

		/* apply message handler in a different thread */
		if (on_data_handler != NULL)
			(*on_data_handler)(thismodule, _conn, buf);

		free(buf); /* TODO: it can't be passed btw threads */

	} while (1);
	return NULL;
}

SSL* ssl_start_client(int fd) {
	const SSL_METHOD* method;
	SSL_CTX* ctx;
	method = SSLv23_client_method();
	ctx = SSL_CTX_new(method);
	if (!ctx) {
		slog(SLOG_ERROR, "Unable to create SSL context\n");
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}

	slog(SLOG_INFO,
			"CONN: creating client SSL connection on socket (%d) ", fd);
	slog(SLOG_INFO, "CONN: ssl ca cert file %s ", cli_cafile);
	SSL_CTX_load_verify_locations(ctx, cli_cafile, ".");
	/* Set the key and cert */
	slog(SLOG_INFO, "CONN: ssl client cert file %s ", cli_certfile);
	if (SSL_CTX_use_certificate_file(ctx, cli_certfile, SSL_FILETYPE_PEM) < 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	slog(SLOG_INFO, "CONN: ssl client key file %s ", cli_keyfile);
	if (SSL_CTX_use_RSAPrivateKey_file(ctx, cli_keyfile, SSL_FILETYPE_PEM)
			< 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	if (!SSL_CTX_check_private_key(ctx)) {
		slog(SLOG_ERROR,
				"Client private key does not match the public certificate\n");
	}
	SSL* ssl = SSL_new(ctx);
	SSL_set_fd(ssl, fd);
	if (SSL_connect(ssl) < 0) {
		ERR_print_errors_fp(stderr);
		exit(EXIT_FAILURE);
	}
	// two way auth
	X509* cert = NULL;
	if (cert = SSL_get_peer_certificate(ssl)) {
		if (SSL_get_verify_result(ssl) == X509_V_OK) {
			slog(SLOG_INFO,
					"CONN: SSL created successfuly on socket  (%d) ", fd);
			printf("SSL created successfully on socket (%d)\n",fd);
			map_insert(skt_ssl, &fd, ssl);
			return ssl;
		} else { // no pass
			slog(SLOG_ERROR, "SSL verify not OK\n");
		}
	} else { // no cert
		slog(SLOG_ERROR, "No SSL cerf found from peer\n");
	}
	return NULL;
}

char* tcp_receive_message(int _conn) {

	uint32_t bufSize = 512;
	int recvSize; /* one pack received size */

	char* buf;
	slog(SLOG_INFO,
			"CONN: About to receive message using ssl on socket (%d)", _conn);

	SSL* ssl;
	if (!map_contains(skt_ssl, &_conn)) {
		printf("receive ssl not found, return\n");
		return "";
	} else {
		ssl = map_get(skt_ssl, &_conn);
	}

	/* reading msg */
	buf = (char*) malloc((bufSize + 1) * sizeof(char));
	memset(buf, '\0', (bufSize + 1));

	recvSize = SSL_read(ssl, buf, bufSize);
	if (recvSize <=0 ) {
		slog(SLOG_WARN,
				"CONN: Recv msg failed from (%d). closing connection ", _conn);
		// conn_close(_conn);
		if (on_disconnect_handler)
			(*on_disconnect_handler)(thismodule, _conn);
		else
			conn_close(_conn);
		return NULL;
	}

	slog(SLOG_INFO,
			"CONN: received %d total bytes on sock (%d): *%s*", recvSize, _conn,
			buf);
	return buf;
}

void conn_ssl_set_cli_certfile(const char* cert) {
	cli_certfile = cert;
}

void conn_ssl_set_cli_keyfile(const char* key) {
	cli_keyfile = key;
}

void conn_ssl_set_cli_cacert(const char* key) {
	cli_cafile = key;
}

void conn_ssl_set_srv_certfile(const char* cert) {
	srv_certfile = cert;
}

void conn_ssl_set_srv_keyfile(const char* key) {
	srv_keyfile = key;
}

void conn_ssl_set_srv_cacert(const char* key) {
	srv_cafile = key;
}

char* conn_ssl_get_cert_subject(X509* cert) {
	return X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
}

//below are functions moved from conn.c
char *get_addr(const char * full_address) {
	if (full_address == NULL)
		return NULL;

	char *addr = strdup(full_address);
	char *pos = strchr(addr, ':'); //strpbrk

	if (pos == NULL) {
		free(addr);
		return NULL;
	}

	pos[0] = '\0';

	return addr;
}

int get_port(const char * full_address) {
	if (full_address == NULL)
		return -1;

	char *addr = strdup(full_address);
	char *pos = strchr(addr, ':'); //strpbrk

	if (pos == NULL)
		return -1;

	int port;
	int scanned = sscanf(pos + 1, "%d", &port);

	free(addr);
	if (scanned < 1)
		return -1;

	return port;
}
