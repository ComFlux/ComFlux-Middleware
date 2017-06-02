/*
 * core_module_api.h
 *
 *  Created on: 13 Aug 2016
 *      Author: rad
 */

#ifndef CORE_CORE_MODULE_API_H_
#define CORE_CORE_MODULE_API_H_

#include "array.h"
#include "message.h"

/*
 * Wrapper for the functionality in core_module
 * These can be called via
 *  - function_call by the lib
 */


/* initialises this module */
int _core_init();


/*
 * stores info about the app's endpoint
 * the json must contain char * ep_name, char * addr, int port
 * atm functionality exposed to the lib only
 */
int core_register_endpoint_array(Array* argv);

/* remove endpoint */
void core_remove_endpoint_array(Array* argv);


/* maps ep with name ep->name to addr, port */
int core_map_all_modules_array(Array* argv);

int core_map_module_array(Array* argv);

void core_map_lookup_array(Array* argv);

/* unmaps ep with ep->name from all connections */
int core_unmap_array(Array* argv);

int core_unmap_all_array(Array* argv);

/* TODO*/
int core_divert_array(Array* argv);

/* ep communications
 * This is th function that decides to send messages to an ep or not.
 * Access control and filtering should be added here.
 */
int core_ep_send_message_array(Array* argv);


int core_ep_send_request_array(Array* argv);

int core_ep_send_response_array(Array* argv);

int core_ep_more_messages_array(Array* argv);

int core_ep_more_requests_array(Array* argv);

int core_ep_more_responses_array(Array* argv);

MESSAGE* core_ep_fetch_message_array(Array* argv);

MESSAGE* core_ep_fetch_request_array(Array* argv);

MESSAGE* core_ep_fetch_response_array(Array* argv);

void core_ep_stream_start_array(Array* argv);

void core_ep_stream_stop_array(Array* argv);

void core_ep_stream_send_array(Array* argv);

/* metadta */
int core_add_manifest_array(Array* argv);

char* core_get_manifest_array(Array* argv);

/* rdc */
void core_add_rdc_array(Array* argv);

void core_rdc_register_array(Array* argv);

void core_rdc_unregister_array(Array* argv);

/* endpoint filters and access*/
void core_add_filter_array(Array* argv);

void core_reset_filter_array(Array* argv);

void core_ep_set_access_array(Array* argv);

void core_ep_reset_access_array(Array* argv);

/* connections */
char* core_ep_get_all_connections_array(Array* argv);

char* core_get_remote_metdata_array(Array* argv);

/* termination */
void core_terminate_array(Array* argv);

/* modules */

/* load a com module */
int core_load_com_module_array(Array* argv);

/* load access module */
int core_load_access_module_array(Array* argv);


/*
 * calls one of the functions declared above.
 * checks and assigns the parameters from an array
 */
MESSAGE* _core_call_array(
		const char* module_id,
		const char* fc_name,
		const char* fc_return,
		Array* fc_args);


#endif /* CORE_CORE_MODULE_API_H_ */
