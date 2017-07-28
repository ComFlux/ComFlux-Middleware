/*
 * json_parser.c
 *
 *  Created on: 21 Mar 2016
 *	  Author: Raluca Diaconu
 */


#include "endpoint.h"

#include "json_builds.h"
#include "hashmap.h"
#include "state.h"
#include "json_filter.h"
#include <utils.h>

#include <string.h>
#include <stdlib.h>

extern STATE* app_state;

/* in core.c */
extern HashMap *endpoints;
extern HashMap *locales;

/* default handlers for messages coming from other to the local ep */
void ep_default_handler_send_to_app(MESSAGE* msg);
void ep_default_handler_queuing(MESSAGE* msg);
void ep_send_stream_fd();

LOCAL_EP* ep_local_new(JSON *ep_json, void(* from_ext_handler)(MESSAGE*))
{
	JSON* json_data = ep_json; // json_new(json_get_str(ep_json, "msg"));

	/*if (json_validate_ep_def(json_data))
	{
		slog(SLOG_ERROR, SLOG_ERROR, "EP LOCAL: Does not pass general ep schema: %s", json_to_str(ep_json));
		return NULL;
	}*/


	LOCAL_EP *lep = (LOCAL_EP*)malloc(sizeof(LOCAL_EP));
	lep->mappings_states = lep->messages = lep->responses = lep->filters = NULL;

	void(* ep_handler)(MESSAGE*);
	lep->id = strdup_null(json_get_str(json_data, "ep_id"));
	lep->queuing = json_get_int(json_data, "blocking");

	if (from_ext_handler == NULL) // app ep
	{
		if(lep->queuing == 1)
		{
			ep_handler = &ep_default_handler_queuing;
		}
		else
		{
			ep_handler = &ep_default_handler_send_to_app;
		}
	}
	else // default ep
		ep_handler = from_ext_handler;


	lep->msg_schema = json_get_json(json_data, "message");
	lep->resp_schema = json_get_json(json_data, "response");

	char* msg_schema_str = json_to_str(lep->msg_schema);
	char* resp_schema_str = json_to_str(lep->resp_schema);
	char* msg_hash = NULL;
	if(msg_schema_str)
		msg_hash = hash(msg_schema_str);
	char* resp_hash = NULL;
	if(resp_schema_str)
		resp_hash = hash(resp_schema_str);
	free(msg_schema_str);
	free(resp_schema_str);

	lep->ep = endpoint_init(
			json_get_str(json_data, "ep_name"), json_get_str(json_data, "ep_description"),
			get_ep_type_int(json_get_str(json_data, "ep_type")),
			msg_hash, resp_hash,
			ep_handler, lep->id);

	if (lep->ep == NULL)
	{
		ep_local_free(lep);
		return NULL;
	}
	lep->ep->data = lep;
	lep->fifo = 0;

	lep->mappings_states = array_new(ELEM_TYPE_PTR);
	lep->com_modules = array_new(ELEM_TYPE_PTR);

	/* make the arrays of arrays */
	lep->messages = array_new(ELEM_TYPE_PTR);
	lep->responses = array_new(ELEM_TYPE_PTR);

	lep->filters = array_new(ELEM_TYPE_STR);

	lep->is_default = 0; /* is app endpoint */
	lep->is_visible = 1; /* is visible */

	/* add to maps of all eps */
	map_insert(endpoints, lep->id, lep->ep);
	map_insert(locales, lep->id, lep);

	return lep;
}

int ep_local_remove(LOCAL_EP *lep)
{
	if(lep == NULL)
		return 0;

	ep_unmap_all(lep);

	map_remove(locales, lep->ep->id);
	map_remove(endpoints, lep->ep->id);

	ep_local_free(lep);
	return 0;
}

void ep_local_free(LOCAL_EP *lep)
{
	if(lep == NULL)
		return;


	array_free(lep->messages);
	array_free(lep->responses);
	//array_free(lep->com_modules);

	array_free(lep->filters);

	json_free(lep->msg_schema);
	json_free(lep->resp_schema);

	array_free(lep->mappings_states);

	free(lep->id);
	endpoint_free(lep->ep);

	free(lep);
}

void ep_default_handler_send_to_app(MESSAGE* msg)
{
	//printf("here\n");
	//slog(SLOG_INFO, SLOG_INFO, "EP LOCAL: default handler send_to_app: %d: %s", array_size(((LOCAL_EP*)(msg->ep->data))->filters), msg->msg_str);
	//JSON* msg_json = msg->_msg_json;
	//JSON* filter_json = json_new(NULL);
	//json_set_array(filter_json, NULL, ((LOCAL_EP*)(msg->ep->data))->filters);
	//if(json_filter_validate_array(msg_json, ((LOCAL_EP*)(msg->ep->data))->filters))
	{
		state_send_message(app_state, msg);
	}
}

void ep_default_handler_queuing(MESSAGE* msg)
{
	//slog(SLOG_INFO, SLOG_INFO, "EP LOCAL: default handler queuing: %s", msg->msg_str);

	JSON* msg_json = msg->_msg_json;
	if( msg->status == MSG_REQ && (msg->ep->type == EP_RESP || msg->ep->type == EP_RESP_P))
		if(json_filter_validate_array(msg_json, ((LOCAL_EP*)(msg->ep->data))->filters))
			array_add(((LOCAL_EP*)msg->ep->data)->messages, msg);

	if( (msg->status == MSG_RESP_NEXT || msg->status == MSG_RESP_LAST) &&
		(msg->ep->type == EP_REQ || msg->ep->type == EP_REQ_P))
		if(json_filter_validate_array(msg_json, ((LOCAL_EP*)(msg->ep->data))->filters))
			array_add(((LOCAL_EP*)msg->ep->data)->responses, msg);

	if(	msg->status == MSG_MSG &&
		(msg->ep->type == EP_SNK || msg->ep->type == EP_SS))
		if(json_filter_validate_array(msg_json, ((LOCAL_EP*)(msg->ep->data))->filters))
			array_add(((LOCAL_EP*)msg->ep->data)->messages, msg);

}

void ep_send_stream_fd()
{

}

JSON *ep_to_json(ENDPOINT* endpoint)
{
	if (!endpoint)
		return NULL;

	JSON *ep_json = json_new(NULL);
	json_set_str(ep_json, "ep_id", endpoint->id);
	json_set_str(ep_json, "ep_name", endpoint->name);
	json_set_str(ep_json, "ep_description", endpoint->description);
	char* type =  get_ep_type_str(endpoint->type);
	json_set_str(ep_json, "ep_type", type);
	free(type);
	if(endpoint->msg)
		json_set_str(ep_json, "message", endpoint->msg);
	if(endpoint->resp)
		json_set_str(ep_json, "response", endpoint->resp);

	json_set_int(ep_json, "blocking", (int)endpoint->queuing);//TODO

	return ep_json;
}



int ep_match_type(LOCAL_EP *lep, int type)
{
	if(lep->ep->type == EP_SRC && type == EP_SNK) return 1;
	if(lep->ep->type == EP_SNK && type == EP_SRC) return 1;

	if(lep->ep->type == EP_REQ && type == EP_RESP) return 1;
	if(lep->ep->type == EP_RESP && type == EP_REQ) return 1;

	if(lep->ep->type == EP_REQ_P && type == EP_RESP_P) return 1;
	if(lep->ep->type == EP_RESP_P && type == EP_REQ_P) return 1;

	/* TODO: maybe not correct */
	if(lep->ep->type == EP_RR && type == EP_RR) return 1;
	if(lep->ep->type == EP_RR_P && type == EP_RR_P) return 1;
	if(lep->ep->type == EP_SS && type == EP_SS) return 1;

	return 0;
}

int ep_match_hash(LOCAL_EP *lep, const char *msg_hash, const char *resp_hash)
{
	if (lep->ep->resp == NULL && resp_hash == NULL )
	{
		return (strcmp(lep->ep->msg, msg_hash)==0);
	}
	else if (lep->ep->resp != NULL && resp_hash != NULL )
	{
		return (strcmp(lep->ep->msg, msg_hash)==0 &&
				strcmp(lep->ep->resp, resp_hash)==0 );
	}
	else
		return 0;
}


int ep_match_desc(LOCAL_EP *lep, const char *rep_desc)
{
	JSON* query_json = json_new(rep_desc);
	//char *ep_name_from 		  = json_get_str(query_json, "ep_name");
	//char *ep_description_from = json_get_str(query_json, "ep_description");
	int   ep_type_from 		  = get_ep_type_int(json_get_str(query_json, "ep_type"));
	char *msg_hash 			  = json_get_str(query_json, "msg_hash");
	char *resp_hash			  = json_get_str(query_json, "resp_hash");

	if (! ep_match_type(lep, ep_type_from))
		return 0;
	if (! ep_match_hash(lep, msg_hash, resp_hash))
		return 0;

	return 1;
}

#include "manifest.h"
extern COMPONENT* cpt;
JSON *ep_local_to_json(LOCAL_EP *lep)
{
	if (!lep)
		return NULL;

	JSON *lep_json = json_new(NULL);
	json_set_str(lep_json, "ep_name", lep->ep->name);
	json_set_str(lep_json, "ep_description", lep->ep->description);
	json_set_str(lep_json, "ep_type", get_ep_type_str(lep->ep->type));
	if(lep->ep->msg)
		json_set_str(lep_json, "msg_hash", lep->ep->msg);
	if(lep->ep->resp)
		json_set_str(lep_json, "resp_hash", lep->ep->resp);
	//JSON* cpt = md_cpt();


	int i;
	COM_MODULE* com_module;
	Array* com_modules_json_array = array_new(ELEM_TYPE_PTR);
	JSON* com_module_json;
	lep->com_modules = map_get_values(com_modules);
	for (i=0; i<array_size(lep->com_modules); i++)
	{
		com_module = array_get(lep->com_modules, i);
		com_module_json = json_new(NULL);
		json_set_str(com_module_json, "name", com_module->name);
		json_set_str(com_module_json, "address", com_module->address);
		array_add(com_modules_json_array, com_module_json);
	}
	json_set_array(lep_json, "com_modules", com_modules_json_array);

	json_merge(lep_json, cpt->metadata);

	return lep_json;
}

char *ep_local_to_str(LOCAL_EP *lep)
{
	if (!lep)
		return NULL;
	JSON* ep_json = ep_local_to_json(lep);
	char* ep_str = json_to_str(ep_json);
	json_free(ep_json);
	return ep_str;
}

/* actual mapping given given the checks were done before */
int ep_map(LOCAL_EP *ep_local, STATE* state)
{
	if(ep_local==NULL)
	{
		/* if ep was removed recently */
		return EP_NO_EXIST;
	}

	 if(!state || state->conn<=0)
	 {
		 /* this may appear only when conn has stopped meanwhile */
		 return -1;
	 }

	 array_add(ep_local->mappings_states, state);
	 state->lep = ep_local;

	return EP_OK;
}

void ep_unmap_addr(LOCAL_EP* lep, const char* addr)
{
    if (lep == NULL || addr == NULL)
        return;

    int i;
    STATE* peer_;
    for (i=0; i<array_size(lep->mappings_states); i++)
    {
    	peer_ = array_get(lep->mappings_states, i);
    	if (peer_->lep == NULL || peer_->lep->id == NULL || peer_->addr == NULL)
    	{
    		// this case should not occur
    	    continue;
    	}
    	if(!strcmp(addr, peer_->addr) && !strcmp(peer_->lep->id, lep->id))
    	{
    		ep_unmap_send(lep, peer_);
    	}

    }
}


void ep_unmap_send(LOCAL_EP* lep, STATE* state_ptr)
{
	/* error checking */
	if(state_ptr == NULL)
	{
		return;
	}

	if(lep == NULL)
	{
		return;
	}

	if(state_ptr->lep != lep)
	{
		return;
	}

	MESSAGE* unmap_msg = message_new(NULL, MSG_UNMAP);
	state_send_message(state_ptr, unmap_msg);
	message_free(unmap_msg);

	//lep->flag ;
}

void ep_unmap_recv(LOCAL_EP* lep, STATE* state_ptr)
{
	/* error checking */
	if(state_ptr == NULL)
	{
		return;
	}

	if(lep == NULL)
	{
		return;
	}

	if(state_ptr->lep != lep)
	{
		return;
	}

	/* send acknowledge */
	MESSAGE* unmap_ack_msg = message_new(NULL, MSG_UNMAP_ACK);
	state_send_message(state_ptr, unmap_ack_msg);
	message_free(unmap_ack_msg);
	state_ptr->is_mapped = 0;

    /* close connection */
    ep_unmap_final(lep, state_ptr);
}

void ep_unmap_final(LOCAL_EP* lep, STATE* state_ptr)
{
	/* error checking */
	if(state_ptr == NULL)
	{
		return;
	}

	if(lep == NULL)
	{
		return;
	}

	if(state_ptr->lep != lep)
	{
		return;
	}

	/* remove from the array of mappings */
	if(array_remove(lep->mappings_states, state_ptr) < 0)
	{
		return;
	}

	/* close the connection
	(*(state_ptr->module->fc_connection_close))(state_ptr->conn);
	state_free(state_ptr);*/
}



void ep_unmap_all(LOCAL_EP *lep)
{
    if (!lep)
        return;


    int i;
	STATE* state;
	for(i=0; i<array_size(lep->mappings_states); i++)
	{
		state = array_get(lep->mappings_states, i);
		ep_unmap_send(lep, state);
	}

	array_free(lep->mappings_states);
	lep->mappings_states = array_new(ELEM_TYPE_PTR);
}


LOCAL_EP* endpoint_query(JSON* query_json)
{

	LOCAL_EP *lep;
	JSON* lep_json;

	int i; Array *_keys = map_get_keys(locales);
	for(i=0;i<array_size(_keys); i++)
	{
		char* key = array_get(_keys, i);
		lep = map_get(locales, key);
		lep_json = ep_local_to_json(lep);

		if( json_filter_validate(lep_json, query_json) )
		{
			//if()
			//slog(SLOG_INFO, SLOG_INFO, "EP QUERY result: %s", lep->ep->name);
			return lep;
		}
	}

	return NULL;
}


int ep_send_json(LOCAL_EP *lep, JSON* json, const char* msg_id, int status)
{
	//LOCAL_EP *lep = (LOCAL_EP*)(ep->data);
	STATE* state;
	int i;
	for(i=0; i<array_size(lep->mappings_states); i++)
	{
		state = array_get(lep->mappings_states, i);
		state_send_json(state, msg_id, json, status);
	}

	return 0;
}

int ep_send_message(LOCAL_EP *lep, MESSAGE* msg)
{
	//LOCAL_EP *lep = (LOCAL_EP*)(ep->data);
	STATE* state;
	int i;
	for(i=0; i<array_size(lep->mappings_states); i++)
	{
		state = array_get(lep->mappings_states, i);
		state_send_message(state, msg);
	}

	return 0;
}


int eps_init()
{
	endpoints = map_new(KEY_TYPE_STR);
	locales = map_new(KEY_TYPE_STR);

	return (endpoints != NULL && locales != NULL);
}
