/*
 * conn_udp.c
 *
 *  Created on: 20Oct 2016
 *      Author: Jie Deng
 */
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr

#ifdef __unix__
#include <linux/if.h>
#include <linux/if_tun.h>
#else
#include <net/if.h>
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "com.h"
#include "conn_udp.h"
#include "hashmap.h"
#include <slog.h>
#include <json.h>

int udp_initiated = 0; // False;
HashMap* idsktaddr;
HashMap* idfd;
HashMap* fdtype;

void* thismodule;

struct sockaddr_in curr_send;
struct sockaddr_in currr_recv;

/*
 * listens NONBLOCK for connections as a server
 * returns address string of the server
 */
char* com_init(void* module, const char *config_json)
{
	thismodule = module;
	udp_init();

    JSON* args_json = json_new(config_json);
    char* server_address = json_get_str(args_json, "address");
    json_free(args_json);

	char* server_addr = udp_get_addr(server_address);
	int server_port = udp_get_port(server_address);
	if (server_port == -1) {
		slog(SLOG_ERROR,
				"CONN UDP: Illegal value for server port: %d.", server_port);
		return NULL;
	}

	int serversock;              //, peersock;
	struct sockaddr_in serverin; //, peerin;
	serversock = socket(AF_INET, SOCK_DGRAM, 0);
	if (serversock == -1) {
		slog(SLOG_ERROR, "CONN UDP: Could not create server socket.");
		return NULL;
	}

	serverin.sin_family = AF_INET;
	serverin.sin_addr.s_addr = INADDR_ANY;
	serverin.sin_port = htons(server_port);

	if (bind(serversock, (struct sockaddr*) &serverin, sizeof(serverin)) < 0) {
		slog(SLOG_ERROR, "CONN UDP: Bind to %s:%d failed: %s",
				server_addr, server_port, strerror(errno));
		// free(server_addr);
		return NULL;
	}
	char* buf;
	buf = (char*) malloc(sizeof(char) * 10);
	snprintf(buf, 10, "%d", serversock);
	map_insert(fdtype, buf, "s");
	slog(SLOG_INFO, "CONN: Bind to %s:%d successed.", server_addr,
			server_port);
	udp_run_receive_thread(serversock);

	return server_address;
}

int udp_init() {
	idsktaddr = map_new(KEY_TYPE_STR);
	idfd = map_new(KEY_TYPE_STR);
	fdtype = map_new(KEY_TYPE_STR);
	if (idsktaddr) {
		udp_initiated = 1;
	}
	return udp_initiated;
}

int com_is_bridge(void) {
	return 1;
}

void udp_run_receive_thread(int conn) {
	if (!udp_initiated)
		udp_init();
	pthread_t rcvthread;
	int err;
	int* conn_ptr = (int*) malloc(sizeof(int));
	*conn_ptr = conn;

	err = pthread_create(&rcvthread, NULL, &udp_receive_function,
			(void*) conn_ptr);
	if (err != 0) {
		slog(SLOG_ERROR, "CONN UDP: can't create receive thread  for (%d).",
				conn);
		return;
	}
	err = pthread_detach(rcvthread);
	if (err != 0) {
		slog(SLOG_ERROR, "CONN UDP: Could not detach rcv thread for (%d) ",
				conn);
		return;
	}

	slog(SLOG_INFO,
			"CONN UDP: Receive thread created successfully for (%d).",
			*conn_ptr);
}

void udp_stop_receive_thread(int conn) {
}

void* udp_receive_function(void* conn) {
	int _conn = *((int*) conn);
	if (_conn <= 0) {
		slog(SLOG_ERROR,
				"CONN UDP: not established with (%d), can't recv", conn);
		return NULL;
	}

	char* buf;
	do {
		/* read message */
		buf = udp_receive_message(_conn);

		/* recv failed or disconnected */
		if (buf == NULL) {
			continue;
			// return NULL;
		}

		free(buf); /* TODO: it can't be passed btw threads */

	} while (1);
	return NULL;
}

char* udp_receive_message(int _conn) {

	uint32_t varSize;
	int allBytesRecv; /* sum af all received packets */
	int recvSize; /* one pack received size */

	struct sockaddr_in* peerin;
	peerin = malloc(sizeof(struct sockaddr_in));
	// bzero(peerin, sizeof(struct sockaddr_in));
	socklen_t addrlen = sizeof(peerin);
	int count = 0;

	slog(SLOG_INFO,
			"CONN UDP: About to receive message using udp on socket (%d)",
			_conn);
	do {
		/* reading msg */

		char* buf;
		buf = (char*) malloc((512 + 1) * sizeof(char));
		memset(buf, '\0', (512 + 1));
		recvSize = recvfrom(_conn, buf, 512, 0,
				(struct sockaddr*) peerin, &addrlen);

		if (recvSize == -1) {
			slog(SLOG_WARN,
					"CONN UDP: Recv msg failed from (%d). closing connection ",
					_conn);
			// com_connection_close(_conn);
			if (on_disconnect_handler)
				(*on_disconnect_handler)(thismodule, _conn);
			else
				com_connection_close(_conn);
			return NULL;
		}
		char* buffer;
		buffer = (char*) malloc(sizeof(char) * 10);
		snprintf(buffer, 10, "%d", _conn);

		if (map_contains(fdtype, buffer)) {
			if (strcmp("s", map_get(fdtype, buffer)) == 0) { // server
				printf("serv fd %d\n", _conn);
				char* buffer2;
				buffer2 = (char*) malloc(sizeof(char) * 10);
				snprintf(buffer2, 10, "%d", ntohs(peerin->sin_port));
				if (!map_contains(idfd, buffer2)) { // first time only
					printf("recv conn %d with port %s and sock %d\n", _conn,
							buffer2, ntohs(peerin->sin_port));
					map_insert(idsktaddr, buffer2, peerin); // peer port & skt_addr
					map_insert(idfd, buffer2, &_conn);

					if (on_connect_handler != NULL)
						(*on_connect_handler)(thismodule,
								ntohs(peerin->sin_port));
				}

				//  map_insert(idfd, buffer, &_conn); // peer port & fd
			} else {
				printf("client fd %d\n", _conn);
			}
		} else { // from peer
			printf("from peer %d\n", _conn);
		}

		slog(SLOG_INFO,
				"CONN UDP: received %d total bytes on sock (%d): *%s*",
				allBytesRecv, _conn, buf);
		/* apply message handler in a different thread */
		if (map_contains(fdtype, buffer)) {
			if (strcmp("s", map_get(fdtype, buffer)) == 0) { // server
				printf("msg on serv %d\n", _conn);
				if (on_data_handler != NULL)
					(*on_data_handler)(thismodule, ntohs(peerin->sin_port),
							buf, strlen(buf));
			} else {
				printf("msg on cli%d\n", _conn);
				if (on_data_handler != NULL)
					(*on_data_handler)(thismodule, _conn, buf, strlen(buf));
			}
		} else { // from peer
			printf("msg on peer%d\n", _conn);
			if (on_data_handler != NULL)
				(*on_data_handler)(thismodule, ntohs(peerin->sin_port), buf, strlen(buf));
		}

		return buf;
	} while (1);
}

int com_connect(const char* server_address) {
	if (!udp_initiated)
		udp_init();
	if (!com_is_valid_address(server_address)) {
		slog(SLOG_ERROR, "Invalid address format given: %s",
				server_address);
		return -1;
	}

	char* server_addr = udp_get_addr(server_address);
	int server_port = udp_get_port(server_address);
	slog(SLOG_INFO, "CONN UDP: Connecting to server %s:%d",
			server_addr, server_port);

	int peersock;
	peersock = socket(
	AF_INET, SOCK_DGRAM, 0); // not build the sockaddr_in here, cause it won't be passed to another (send) thread
	if (peersock == -1) {
		slog(SLOG_ERROR,
				"CONN UDP: Could not create client socket.");
		return -1;
	}
	struct sockaddr_in* peerin = (struct sockaddr_in*) malloc(
			sizeof(struct sockaddr_in));
	peerin->sin_addr.s_addr = inet_addr(server_addr);
	peerin->sin_family = AF_INET;
	peerin->sin_port = htons(server_port);

	//  free(server_addr);
	char* buf;
	buf = (char*) malloc(sizeof(char) * 10);
	snprintf(buf, 10, "%d", peersock);
	map_insert(idsktaddr, buf, peerin);
	map_insert(fdtype, buf, "c");

	slog(SLOG_INFO, "CONN UDP: server connected on %d", peersock);

	if (on_connect_handler != NULL)
		(*on_connect_handler)(thismodule, peersock);
	udp_run_receive_thread(peersock);

	// receivethread = rcvthread;
	return peersock;
}

int com_send_data(int pconn, const char* msg) {
	// printf("about to send msg%d\n", pconn);
	// in udp, conn is actually the peer's port (destination port); this is for distinguish the peers from the same fd
	char* buffer;
	buffer = (char*) malloc(sizeof(char) * 10);
	snprintf(buffer, 10, "%d", pconn);
	int conn;
	if (map_contains(idfd, buffer)) {
		conn = *((int*) (map_get(idfd, buffer)));
	} else {
		conn = pconn;
	}
	struct sockaddr_in peer;
	snprintf(buffer, 10, "%d", pconn);
	// printf("%d \n", map_size(idsktaddr));
	if (map_contains(idsktaddr, buffer)) {
		peer = *((struct sockaddr_in*) (map_get(idsktaddr, buffer)));
		printf(
				"about to send msg to %d ,send to conn %d  port %d; map size %d\n",
				pconn, conn, ntohs(peer.sin_port), map_size(idsktaddr));
	} else {
		slog(SLOG_ERROR,
				"CONN UDP: not established with (%d), can't send msg *%s*",
				conn, msg);
		return -1;
	}

	slog(SLOG_INFO, "CONN UDP: send to (%d) *%s*", conn, msg);

	const uint32_t varSize = strlen(msg);
	int allBytesSent; /* sum of all sent sizes */
	ssize_t sentSize; /* one shot sent size */

	allBytesSent = 0;
	while (allBytesSent < varSize) {
		sentSize = sendto(conn, msg + allBytesSent, varSize - allBytesSent, 0,
				(struct sockaddr*) &peer, sizeof(peer));
		/*if(varSize - allBytesSent < 512)
		 sentSize = send(conn , msg+allBytesSent , varSize - allBytesSent , 0);
		 else
		 sentSize = send(conn , msg+allBytesSent , 512 , 0);*/
		if (sentSize < 0) {
			slog(SLOG_ERROR,
					"CONN UDP: error sending msg on sock (%d)", conn);
			break;
		}
		allBytesSent += sentSize;
	}
	slog(SLOG_INFO, "CONN UDP: send to (%d) finished ", conn);
	return (int) allBytesSent;
}


int com_send(int pconn, const void* msg, unsigned int size) {
	// printf("about to send msg%d\n", pconn);
	// in udp, conn is actually the peer's port (destination port); this is for distinguish the peers from the same fd
	char* buffer;
	buffer = (char*) malloc(sizeof(char) * 10);
	snprintf(buffer, 10, "%d", pconn);
	int conn;
	if (map_contains(idfd, buffer)) {
		conn = *((int*) (map_get(idfd, buffer)));
	} else {
		conn = pconn;
	}
	struct sockaddr_in peer;
	snprintf(buffer, 10, "%d", pconn);
	// printf("%d \n", map_size(idsktaddr));
	if (map_contains(idsktaddr, buffer)) {
		peer = *((struct sockaddr_in*) (map_get(idsktaddr, buffer)));
		printf(
				"about to send msg to %d ,send to conn %d  port %d; map size %d\n",
				pconn, conn, ntohs(peer.sin_port), map_size(idsktaddr));
	} else {
		slog(SLOG_ERROR,
				"CONN UDP: not established with (%d)",
				conn);
		return -1;
	}

	int allBytesSent; /* sum of all sent sizes */
	ssize_t sentSize; /* one shot sent size */

	allBytesSent = 0;
	while (allBytesSent < size) {
		sentSize = sendto(conn, msg + allBytesSent, size - allBytesSent, 0,
				(struct sockaddr*) &peer, sizeof(peer));
		/*if(varSize - allBytesSent < 512)
		 sentSize = send(conn , msg+allBytesSent , varSize - allBytesSent , 0);
		 else
		 sentSize = send(conn , msg+allBytesSent , 512 , 0);*/
		if (sentSize < 0) {
			slog(SLOG_ERROR,
					"CONN UDP: error sending msg on sock (%d)", conn);
			break;
		}
		allBytesSent += sentSize;
	}
	slog(SLOG_INFO, "CONN UDP: send to (%d) finished ", conn);
	return (int) allBytesSent;
}

int com_connection_close(int conn) {
	return 0;
}

int com_set_on_data(void (*handler)(void*, int, const void*, unsigned int)) {
	on_data_handler = handler;
	// return (int)map_insert(handler_table, &conn, _handler);
	return 0;
}

int com_set_on_connect(void (*handler)(void*, int)) {
	on_connect_handler = handler;
	// return (int)map_insert(handler_table, &conn, _handler);
	return 0;
}

int com_set_on_disconnect(void (*handler)(void*, int)) {
	on_disconnect_handler = handler;
	// return (int)map_insert(handler_table, &conn, _handler);
	return 0;
}

int com_is_valid_address(const char* full_address) {
	return udp_is_addr(full_address);
}
//below are functions moved from conn.c
int udp_is_addr(const char* full_address) {
	char* addr = udp_get_addr(full_address);
	int port = udp_get_port(full_address);

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

char *udp_get_addr(const char * full_address) {
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

int udp_get_port(const char * full_address) {
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
