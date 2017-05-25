/*
 * state.h
 *
 *  Created on: 6 Jul 2016
 *      Author: Raluca Diaconu
 */

#ifndef CORE_STATE_H_
#define CORE_STATE_H_


#include <hashmap.h>
#include <endpoint.h>
#include <com_wrapper.h>
#include "../module_wrappers/access_wrapper.h"

/* state error codes */
#define STATE_HELLO_S		 1
#define STATE_HELLO_ACK_S	 2
#define STATE_HELLO_2		 3

#define STATE_AUTH			 4
#define STATE_AUTH_ACK		 5
#define STATE_AUTH_2		 6

#define STATE_MAP			 7
#define STATE_MAP_ACK 		 8
#define STATE_EXT_MSG 		 9
#define STATE_APP_MSG 		 10
#define STATE_FIRST_MSG 	 11


#define STATE_BAD			0

struct _STATE;

typedef struct _BUFFER{
	char* data;
	int size;

	int buffer_state;
	int brackets;

	struct _STATE* state;
}BUFFER;

BUFFER* buffer_new(struct _STATE* state);

void buffer_free(BUFFER* buffer);

void buffer_set(BUFFER* buffer, const char* new_data, int new_start, int new_end);

void buffer_update(BUFFER* buffer, const char* new_data, int new_end);
//size should be fixed?


typedef struct _STATE{
	/* the access module that provided auth for this connection */
	ACCESS_MODULE* access_module;
	Array* tokens;

	unsigned int is_auth	:1;
	unsigned int am_auth	:1;

	unsigned int is_mapped	:1;

	COM_MODULE* module;
	int conn; /* module+con must be unique, key */

	char *addr;
	JSON *cpt_manifest;
	JSON *ep_metadata;

	LOCAL_EP *lep;

	unsigned int state		:5;
	int flag;

	BUFFER* buffer;
	/* on_message handler for each connection */
	void (*on_message)(struct _STATE*, MESSAGE*);

}STATE;


/*******************
 * state instance
 *******************/

STATE* state_new(COM_MODULE* module, int conn, int state);

void state_free(STATE* state);

int state_send_message(STATE* state, MESSAGE* msg);
int state_send_json(STATE* state, const char* id, JSON* json, int status);

const char* state_get_str(int state);


/********************
 * all states container functionality
 ********************/

HashMap *conn_state;

int init_states();

STATE* states_get(COM_MODULE* module, int conn);

int states_set(COM_MODULE* module, int conn, STATE* state);


#endif /* CORE_STATE_H_ */
