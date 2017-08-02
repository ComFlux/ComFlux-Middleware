/*
 * core_functions.h
 *
 *  Created on: 3 Apr 2016
 *      Author: rad
 */

#ifndef CORE_FUNCTIONS_H_
#define CORE_MODULE_H_


#include "hashmap.h"
#include "endpoint.h"

/*
 * Basic functionalities of the core
 * These are exposed to
 *  - the apilib in core_module_api.h
 *  - the exterior with endpoints in default_eps.h
 */

/* returns an id to use in the function map.
 * to return the id, allocates memory on heap and returns a pointer */
char* _core_get_id(char id[50], const char * module_id, const char * function_id, const char * return_type);

/*
 * stores info about the app's endpoint
 * the json must contain char * ep_name, char * addr, int port
 * atm functionality exposed to the lib only
 */
int core_register_endpoint(const char* json);

/* remove endpoint */
void core_remove_endpoint(const char* ep_id);

/* maps ep with name ep->name to addr, port */

int core_map(LOCAL_EP* lep,
		COM_MODULE* com_module, const char* addr,
		JSON* ep_query, JSON* cpt_query);

int core_map_all_modules(LOCAL_EP* lep,
		const char* addr,
		JSON* ep_query, JSON* cpt_query);

void core_map_lookup(LOCAL_EP* lep, JSON* ep_query, JSON* cpt_query, int max_maps);

/* unmaps ep with ep->name from all connections */
int core_unmap(LOCAL_EP* lep, const char *addr);

/* unmaps ep with ep->name from a specific connections */
int core_unmap_connection(LOCAL_EP* lep, COM_MODULE* module, int conn);

int core_unmap_all(LOCAL_EP* lep);

/* TODO */
int core_divert(LOCAL_EP* lep, const char *ep_id_from, const char* addr);

/* ep communications
 * This is th function that decides to send messages to an ep or not.
 * Access control and filtering should be added here.
 */
int core_ep_send_message(LOCAL_EP* lep, const char* msg_id, const char* msg);

int core_ep_send_request(LOCAL_EP* lep, const char* req_id, const char* msg);

int core_ep_send_response(LOCAL_EP* lep, const char* req_id, const char* msg);

int core_ep_more_messages(LOCAL_EP* lep);

int core_ep_more_requests(LOCAL_EP* lep);

int core_ep_more_responses(LOCAL_EP* lep, const char* req_id);

MESSAGE* core_ep_fetch_message(LOCAL_EP* lep);

MESSAGE* core_ep_fetch_request(LOCAL_EP* lep);

MESSAGE* core_ep_fetch_response(LOCAL_EP* lep, const char* req_id);

void core_ep_stream_start(LOCAL_EP* lep);

void core_ep_stream_stop(LOCAL_EP* lep);

void core_ep_stream_send(LOCAL_EP* lep, const char* msg);

/* own manifest */
int core_add_manifest(const char* msg);

char* core_get_manifest(void);

/* rdc */
int  core_add_rdc(COM_MODULE* module, const char *addr);

void core_rdc_register(COM_MODULE *com_module, const char *addr);

void core_rdc_unregister(const char *addr);

/* ep filters and access */
void core_add_filter(LOCAL_EP* lep, const char *filter);

void core_reset_filter(LOCAL_EP* lep, Array* new_filters);

void core_ep_set_access(LOCAL_EP* lep, const char* subject);

void core_ep_reset_access(LOCAL_EP* lep, const char* subject);

/* connections */
char* core_ep_get_all_connections(LOCAL_EP* lep);

char* core_get_remote_manifest(COM_MODULE* module, int conn);

/* termination */
void core_terminate();


/*********************
 *  modules
 *********************/

/* load a com module */
int core_load_com_module(const char* path, const char* cfgfile);

/* load access module */
int core_load_access_module(const char* path, const char* cfgfile);


/* initialises this module */
//TODO


#endif /* CORE_FUNCTIONS_H_ */
