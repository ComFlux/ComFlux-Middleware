/*
 * json_parser.h
 *
 *  Created on: 21 Mar 2016
 *      Author: Raluca Diaconu
 */

#ifndef ENDPOINT_H_
#define ENDPOINT_H_
/*
 * this describes endpoint related definitions functions and flags
 */
#include "endpoint_base.h"
#include "array.h"
#include "json.h"
#include "message.h"

#include "../module_wrappers/access_wrapper.h"
//#include "state.h"
#include "com_wrapper.h"

struct _STATE;

typedef struct _LOCAL_EP{
	ENDPOINT *ep;
	char *id;
	//COMPONENT *component;

	/* all enpoints have an array of conn (fd) mappings */
	Array *mappings_states;

	Array *messages;
	Array *responses;

	JSON * msg_schema;
	JSON * resp_schema;

	int flag;

	int queuing;/* wether it stores incoming messages or not */

	int is_default;
	int is_visible;

	int fifo;
	char fifo_name[20];

	/* com modules for the outside world */
	Array* com_modules;

	/* an array of strings */
	Array* filters;

	/* handler called for messages from the app.
	 * external msgs: ep->handler.
	 */
	void (*fromapp_handler)(MESSAGE*);

	/* when msg is valid for module fcs or internal to the core
	 * relevant when an ep is invoked from exterior
	 * the def handler should be sent-to-app
	 * if its different add that fc as a handler*/
	//void (*handler)(MESSAGE*);

}LOCAL_EP;

/*
 * instantiates an endpoint in the core.
 * creates a message schema from the message description and ep type.
 */
LOCAL_EP* ep_local_new(JSON* json, void(* handler)(MESSAGE*));

/*
 * safe way to remove and dealocate an endpoint;
 * unmaps all
 */
int ep_local_remove(LOCAL_EP *lep);

/*
 * frees the memory of an endpoint;
 * for a safe removal of the endpoint think about using @ep_local_remove
 */
void ep_local_free(LOCAL_EP *lep);



/*
 * checks if the ep matches the type
 * @return: 1 yes, 0 no
 */
int ep_match_type(LOCAL_EP *lep, int type);

/*
 * checks if the ep matches msg (and resp) hash(es)
 * @return: 1 yes, 0 no
 */
int ep_match_hash(LOCAL_EP *lep, const char *msg_hash, const char *resp_hash);


/*
 * checks if the ep matches all desc in the json
 * @return: 1 yes, 0 no
 */
int ep_match_desc(LOCAL_EP *lep, const char *rep_desc);


JSON *ep_to_json(ENDPOINT* endpoint);

JSON *ep_local_to_json(LOCAL_EP *lep);

char *ep_local_to_str(LOCAL_EP *lep);

/*
 * add @ep_remote to the list of connections if src &| snk
 * set connection otherwise
 * does not perform verifications
 */
int ep_map(LOCAL_EP *lep, struct _STATE* state);


/*
 * unmap is called by the current component API
 */
void ep_unmap_send(LOCAL_EP *lep, struct _STATE* state);
/*
 * unmap was called by the peer, the other side
 */
void ep_unmap_recv(LOCAL_EP *lep,  struct _STATE* state);
/*
 * connection was cut unexpectedly or
 * the unmap protocol succeeded
 */
void ep_unmap_final(LOCAL_EP *lep, struct _STATE* state);

/*
 * resolves the addr to a state(module+connection)
 * then calls ep_unmap_send
 */
void ep_unmap_addr(LOCAL_EP *lep, const char* addr);

/*
 * unmap from all peers
 * calls ep_unmap_send for all states in ep->mappings
 */
void ep_unmap_all(LOCAL_EP *lep);


LOCAL_EP * endpoint_query(JSON* query_json);

/*com modules and message passing */

/* send message on all com modules */

/* send message on a specific com modules */
//int ep_module_send_message(ENDPOINT *ep, COM_MODULE* module, const char* msg);

/* send json message on all com modules */
int ep_send_json(LOCAL_EP *lep, JSON* json, const char* msg_id, int status);

/* send json message on a specific com modules */
//int ep_module_send_json(ENDPOINT *ep, COM_MODULE* module, JSON* json);



/*
 * TODO: make ep hash functions better
 */
//unsigned long ep_hash_long(ENDPOINT* ep);
//char* ep_hash_str(ENDPOINT* ep);


/*
 * validates send type and schema
 * TODO: this fc may replace the previous
 */
//int ep_validate_send_msg(ENDPOINT* ep, JSON *msg);

/*
 * validates received type and schema
 * TODO: this fc may replace the previous
 */
//int ep_validate_received_msg(ENDPOINT* ep, JSON *msg);



/*
 * if mapped, send th message to all connections
 * @ret: the number of connections that the message has been sent
 */
//int ep_send_message(ENDPOINT *ep, char* msg);
//int endpoint_reply(ENDPOINT *ep,int msg_id, WJElement json);

/*
 * finds a matching endpoint in the list of all registered endpoints.
 * ENDPOINT* ep_find_match(char *msg);
 */

//unsigned long hash_long(unsigned char *str);
//char* hash_str(unsigned char *str);

//int ep_register_connection(ENDPOINT *ep, int conn);

/*
 * container functions
 */

HashMap *endpoints; 	/* id->all endpoints */
HashMap *locales;	 	/* id->remote_endpoints */
int eps_init();

#endif /* ENDPOINT_H_ */
