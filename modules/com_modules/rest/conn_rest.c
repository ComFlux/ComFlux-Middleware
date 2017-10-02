/*
 * conn_rest.c
 *
 *  Created on: Mar 15, 2017
 *      Author: jie
 *
 *  This is module is for the middleware to talk directly to a HTTP entity,
 *  eg HTTP client or HTTP server.
 *  To establish connection and get data back in one round of message,
 *  the HTTP client send the map query in the GET/POST request, and
 *  this module init the map command and send data back.
 */

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "com.h"
#include "conn_rest.h"
#include "hashmap.h"
#include "com_wrapper.h"
#include "slog.h"
#include "json.h"

/*
 * listens NONBLOCK for connections as a server
 * returns connection id
 */
COM_MODULE* basemodule;
COM_MODULE* this; /* this module*/
HashMap* skttype;

HashMap* conn_table;   /* conn -> rest_peer */

typedef struct _rest_peer__
{
	char* host;
	char* address;

	int client; /* 1 - client; 0 - server */

	int fd;
	char* fd_str;
	int connected;

	pthread_t listenthread;
}_rest_peer;

_rest_peer* rest_new(const char* host, int fd, int client)
{
	//static int conn_counter = 0;
	_rest_peer* peer;
	int err;

	/* if bad data */
	if(host == NULL)
		return NULL;

	peer = (_rest_peer*) malloc(sizeof(_rest_peer));
	peer->host = strdup(host);
	peer->client = client;

	peer->connected = 0;
	peer->fd = fd; //++conn_counter;

	peer->fd_str = (char*)malloc(20*sizeof(char));
	sprintf(peer->fd_str, "%d", peer->fd);

	//printf("channel new %d addr:%s, client:%d\n", peer->fd, peer->host, peer->client);
	return peer;
}

void rest_free(_rest_peer* peer)
{
	if (peer == NULL)
		return;

	map_remove(conn_table, (void*)peer->fd_str);

	free(peer->address);
	free(peer->fd_str);
	free(peer);
}

char* com_init(void* module, const char* config_json) {
	this = module;

    /* initialise connection table: connection id -> peer */
    conn_table = map_new(KEY_TYPE_STR);

	//read cfgfile & load base module
	JSON* jfile = json_new(config_json);
	char* server_address = load_base_module(
			json_get_str(jfile, "basemodulepath"),
			json_get_str(jfile, "basemodulecfg"));
	//the server should be loaded automatically
	skttype = map_new(KEY_TYPE_PTR);
	if (basemodule == NULL) {
		return NULL;
	} else {
		printf("%s\n", server_address);
		return server_address;
	}

}

char* file_to_str(const char* filename) {

	FILE* _file = fopen(filename, "r");

	if (_file == NULL)
		return NULL;

	/* get size of file */
	fseek(_file, 0L, SEEK_END);
	int size = (int) ftell(_file);
	rewind(_file);

	/* read file */
	char* var;
	var = (char*) malloc((size_t) size + 1);
	memset(var, 0, (size_t) size + 1);

	fread(var, 1, (size_t) size, _file);
	var[size] = '\0';
	fclose(_file);

	/* return text */
	return var;
}

char* load_base_module(const char* lib_path, const char* cfgfile) {

	char* config_json = file_to_str(cfgfile); //can't use text_load_from_file which cause error; don't know why
	printf("working in rest %s\n", config_json);
	basemodule = com_module_new(lib_path, config_json); // com_get_module(path);//change after socketpair with app
	if (basemodule == NULL) {
		slog(SLOG_ERROR, "COMM REST: could not load com module %s",
				lib_path);
		return NULL;
	}

	(*(basemodule->fc_set_on_data))(&on_message_for_base);
	(*(basemodule->fc_set_on_connect))(&on_connect_for_base);
	(*(basemodule->fc_set_on_disconnect))(&on_disconnect_for_base);
	printf("REST base module address %s\n", basemodule->address);
	return basemodule->address;
}

//received data from base module
void on_message_for_base(void* module, int fd, const char* msg) {
	//process data from (tcp) rest -> json (core)
	//remove http header

	printf("on data: %d %s\n", fd, msg);
	JSON *datatosend = json_new(NULL);
	if (map_get(skttype, &fd) == NULL) {//data from client: transfer GET to json msg
		/*fake map message via ep_name, ep_type,
		 * {"status":5,"msg":"{\"ep_query\":[\"ep_name = 'source'\",\"ep_type = 'src'\",\"msg_hash = '12679001002523692768'\"],\"ep_metadata\":{\"ep_id\":\"GxTeGk2gtd\",\"ep_name\":\"sink\",\"ep_description\":\"example snk endpoint\",\"ep_type\":\"snk\",\"message\":\"12679001002523692768\",\"blocking\":0}}"}
		 */

		JSON* msg1 = json_new(NULL);
		Array* query = array_new(ELEM_TYPE_STR);
		//msg query
		char* ep_name;
		char* ep_type;
		char type[4];
		memcpy(type, msg, 3);
		type[3] = '\0';
		if (strcmp(type, "GET") == 0) {	//get method
			//extract parameter and form the map message
			printf("get\n");
			//ep_name
			ep_name = strstr(strdup(msg), "ep_name=");
			ep_name = strchr(ep_name, '=');
			ep_name++; //remote =
			if (strchr(ep_name, '&') != NULL) {	//final parameter
				ep_name[strlen(ep_name) - strlen(strchr(ep_name, '&'))] = '\0';
			} else {
				//ep_name[strlen(ep_name) - strlen(strchr(ep_name, ' '))] = '\0';
			}

			//ep_type
			ep_type = strstr(strdup(msg), "ep_type=");
			ep_type = strchr(ep_type, '=');
			ep_type++; //remote =
			if (strchr(ep_type, '&') != NULL) {	//final parameter
				ep_type[strlen(ep_type) - strlen(strchr(ep_type, '&'))] = '\0';
			} else {
				ep_type[3] = '\0';
			}

		} else if (strcmp(type, "POS") == 0) {
			printf("post\n");
		}
		char bufepname[50];
		memset(bufepname, '\0', strlen(bufepname));
		sprintf(bufepname, "ep_name = '%s'", ep_name);
		array_add(query, bufepname);

		char bufeptype[50];
		memset(bufeptype, '\0', strlen(bufeptype));
		sprintf(bufeptype, "ep_type = '%s'", ep_type);
		array_add(query, bufeptype);
		array_add(query, "msg_hash = '9338696174743756141'");
		json_set_array(msg1, "ep_query", query);
		json_set_array(msg1, "cpt_query", array_new(ELEM_TYPE_STR));

		//metadata
		JSON *metadata = json_new(NULL);
		json_set_str(metadata, "ep_id", "hN9SEJ2LJB");
		json_set_str(metadata, "ep_name", "ep_rest");
		json_set_str(metadata, "ep_description",
				"fake endpoint from rest module");
		if (strcmp(ep_type, "src") == 0) {
			json_set_str(metadata, "ep_type", "snk");
		} else if (strcmp(ep_type, "sink") == 0) {
			json_set_str(metadata, "ep_type", "src");
		} else if (strcmp(ep_type, "request") == 0) {
			json_set_str(metadata, "ep_type", "rep");
		} else if (strcmp(ep_type, "response") == 0) {
			json_set_str(metadata, "ep_type", "req");
		}

		json_set_str(metadata, "message", "11005971662832010848");
		json_set_int(metadata, "blocking", 0);
		json_set_json(msg1, "ep_metadata", metadata);
		json_set_int(datatosend, "status", 5);
		json_set_str(datatosend, "msg", json_to_str(msg1));

	} else {	//data from server, remove header get the last \r\n part

	}

	printf("%s\n", json_to_str(datatosend));

	//send message to core
	(*on_data_handler)(this, fd, json_to_str(datatosend));
}
void on_connect_for_base(void* module, int fd) {
	//take care of the protocol here
	//(*on_connect_handler)(this, fd);

	char  fd_str[20];
	_rest_peer* peer = NULL;
	printf("on_connect_for_base \n");
	sprintf(fd_str, "%d", fd);
	peer = (_rest_peer*)map_get(conn_table, fd_str);
}
void on_disconnect_for_base(void* module, int fd) {
	(*on_disconnect_handler)(this, fd);
}

/*
 * initiates a connection as a client to the server
 * returns connection id / socket
 */
int com_connect(const char *addr) {

	JSON* connect_json = NULL;
	char* host         = NULL;
	int   client       = 1;
	int   fd           = 0;

	_rest_peer* peer   = NULL;

	connect_json = json_new(addr);
	host         = json_get_str(connect_json, "host");
	client       = json_get_int(connect_json, "client");
	/* set default value to be client  mode */
	if(client <=0 )
		client = 1;

	/* connect with base module */
	fd           = (*(basemodule->fc_connect))(host);
	if(fd <= 0)
		goto final;

	/* declare new peer */
	peer = rest_new(host, fd, client);
	if(on_connect_handler && peer != NULL)
	{
		//if (peer -> client == 0)
		//	peer->connected = 1;

		map_update(conn_table,  peer->fd_str, (void*)peer );
		map_insert(skttype, &fd, "c"); //rest as client

		(*on_connect_handler)(this, fd);

		printf("peer connected: %d\n", fd);
	}

	final:{
		free(host);
		json_free(connect_json);

		return fd;
	}
}

int com_connection_close(int conn) {
	return (*(basemodule->fc_connection_close))(conn);
}

// TODO: rewrite com_send_data using com_send
int com_send(int conn, const void *data, unsigned int size)
{
	int ret_value;
	char* data_str = (char* )malloc(size+1);
	data_str[size]='\0';
	memcpy(data_str, data, size);
	ret_value = com_send_data(conn, data_str);
	free(data_str);
	return ret_value;
}

int com_send_data(int conn, const char *data) {

	char  fd_str[20];
	_rest_peer* peer = NULL;
	JSON* msg_json = NULL;
	int msg_status = -1;

	sprintf(fd_str, "%d", conn);
	peer = (_rest_peer*)map_get(conn_table, fd_str);

	/* peer was not found */
	if( peer == NULL)
	{
		return -1;
	}

	msg_json = json_new(data);
	msg_status = json_get_int(msg_json, "status");
	/* check if it's first message
	 * MSG_MAP is 5
	 * MSG_MAP_ACK is 6 */
	if(msg_status == 5 && peer->connected == 0 && peer->client)
	{
		JSON* ack_json = json_new(NULL);
		JSON* msg_json = json_new(NULL);
		JSON* ep_md_json = json_new(NULL);

		json_set_int(msg_json, "ack_code", 0);
		json_set_str(ep_md_json, "ep_id", "rest");
		json_set_json(msg_json, "ep_metadata", ep_md_json);
		json_set_int(ack_json, "status", 6);
		json_set_str(ack_json, "msg", json_to_str(msg_json));

		//printf("send back: %s\n\n", json_to_str(ack_json));
		(*on_data_handler)(this, peer->fd,
				json_to_str(ack_json));

		json_free(ep_md_json);
		json_free(msg_json);
		json_free(ack_json);
		return 0;
	}

	//if(peer->connected == 0)
	//	return 0;

	printf( "rest send data: %d: %s\n\n", conn, data);
	char datatosend[520];
	JSON* json = json_new(data);
	if (json_get_int(json, "status") == 9) {
		if (map_get(skttype, &conn) == NULL) {	//sending to client
			char* httpheader =
					"HTTP/1.1 200 OK\r\n"
					"Server: Apache\r\n"
					"Content-Type: text/json; charset=UTF-8\r\n\r\n";
			strcpy(datatosend, httpheader);
			strcat(datatosend, data);
			strcat(datatosend, "\n\n\0");

		} else { //sending to server as GET request
			printf("REST SEND TO SERVER \r\n");
		}
		//send data
		(*basemodule->fc_send_data)(conn, datatosend);
		close(conn); //connectionless
		//on_disconnect_for_base(this,conn);
	}
	return 1;

}

int com_set_on_data(void (*handler)(void*, int, const char*)) {
	on_data_handler = handler;
	return (on_data_handler != NULL);
}

int com_set_on_connect(void (*handler)(void*, int)) {
	on_connect_handler = handler;
	return (on_connect_handler != NULL);
}

int com_set_on_disconnect(void (*handler)(void*, int)) {
	on_disconnect_handler = handler;
	return (on_disconnect_handler != NULL);
}

int com_is_valid_address(const char* full_address) {
	return (*(basemodule->fc_is_valid_address))(full_address);
}

int com_is_bridge(void) {
	return 0;
}

