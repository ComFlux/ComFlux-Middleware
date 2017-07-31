/*
 * endpoint.c
 *
 *  Created on: 18 Mar 2016
 *      Author: Raluca Diaconu
 */

#include "endpoint.h"

#include "middleware.h"

#include "json.h"
#include <utils.h>
#include <hashmap.h>

#include <stdlib.h>
#include <string.h>
#include <file.h>


HashMap *endpoints = NULL; /* id -> ep*/

/* helper fc definitions */

/* sends data about the endpoint to the core */
int endpoint_register(ENDPOINT *ep);


/* api functionality */

ENDPOINT* endpoint_new( const char* name, const char *description, int type,
						const char *msg_str, const char *resp_str,
						void (*callback_function)(MESSAGE*),
						const char* id)
{
	/* basic param init */
	ENDPOINT *ep = endpoint_init(name, description, type,
								 msg_str, resp_str,
								callback_function, id);

	if(ep == NULL)
		return NULL;

    // If no handler was given, queue messages on the core and wait for blocking calls to
    // endpoint_get_message to retrieve them. Otherwise, have the core immediately forward
    // the messages to the API so it may call the handler.
	if (ep->handler == NULL) {
		ep->queuing = 1;
	} else {
		ep->queuing = 0;
	}

	if (endpoints == NULL)
		endpoints = map_new(KEY_TYPE_STR);
	map_insert(endpoints, ep->id, ep);

	endpoint_register(ep);
	return ep;
}

ENDPOINT* endpoint_new_src(const char* name, const char *description, const char *msg_str)
{
	return endpoint_new(name, description, EP_SRC,
			 msg_str, NULL,
			NULL, NULL);
}

ENDPOINT* endpoint_new_snk(const char* name, const char *description, const char *msg_str,
                           void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_SNK,
			 msg_str, NULL,
			callback_function, NULL);
}

ENDPOINT* endpoint_new_ss(const char* name, const char *description, const char *msg_str,
                          void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_SS,
			 msg_str, NULL,
			callback_function, NULL);
}

ENDPOINT* endpoint_new_req(const char* name, const char *description,
                           const char *msg_str, const char *resp_str,
						   void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_REQ,
			 msg_str, resp_str,
			callback_function, NULL);
}

ENDPOINT* endpoint_new_resp(const char* name, const char *description,
                            const char *msg_str, const char *resp_str,
		                    void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_RESP,
			 msg_str, resp_str,
			callback_function, NULL);
}

ENDPOINT* endpoint_new_req_p(const char* name, const char *description,
                             const char *msg_str, const char *resp_str,
                             void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_REQ_P,
			 msg_str, resp_str,
			callback_function, NULL);
}

ENDPOINT* endpoint_new_resp_p(const char* name, const char *description,
                              const char *msg_str, const char *resp_str,
                              void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_RESP_P,
			 msg_str, resp_str,
			callback_function, NULL);
}

ENDPOINT* endpoint_new_rr(const char* name, const char *description,
                          const char *msg_str, const char *resp_str,
                          void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_RR,
			 msg_str, resp_str,
			callback_function, NULL);
}

ENDPOINT* endpoint_new_rr_p(const char* name, const char *description,
                            const char *msg_str, const char *resp_str,
                            void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_RR_P,
			 msg_str, resp_str,
			callback_function, NULL);
}

ENDPOINT* endpoint_new_stream_src(const char* name, const char *description)
{
	return endpoint_new(name, description, EP_STR_SRC,
			NULL, NULL,
			NULL, NULL);
}

ENDPOINT* endpoint_new_stream_snk(const char* name, const char *description,
                           void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_STR_SNK,
			NULL, NULL,
			callback_function, NULL);
}



/* file schema declaration */

ENDPOINT* endpoint_new_src_file(const char* name, const char *description, const char *msg_fn)
{
	return endpoint_new(name, description, EP_SRC,
			text_load_from_file(msg_fn), NULL,
			NULL, NULL);
}

ENDPOINT* endpoint_new_snk_file(const char* name, const char *description, const char *msg_fn,
                           void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_SNK,
			text_load_from_file(msg_fn), NULL,
			callback_function, NULL);
}

ENDPOINT* endpoint_new_ss_file(const char* name, const char *description, const char *msg_fn,
                          void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_SS,
			text_load_from_file(msg_fn), NULL,
			callback_function, NULL);
}

ENDPOINT* endpoint_new_req_file(const char* name, const char *description,
                           const char *msg_fn, const char *resp_fn,
						   void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_REQ,
			text_load_from_file(msg_fn), text_load_from_file(resp_fn),
			callback_function, NULL);
}

ENDPOINT* endpoint_new_resp_file(const char* name, const char *description,
                            const char *msg_fn, const char *resp_fn,
		                    void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_RESP,
			text_load_from_file(msg_fn), text_load_from_file(resp_fn),
			callback_function, NULL);
}

ENDPOINT* endpoint_new_req_p_file(const char* name, const char *description,
                             const char *msg_fn, const char *resp_fn,
                             void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_REQ_P,
			text_load_from_file(msg_fn), text_load_from_file(resp_fn),
			callback_function, NULL);
}

ENDPOINT* endpoint_new_resp_p_file(const char* name, const char *description,
                              const char *msg_fn, const char *resp_fn,
                              void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_RESP_P,
			text_load_from_file(msg_fn), text_load_from_file(resp_fn),
			callback_function, NULL);
}

ENDPOINT* endpoint_new_rr_file(const char* name, const char *description,
                          const char *msg_fn, const char *resp_fn,
                          void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_RR,
			text_load_from_file(msg_fn), text_load_from_file(resp_fn),
			 callback_function, NULL);
}

ENDPOINT* endpoint_new_rr_p_file(const char* name, const char *description,
                            const char *msg_fn, const char *resp_fn,
                            void (*callback_function)(MESSAGE*))
{
	return endpoint_new(name, description, EP_RR_P,
			text_load_from_file(msg_fn), text_load_from_file(resp_fn),
			callback_function, NULL);
}


/* other functions */

int endpoint_register(ENDPOINT *ep)
{
	char*    ep_str = NULL;
	//MESSAGE* reg_msg = NULL;

	MESSAGE* ret_msg = NULL;
	JSON*    ret_json = NULL;
	int      return_value = -1;

	ep_str = ep_to_str(ep);
	//reg_msg = message_new(ep_str, MSG_CMD);

	//reg_msg->ep = ep;
    char* result = (char*) mw_call_module_function_blocking(
            NULL,
            "core", "register_endpoint", "int",
            ep_str, NULL);

	if (result == NULL)
		goto final;

	ret_msg = message_parse(result);
	ret_json = ret_msg->_msg_json;
	return_value = json_get_int(ret_json, "return_value");

	final:
	{
		json_free(ret_json);
		message_free(ret_msg);
		//message_free(reg_msg);
		//TODO: free(ep_str);

		return return_value;
	}
}


void endpoint_unregister(ENDPOINT *ep)
{
	//MESSAGE *reg_msg = message_new(ep_to_str(ep), MSG_REM_EP);
	//reg_msg->ep = ep;
    mw_call_module_function(
            NULL,
            "core", "remove_endpoint", "void",
            ep->id, NULL);
}

void endpoint_remove(ENDPOINT* endpoint)
{
	endpoint_unregister(endpoint);
	map_remove(endpoints, endpoint->id);
	endpoint_free(endpoint);
}

void endpoint_send_message(ENDPOINT* endpoint, const char* msg)
{
	MESSAGE *src_msg = message_new(msg, MSG_MSG);
	src_msg->ep = endpoint;

	//endpoint_send(ep, src_msg);
	char* msg_str = message_to_str(src_msg);
    mw_call_module_function(
            NULL,
            "core", "ep_send_message", "void",
            endpoint->id, src_msg->msg_id, msg_str, NULL);

	free(msg_str);
	message_free(src_msg);
}

void endpoint_send_message_json(ENDPOINT* endpoint, JSON* msg_json)
{
	MESSAGE *src_msg = message_new_json(msg_json, MSG_MSG);
	src_msg->ep = endpoint;

	//endpoint_send(ep, src_msg);
	char* msg_str = message_to_str(src_msg);
    mw_call_module_function(
            NULL,
            "core", "ep_send_message", "void",
            endpoint->id, src_msg->msg_id, msg_str, NULL);

	free(msg_str);
	message_free(src_msg);
}

void endpoint_start_stream(ENDPOINT* endpoint)
{
    mw_call_module_function(
            NULL,
            "core", "ep_stream_start", "void",
            endpoint->id, NULL);
}

void endpoint_stop_stream(ENDPOINT* endpoint)
{
    mw_call_module_function(
            NULL,
            "core", "ep_stream_stop", "void",
            endpoint->id, NULL);
}

void endpoint_send_stream(ENDPOINT* endpoint, char* msg)
{
    mw_call_module_function(
            NULL,
            "core", "ep_stream_send", "void",
            endpoint->id, msg, NULL);
}

char* endpoint_send_request(ENDPOINT* endpoint, const char* msg)
{
	const char* _msg;
	if (msg == NULL)
		_msg = msg;
	else
		_msg = "";

	MESSAGE* req_msg = message_new(_msg, MSG_REQ);
	req_msg->ep = endpoint;

	char* msg_str = message_to_str(req_msg);

    mw_call_module_function(
            req_msg->msg_id,
            "core", "ep_send_request", "void",
            endpoint->id, req_msg->msg_id, msg_str, NULL);

	char* msg_id = strdup_null(req_msg->msg_id);

	free(msg_str);
	message_free(req_msg);

	return msg_id;
}

MESSAGE* endpoint_send_request_blocking(ENDPOINT* endpoint, const char* msg)
{
	//if (!endpoint->queuing) {}

	const char* _msg;
	if (msg != NULL)
		_msg = msg;
	else
		_msg = "";

	MESSAGE* req_msg = message_new(_msg, MSG_REQ);
	req_msg->ep = endpoint;

	char* msg_str = message_to_str(req_msg);

	char* result = (char*) mw_call_module_function_blocking(
			req_msg->msg_id,
			"core", "ep_send_request", "void",
			endpoint->id, req_msg->msg_id, msg_str, NULL);

	MESSAGE* resp = message_parse(result);

	free(result);
	free(msg_str);
	message_free(req_msg);

	return resp;
}

void endpoint_send_response(ENDPOINT* endpoint, const char* req_id, const char* msg)
{
	MESSAGE *resp_msg = message_new(msg, MSG_RESP_NEXT);
	resp_msg->msg_id = strdup(req_id);
	resp_msg->ep = endpoint;

    char* msg_str = message_to_str(resp_msg);

    mw_call_module_function(
            resp_msg->msg_id,
            "core", "ep_send_response", "void",
            endpoint->id, resp_msg->msg_id, msg_str, NULL);

    free(msg_str);
    message_free(resp_msg);
}

void endpoint_send_last_response(ENDPOINT* endpoint, const char* req_id, const char* msg)
{
	MESSAGE * resp_msg;
	JSON* new_msg = json_new(NULL);
	json_set_str(new_msg, "ep_name", endpoint->name);
	json_set_json(new_msg, "response", json_new(msg));

	//resp_msg = message_new(ep, req_id, msg, MSG_RESP_LAST); /* 1 = the last message */
	resp_msg = message_new(msg, MSG_RESP_LAST);
	resp_msg->msg_id = strdup_null(req_id);
	resp_msg->ep = endpoint;

    mw_call_module_function(
            req_id,
            "core", "ep_send_response", "void",
            endpoint->id, req_id,
            message_to_str(resp_msg), NULL);

	json_free(new_msg);
	message_free(resp_msg);
}

/* internal */
void endpoint_send(ENDPOINT* endpoint, MESSAGE* msg)
{
    mw_call_module_function(NULL, "core", "ep_send_message", "void", endpoint, message_to_str(msg), NULL);
}

/* ask the core if there are queued messages for @ep */
int endpoint_more_messages(ENDPOINT* endpoint)
{
	char* result = (char*) mw_call_module_function_blocking(
			NULL,
			"core", "ep_more_messages", "int",
			endpoint->id, NULL);

	if (result == NULL)
		return -1;

	MESSAGE* msg = message_parse(result);

	free(result);

	return json_get_int(msg->_msg_json, "return_value");
}

/* ask the core if there are queued requests for @ep */
int endpoint_more_requests(ENDPOINT* endpoint)
{
	char* result = (char*) mw_call_module_function_blocking(
			NULL,
			"core", "ep_more_requests", "int",
			endpoint->id, NULL);

	if (result == NULL)
		return -1;

	MESSAGE* msg = message_parse(result);

	free(result);

	return json_get_int(msg->_msg_json, "return_value");
}

/* ask the core if there are queued responses for @ep */
int endpoint_more_responses(ENDPOINT* endpoint, const char* req_id)
{
	char* result = (char*) mw_call_module_function_blocking(
			NULL,
			"core", "ep_more_responses", "int",
			endpoint->id, req_id, NULL);

	if (result == NULL)
		return -1;

	MESSAGE* msg = message_parse(result);

	free(result);

	return json_get_int(msg->_msg_json, "return_value");
}

/* receive queued message from the core */
MESSAGE* endpoint_fetch_message(ENDPOINT* endpoint)
{
	char* result = (char*) mw_call_module_function_blocking(
			NULL,
			"core", "ep_receive_message", "message",
			endpoint->id, NULL);

	MESSAGE* msg = message_parse(result);

	free(result);

	return msg;
}

/* receive queued request from the core */
MESSAGE* endpoint_fetch_request(ENDPOINT* endpoint)
{
	char* result = mw_call_module_function_blocking(
			NULL,
			"core", "ep_receive_request", "message",
			endpoint->id, NULL);

	if (result == NULL)
		return NULL;

	MESSAGE* req_msg = message_parse(result);

	free(result);

	return req_msg;
}

/* receive queued response from the core */
MESSAGE* endpoint_fetch_response(ENDPOINT* endpoint, const char* req_id)
{
	char* result = (char*) mw_call_module_function_blocking(
			req_id,
			"core", "ep_receive_response", "message",
			endpoint->id, req_id, NULL);

	if (result == NULL)
		return NULL;

	MESSAGE* resp_msg = message_parse(result);

	free(result);

	return resp_msg;
}


void endpoint_add_filter(ENDPOINT* endpoint, const char* filter)
{
    mw_call_module_function(
            NULL,
            "core", "ep_add_filter", "void",
            endpoint->id, filter, NULL);
}

void endpoint_set_filters(ENDPOINT* endpoint, const char* filter_json)
{
    mw_call_module_function(
            NULL,
            "core", "ep_reset_filter", "void",
            endpoint->id, filter_json, NULL);
}

void endpoint_set_accesss(ENDPOINT* endpoint, const char* subject)
{
    mw_call_module_function(
            NULL,
            "core", "ep_set_access", "void",
            endpoint->id, subject, NULL);
}

void endpoint_reset_accesss(ENDPOINT* endpoint, const char* subject)
{
    mw_call_module_function(
            NULL,
            "core", "ep_reset_access", "void",
            endpoint->id, subject, NULL);
}

JSON *ep_to_json(ENDPOINT* endpoint)
{
	if (!endpoint)
		return NULL;

	JSON *ep_json = json_new(NULL);
	json_set_str(ep_json, "ep_id", endpoint->id);
	json_set_str(ep_json, "ep_name", endpoint->name);
	json_set_str(ep_json, "ep_description", endpoint->description);
	char* type = get_ep_type_str(endpoint->type);
	json_set_str(ep_json, "ep_type", type);
	free(type);
	if(endpoint->msg)
	{
		JSON* msg_json = json_new(endpoint->msg);
		json_set_json(ep_json, "message", msg_json);
		json_free(msg_json);
	}
	if(endpoint->resp)
	{
		JSON* resp_json = json_new(endpoint->resp);
		json_set_json(ep_json, "response", resp_json);
		json_free(resp_json);
	}

	json_set_int(ep_json, "blocking", (int)endpoint->queuing);//TODO

	return ep_json;
}

char *ep_to_str(ENDPOINT* endpoint)
{
	if (!endpoint)
		return NULL;
	JSON* ep_json = ep_to_json(endpoint);
	char* ep_str = json_to_str(ep_json);
	json_free(ep_json);
	return ep_str;
}

Array* ep_get_all_connections(ENDPOINT* endpoint)
{
	if (endpoint == NULL)
		return NULL;

	/* endpoint update core */
	char* resp = mw_call_module_function_blocking(
			NULL,
			"core", "ep_get_all_connections", "string",
			endpoint->id, NULL);
	MESSAGE* resp_msg = message_parse(resp);
	JSON* resp_json = resp_msg->_msg_json;
	char* return_value = json_get_str(resp_json, "return_value");

	JSON* all_conns_json = json_new(return_value);
	Array* all_conns_array = json_get_jsonarray(all_conns_json, "all_mappings");

	return all_conns_array;
}

/* blah blah fcs */

void endpoint_update_description(ENDPOINT* ep, const char *description)
{
	if(ep != NULL)
	{
		free(ep->description);
		ep->description = strdup(description);

		/* endpoint update core */
        mw_call_module_function(
                NULL,
                "core", "update_description", "void",
                description, NULL);
	}
}

void endpoint_update_resp(ENDPOINT* ep, const char *resp_fn)
{
	if(ep != NULL)
	{
		free(ep->resp);
		ep->resp = text_load_from_file(resp_fn);

		/* endpoint update core */
        mw_call_module_function(
                NULL,
                "core", "update_resp", "void",
                ep->resp, NULL);
	}
}

void endpoint_update_msg(ENDPOINT* ep, const char *msg_fn)
{
	if(ep != NULL)
	{
		free(ep->msg);
		ep->msg = text_load_from_file(msg_fn);

		/* endpoint update core */
        mw_call_module_function(
                NULL,
                "core", "update_msg", "void",
                ep->msg, NULL);
	}
}



/* mapping */

int endpoint_map_to(ENDPOINT* endpoint, const char* address, const char* ep_query, const char* cpt_query)
{
	char* result = NULL;

	MESSAGE* msg = NULL;
	JSON* map_json = NULL;
	int return_value;

	if(endpoint == NULL)
	{
		return -2;
	}

	if(address == NULL)
	{
		return -2;
	}

	if(ep_query == NULL)
		ep_query = "";
	if(cpt_query == NULL)
		cpt_query = "";


	result = (char*) mw_call_module_function_blocking(
			NULL,
			"core", "map", "int",
			endpoint->id, address, ep_query, cpt_query,
			NULL);

	if(result == NULL)
		return -1;

	msg = message_parse(result);
	map_json = msg->_msg_json;
	return_value = json_get_int(map_json, "return_value");


	json_free(map_json);
	message_free(msg);

	return return_value;
}

int endpoint_map_module(ENDPOINT* endpoint, const char* module, const char* address, const char* ep_query, const char* cpt_query)
{
	char* result = NULL;

	MESSAGE* msg = NULL;
	JSON* map_json = NULL;
	int return_value;

	if(endpoint == NULL)
	{
		return -2;
	}

	if(address == NULL || module == NULL)
	{
		return -2;
	}

	if(ep_query == NULL)
		ep_query = "";
	if(cpt_query == NULL)
		cpt_query = "";

	result = (char*) mw_call_module_function_blocking(
			NULL,
			"core", "map_module", "int",
			endpoint->id, module, address, ep_query, cpt_query,
			NULL);

	if(result == NULL)
		return -1;

	msg = message_parse(result);
	map_json = msg->_msg_json;
	return_value = json_get_int(map_json, "return_value");

	json_free(map_json);
	message_free(msg);

	return return_value;
}

void endpoint_map_lookup(ENDPOINT* endpoint, const char* ep_query, const char* cpt_query, int max_maps)
{
	char max_nb_str[10];

	if(endpoint == NULL)
	{
		return;
	}

	if(ep_query == NULL)
		ep_query = "";
	if(cpt_query == NULL)
		cpt_query = "";

	sprintf(max_nb_str, "%d", max_maps);

	mw_call_module_function(
			NULL,
			"core", "map_lookup", "void",
			endpoint->id, ep_query, cpt_query, max_nb_str, NULL);
}


/* unmapping */

int endpoint_unmap_from(ENDPOINT* endpoint, const char* addr)
{
	char* result = (char*) mw_call_module_function_blocking(
			NULL,
			"core", "unmap", "int",
			endpoint->id, addr, NULL);

	if(result == NULL)
		return -1;

	MESSAGE* msg = message_parse(result);
	JSON* map_json = msg->_msg_json;
	int return_value = json_get_int(map_json, "return_value");

	//free(result);
	json_free(map_json);
	message_free(msg);

	return return_value;
}

int endpoint_unmap_connection(ENDPOINT* endpoint, const char* module, int conn)
{
	char conn_str[10];
	sprintf(conn_str, "%d", conn);

	char* result = (char*) mw_call_module_function_blocking(
			NULL,
			"core", "unmap_connection", "int",
			endpoint->id, module, conn_str, NULL);

	if(result == NULL)
		return -1;

	MESSAGE* msg = message_parse(result);
	JSON* map_json = msg->_msg_json;
	int return_value = json_get_int(map_json, "return_value");

	//free(result);
	json_free(map_json);
	message_free(msg);

	return return_value;
}

int endpoint_unmap_all(ENDPOINT* endpoint)
{
	char* result = (char*) mw_call_module_function_blocking(
			NULL,
			"core", "unmap_all", "int",
			endpoint->id, NULL);

	MESSAGE* msg = message_parse(result);
	JSON* msg_json = msg->_msg_json;
	int return_value = json_get_int(msg_json, "return_value");

	return return_value;
}

/* TODO: divert */

int endpoint_divert(ENDPOINT *ep, char *ep_id_from, char* addr, char *ep_id_to)
{
	char* result = (char*) mw_call_module_function_blocking(
			NULL,
			"core", "divert", "string",
			ep->id, ep_id_from, addr, ep_id_to, NULL);

	if (result == NULL)
		return -1;

	MESSAGE* msg = message_parse(result);
	JSON* divert_json = msg->_msg_json;
	int return_value = json_get_int(divert_json, "return_value");

	//free(result);
	json_free(divert_json);
	message_free(msg);

	return return_value;
}

