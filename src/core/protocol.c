/*
 * protocol.c
 *
 *  Created on: 14 Apr 2016
 *      Author: Raluca Diaconu
 *
 * This file contains functions for protocol state update.
 * Functions take connection id and message.
 * For external connections only.
 */


#include "protocol.h"

#include "json.h"
#include "json_builds.h"
#include "endpoint.h"
#include "message.h"
#include "state.h"
#include <utils.h>

#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "../common/sync.h"
#include "../module_wrappers/access_wrapper.h"
#include "manifest.h"

//TODO: remove, replace with sync

extern int map_sync_pipe[2];


/* recv hello msg, send hello ack
 * only in STATE_HELLO_S  and STATE_HELLO_2 */
void core_proto_hello(STATE *state_ptr, MESSAGE *hello_msg)
{
	/*
	 * because this function is called from the core only, these should never happen:
	 * if(state_ptr == NULL)
	 * if(hello_msg == NULL)
	 * if(hello_msg->status != MSG_HELLO)
	 */

	JSON *hello_json = hello_msg->_msg_json;
	/* validate */
	int hello_validation =  json_validate_hello(hello_json);
	if(hello_validation != 0)
		goto final;

	/* save the address */
	state_ptr->cpt_manifest = json_get_json(hello_json, "manifest");
	if (state_ptr->cpt_manifest != NULL)
    	state_ptr->addr = strdup_null(json_get_str(state_ptr->cpt_manifest, "address"));

	/* set new state */
	if(state_ptr->state == STATE_HELLO_S)
		state_ptr->state = STATE_HELLO_ACK_S;
	else if(state_ptr->state == STATE_HELLO_2)
	{
		state_ptr->state = STATE_AUTH;
		sync_trigger(map_sync_pipe[0], "hello:0\n");
	}

	final:/* send acknowledge, free stuff */
	{
		JSON* manifest = manifest_get(MANIFEST_SIMPLE);
		JSON* hello_ack_json = json_build_hello_ack(hello_validation, manifest);
		state_send_json(state_ptr, NULL, hello_ack_json, MSG_HELLO_ACK);

		//json_free(hello_json);
		json_free(hello_ack_json);
		json_free(manifest);
	}
}

/* recv hello ack
 * only in state STATE_HELLO_S and STATE_HELLO_ACK_S */
void core_proto_hello_ack(STATE *state_ptr, MESSAGE *hello_ack_msg)
{
	/*
	 * because this function is called from the core only, these should never happen:
	 * if(state_ptr == NULL)
	 * if(hello_ack_msg == NULL)
	 * if(hello_ack_msg->status != MSG_HELLO_ACK)
	 */

	JSON *hello_ack_json = hello_ack_msg->_msg_json;

	/* validate */
	int hello_ack_validation = json_validate_hello_ack(hello_ack_json);
	if(hello_ack_validation != 0)
		goto final;

	/* sync and trigger auth step */
	if(state_ptr->state == STATE_HELLO_S)
		state_ptr->state = STATE_HELLO_2;
	else if(state_ptr->state == STATE_HELLO_ACK_S)
	{
		state_ptr->state = STATE_AUTH;
		sync_trigger(map_sync_pipe[0], "hello:0\n");
	}
	//char ret_pipe[] = "hello:0";
	//send(map_sync_pipe[0], ret_pipe, strlen(ret_pipe), 0);

	final:
	{
		//json_free(hello_ack_json);
		core_proto_send_auth(state_ptr);
	}
}

/* send own credentials: each party */
void core_proto_send_auth(STATE *state_ptr)
{
	/*
	 * because this function is called from the core only, these should never happen:
	 * if(state_ptr == NULL)
	 * if(hello_ack_msg == NULL)
	 * if(hello_ack_msg->status != MSG_HELLO_ACK)
	 */

	/* the structures containing all credentials */
	JSON* auth_creds_json = json_new(NULL);
	Array* auth_creds_aray = array_new(ELEM_TYPE_PTR);

	/* build the structures iterating over all auth modules */
	Array* access_modules_keys = map_get_keys(access_modules);

	int i;
	char* acc_key;
	ACCESS_MODULE* access_module;
	JSON* credential_json;

	for(i=0; i<array_size(access_modules_keys); i++)
	{
		acc_key = array_get(access_modules_keys, i);
		access_module = map_get(access_modules, acc_key);

		credential_json = json_new(NULL);
		json_set_str(credential_json, "name", acc_key);
		json_set_str(credential_json, "credential", (*(access_module->fc_get_credential))());
		array_add(auth_creds_aray, credential_json);
	}

	/* send credentials */
	JSON* auth_json = json_build_auth(auth_creds_aray, NULL);
	state_send_json(state_ptr, NULL, auth_json, MSG_AUTH);
	json_free(auth_json);

	final: /* free allocated memory */
	{
		json_free(auth_creds_json);
		array_free(auth_creds_aray);
	}
}

/* receive and check credentials for component and sends ACK */
void core_proto_check_auth(STATE *state_ptr, MESSAGE *auth_msg)
{
	/*
	 * because this function is called from the core only, these should never happen:
	 * if(state_ptr == NULL)
	 * if(auth_msg == NULL)
	 * if(auth_msg->status != MSG_AUTH)
	 */

	if(access_no_auth())
	{
		/* no auth module loaded,
		 * no auth required
		 */
		state_ptr->access_module = NULL;
		state_ptr->is_auth = 1;
		//conn_send_sockpair(map_sync_pipe[0], "access ok");
		//char ret_pipe[] = "access ok";
		//send(map_sync_pipe[0], ret_pipe, strlen(ret_pipe), 0);

		JSON* access_ack_json = json_build_auth_ack(0, "", NULL);
		state_send_json(state_ptr, NULL, access_ack_json, MSG_AUTH_ACK);

		if(state_ptr->state == STATE_AUTH)
			state_ptr->state = STATE_AUTH_ACK;
		else if(state_ptr->state == STATE_AUTH_2)
		{
			state_ptr->state = STATE_MAP;
			sync_trigger(map_sync_pipe[0], "access:0");
		}

		/*if(state_ptr->is_auth && state_ptr->am_auth)
			state_ptr->state = STATE_MAP;
		else
			state_ptr->state = STATE_AUTH;*/

		return;
	}

	/* validate */
	JSON* auth_ack_json = NULL;
	Array* auth_creds_array = NULL;

	JSON *auth_json = auth_msg->_msg_json;
	int auth_validate = json_validate_auth(auth_json);
	if(auth_validate != 0)
	{
		//(*(state_ptr->module->fc_connection_close))(state_ptr->conn);
		goto final;
	}

	/* get all credentials */
	auth_creds_array = json_get_jsonarray(auth_json, "credentials");
	int i;
	JSON* auth_cred_json;
	char* name = NULL;
	char* cred = NULL;
	ACCESS_MODULE* access_module;

	for(i=0; i<array_size(auth_creds_array); i++)
	{
		free(name);
		free(cred);
		auth_cred_json = array_get(auth_creds_array, i);
		name           = json_get_str(auth_cred_json, "name");
		cred           = json_get_str(auth_cred_json, "credential");
		access_module    = access_get_module(name);
		if(access_module == NULL)
			continue;

		if((*(access_module->fc_authenticate))(cred, state_ptr))
		{
			state_ptr->access_module = access_module;
			state_ptr->is_auth = 1;

			break;
		}
	}
	free(name);
	free(cred);


	/* if auth succeeded */
	if(state_ptr->is_auth)
	{
		if(state_ptr->state == STATE_AUTH)
			state_ptr->state = STATE_AUTH_ACK;
		else if(state_ptr->state == STATE_AUTH_2)
		{
			state_ptr->state = STATE_MAP;
			sync_trigger(map_sync_pipe[0], "access:0");
		}
	}
	else
	{
		slog(SLOG_ERROR, SLOG_ERROR, "Proto: auth failed\n");
		auth_validate = -3;
	}


	final:
	{
		auth_ack_json = json_build_auth_ack(auth_validate, "", NULL);
		state_send_json(state_ptr, NULL, auth_ack_json, MSG_AUTH_ACK);
		json_free(auth_ack_json);
		array_free(auth_creds_array);
		//json_free(auth_json);
	}
}

/* receive credentials ACK for component */
void core_proto_check_auth_ack(STATE *state_ptr, MESSAGE *auth_ack_msg)
{
	/*
	 * because this function is called from the core only, these should never happen:
	 * if(state_ptr == NULL)
	 * if(auth_ack_msg == NULL)
	 * if(auth_ack_msg->status != MSG_AUTH_ACK)
	 */

	JSON *auth_ack_json = auth_ack_msg->_msg_json;
	int auth_ack_validate = json_validate_auth_ack(auth_ack_json);

	if(auth_ack_validate != 0)
	{
		(*(state_ptr->module->fc_connection_close))(state_ptr->conn);
		goto final;
	}
	else
		state_ptr->am_auth = 1;

	if(state_ptr->state == STATE_AUTH)
		state_ptr->state = STATE_AUTH_2;
	else if(state_ptr->state == STATE_AUTH_ACK)
	{
		state_ptr->state = STATE_MAP;
		sync_trigger(map_sync_pipe[0], "access:0");
	}

	if(state_ptr->state == STATE_MAP ||
			state_ptr->state == STATE_MAP_ACK ||
			state_ptr->state == STATE_EXT_MSG)
		goto final;

	final:
	{
		;//json_free(auth_ack_json);
	}
}

/* map cli sends map msg */
int core_proto_map_to(STATE *state_ptr, LOCAL_EP *lep, JSON* ep_query, JSON* cpt_query)
{
	/*
	 * because this function is called from the core only, these should never happen:
	 * if(state_ptr == NULL)
	 */

	state_ptr->lep = lep;
	state_ptr->state = STATE_MAP_ACK;

	JSON *map_json = json_build_map(lep, ep_query, cpt_query);
	state_send_json(state_ptr, NULL, map_json, MSG_MAP);
	json_free(map_json);

	return 0;
}

//TODO
/* map client gets map ack */
void core_proto_map_ack(STATE *state_ptr, MESSAGE *map_ack_msg)
{
	/*
	 * because this function is called from the core only, these should never happen:
	 * if(state_ptr == NULL)
	 * if(map_ack_msg == NULL)
	 * if(map_ack_msg->status != MSG_MAP_ACK)
	 */

	JSON *map_ack_json = map_ack_msg->_msg_json;
	int map_ack_validate = json_validate_map_ack(map_ack_json);

	if(map_ack_validate == 0)
	{
		LOCAL_EP  *lep = state_ptr->lep;
		if(lep == NULL)
		{
			slog(SLOG_ERROR, SLOG_ERROR, "PROTO: received map ack for no ep %s");
			goto final;
		}
		ep_map(lep, state_ptr);
		state_ptr->state = STATE_EXT_MSG;
		state_ptr->ep_metadata = json_get_json(map_ack_json, "ep_metadata");
	}
	else
	{
		state_ptr->state = STATE_BAD;
		state_ptr->flag = map_ack_validate;
		slog(SLOG_WARN, SLOG_WARN, "PROTO: Cannot map, Bad state reached for fd (%s:%d)",
				state_ptr->module->name, state_ptr->conn);
	}

	//TODO
	if(state_ptr->state == STATE_EXT_MSG)
	{
		sync_trigger(map_sync_pipe[0], "state_ext _msg");
		//char ret_pipe[] = "state_ext _msg";
		//send(map_sync_pipe[0], ret_pipe, strlen(ret_pipe), 0);
	}
	else
	{
		sync_trigger(map_sync_pipe[0], "state_bad");
		//char ret_pipe[] = "state_bad";
		//send(map_sync_pipe[0], ret_pipe, strlen(ret_pipe), 0);
	}

	final:
	{
		;//json_free(map_ack_json);
	}
}

/* map srv gets map msg*/
void core_proto_map(STATE *state_ptr, MESSAGE *map_msg)
{
	/*
	 * because this function is called from the core only, these should never happen:
	 * if(state_ptr == NULL)
	 * if(map_msg == NULL)
	 * if(map_ack_msg->status != MSG_MAP)
	 */

	JSON *map_json = map_msg->_msg_json;
	int map_validate = json_validate_map(map_json);
	LOCAL_EP *lep = NULL;

	if(map_validate == 0)
	{
		Array *ep_query = json_get_array(map_json, "ep_query");
		Array *cpt_query = json_get_array(map_json, "cpt_query");

		JSON* ep_query_json = json_new(NULL);
		json_set_array(ep_query_json, NULL, ep_query);

		lep = endpoint_query(ep_query_json);
		if(lep != NULL)
		{
			ep_map(lep, state_ptr);
			state_ptr->state = STATE_EXT_MSG;
			state_ptr->ep_metadata = json_get_json(map_json, "ep_metadata");
			slog(SLOG_DEBUG, SLOG_DEBUG, "ep metadata: %s\n\n", json_to_str_pretty(state_ptr->ep_metadata));
		}
		else
		{
			map_validate = EP_NO_EXIST;
			state_ptr->state = STATE_BAD;
		}
	}

	state_ptr->flag = map_validate;

	JSON *map_ack_json = json_build_map_ack(lep, map_validate, NULL);
	state_send_json(state_ptr, NULL, map_ack_json, MSG_MAP_ACK);

	json_free(map_ack_json);
	//json_free(map_json);
}


