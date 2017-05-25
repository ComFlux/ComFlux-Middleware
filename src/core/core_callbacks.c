/*
 * on_data.c
 *
 *  Created on: 20 Mar 2017
 *      Author: Raluca Diaconu
 */

#include "core_callbacks.h"

#include <json.h>
#include <json_builds.h>
#include <core_module_api.h>
#include <core_module.h>
#include <protocol.h>
#include <utils.h>
#include <sync.h>

#include <conn_fifo.h>

#include <string.h>

/* to check*/
#include "manifest.h"

extern STATE* app_state;

/* callbacks for the com modules */
void core_on_data(COM_MODULE* module, int conn, const char* data)
{
	/*slog(SLOG_INFO, SLOG_INFO, "CORE:core_on_data\n"
			"\tfrom: (%s:%d)\n"
			"\tdata: *%s*", module->name, conn, data);*/
	STATE* state_ptr = states_get(module, conn);
	buffer_update(state_ptr->buffer, data, strlen(data));
}

extern int map_sync_pipe[2];
void core_on_connect(COM_MODULE* module, int conn)
{
	slog(SLOG_INFO, SLOG_INFO, "CORE:core_on_connect:\n"
			"\tconnected to (%s:%d)", module->name, conn);
	/*
	 * these cases should not occur as long as the core connects to the app with sockpair
	 */
	if (app_state == NULL)
	{
		core_terminate();
		slog(SLOG_FATAL, SLOG_FATAL, "CORE:core_on_connect: Component not connected.");
		exit(EXIT_FAILURE);
	}
	if (module == app_state->module && conn == app_state->conn)
	{
		core_terminate();
		slog(SLOG_FATAL, SLOG_FATAL, "CORE:core_on_connect: Something went wrong.");
		exit(EXIT_FAILURE);
	}

	/*
	 * This is a new outside connection.
	 * If it connects to a bridge (transport layer) module
	 * 		then expect another component and perform the protocol
	 * Otherwise, themodule connect to outside, e.g. REST
	 *		go to the final mapped state.
	 */
	STATE* state_ptr = state_new(module, conn, STATE_HELLO_S);
	states_set(module, conn, state_ptr);

	if( (*module->fc_is_bridge)() )
	{
		state_ptr->on_message = &core_on_proto_message;

		JSON* manifest = manifest_get(MANIFEST_SIMPLE);
		json_set_str(manifest, "com_module", module->name);
		json_set_str(manifest, "address", module->address);

		JSON *hello_json = json_build_hello(manifest);
		state_send_json(state_ptr, NULL, hello_json, MSG_HELLO);
		json_free(hello_json);
		json_free(manifest);
	}
	else /* jump over the proto */
	{
		state_ptr->on_message = &core_on_message;

		state_ptr->state = STATE_MAP; //wating for map
		sync_trigger(map_sync_pipe[0], "mapped");
		sync_trigger(map_sync_pipe[0], "mapped");
	}
}

void core_on_disconnect(COM_MODULE* module, int conn)
{
	// If the app connection closed without terminating the core, something bad has happened.
	STATE *state_ptr = states_get(module, conn);

	if(state_ptr == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE:core_on_disconnect: invalid null state");
		return;
	}

	if (state_ptr == app_state)
	{
		core_terminate();
		slog(SLOG_FATAL, SLOG_FATAL, "CORE: Component disconnected unexpectedly.");

		exit(EXIT_FAILURE);
	}

	ep_unmap_final(state_ptr->lep, state_ptr);

	if(state_ptr->access_module)
		(*(state_ptr->access_module->fc_disconnect))(state_ptr);

	state_free(state_ptr);

	slog(SLOG_INFO, SLOG_INFO, "CORE:core_on_disconnect:\n"
			"\tfrom (%s:%d)", module->name, conn);

	(*module->fc_connection_close)(conn);
}



/* callbacks for the state / message stuff */

/*
 * Handles messages related to hello and auth handshake.
 * this handler is assigned to states of transport layer modules
 * -- those for which is_bridge returns 1.
 */
void core_on_proto_message(STATE* state_ptr, MESSAGE* _msg)
{
	if(state_ptr == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: on_proto_message: invalid null state");
		return;
	}
	if(_msg == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: on_proto_message: invalid null message");
		return;
	}

	COM_MODULE* module = state_ptr->module;
	int conn = state_ptr->conn;

	slog(SLOG_INFO, SLOG_INFO, "CORE:core_on_proto_message\n"
		 "\tMessage status: %d: %s\n"
		 "\tstate: %d: %s"
		 "\tconnection: (%s:%d)",
		 _msg->status, message_status_to_str(_msg->status), state_ptr->state, state_get_str(state_ptr->state), module->name, conn);


	/* check message type and connection state */
	switch(_msg->status)
	{
	/* protocol messages */
	case MSG_HELLO:
		if(state_ptr->state == STATE_HELLO_S || state_ptr->state == STATE_HELLO_2)
			core_proto_hello(state_ptr, _msg);
		break;
	case MSG_HELLO_ACK:
		if(state_ptr->state == STATE_HELLO_S || state_ptr->state == STATE_HELLO_ACK_S)
			core_proto_hello_ack(state_ptr, _msg);
		break;
	case MSG_AUTH:
		if(state_ptr->state == STATE_AUTH || state_ptr->state == STATE_AUTH_2)
			core_proto_check_auth(state_ptr, _msg);
		if(state_ptr->am_auth && state_ptr->is_auth)
		{
			slog(SLOG_INFO, SLOG_INFO, "CORE:change callback 1\n");
			state_ptr->on_message = &core_on_message;
		}
		break;
	case MSG_AUTH_ACK:
		if(state_ptr->state == STATE_AUTH || state_ptr->state == STATE_AUTH_ACK)
			core_proto_check_auth_ack(state_ptr, _msg);
		if(state_ptr->am_auth && state_ptr->is_auth)
		{
			slog(SLOG_INFO, SLOG_INFO, "CORE:change callback 2\n");
			state_ptr->on_message = &core_on_message;
		}
		break;

	default:
		/* bad message status */
		slog(SLOG_ERROR, SLOG_ERROR,
			"CORE:core_on_message: bad command status: %d:%s",
			_msg->status, message_status_to_str(_msg->status));
	}

	message_free(_msg);
}

/*
 * Handles messages related to hello and auth handshake.
 * this handler is assigned to states of transport layer modules
 * -- those for which is_bridge returns 1.
 */
void core_on_message(STATE* state_ptr, MESSAGE* _msg)
{
	if(state_ptr == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: on_message: invalid null state");
		return;
	}
	if(_msg == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: on_message: invalid null message");
		return;
	}

	COM_MODULE* module = state_ptr->module;
	int conn = state_ptr->conn;

	slog(SLOG_INFO, SLOG_INFO, "CORE:\n"
		 "\tMessage status: %d: %s\n"
		 "\tstate: %d: %s"
		 "\tconnection: (%s:%d)",
		 _msg->status, message_status_to_str(_msg->status), state_ptr->state, state_get_str(state_ptr->state), module->name, conn);


	/* check message type and connection state */
	switch(_msg->status)
	{
	/* map messages */
	case MSG_MAP:
		if(state_ptr->state == STATE_MAP)
			core_proto_map(state_ptr, _msg);
		break;
	case MSG_MAP_ACK:
		slog(SLOG_DEBUG, SLOG_DEBUG, "\n\n State: %d -- %s\n", state_ptr->state, state_get_str(state_ptr->state));
		if(state_ptr->state == STATE_MAP_ACK)
			core_proto_map_ack(state_ptr, _msg);
		break;
	case MSG_UNMAP:
		if(state_ptr->is_mapped)
			ep_unmap_recv(state_ptr->lep, state_ptr);
		break;
	case MSG_UNMAP_ACK:
		ep_unmap_final(state_ptr->lep, state_ptr);
		break;

	/* endpoint messages */
	case MSG_MSG:
	case MSG_REQ:
	case MSG_RESP_NEXT:
	case MSG_RESP_LAST:
		if(state_ptr->state == STATE_EXT_MSG)
		{
			//char* data = message_to_str(_msg);
			call_external_command_handler(state_ptr, _msg);
		}
		break;
	case MSG_STREAM:
		_msg->ep = state_ptr->lep->ep;
		recv_stream_msg(_msg);
		break;
	case MSG_STREAM_CMD:
		_msg->ep = state_ptr->lep->ep;
		recv_stream_cmd(_msg);
		break;

	case MSG_NONE:
	default:
		/* bad message status */
		slog(SLOG_ERROR, SLOG_ERROR,
			"CORE:core_on_message: bad command status: %d:%s",
			_msg->status, message_status_to_str(_msg->status));
	}
	message_free(_msg);
}

/* core_on_component_message handles messages from the component.
 * This handler is assigned only to the app_state */
void core_on_component_message(STATE* state_ptr, MESSAGE* msg)
{
	/* checking errors */
	if(state_ptr == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: %s: invalid null state", __func__);
		return;
	}
	if(msg == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: %s: invalid null message", __func__);
		return;
	}
	if(state_ptr != app_state)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: %s: invalid state, not app_state", __func__);
		return;
	}
	if(msg->status != MSG_CMD)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: %s: invalid message status, %d:%s\n"
				"\t should be: %d:%s", __func__, msg->status, message_status_to_str(msg->status),
				MSG_CMD, message_status_to_str(MSG_CMD));
		return;
	}
	if(state_ptr->state != STATE_APP_MSG)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: %s: invalid message sequence", __func__);
		return;
	}

	/* all is good, parse function call signature */
	JSON* cmd_json = json_new(msg->msg);
	char *module_id = json_get_str(cmd_json, "module_id");
	char *function_id = json_get_str(cmd_json, "function_id");
	char *return_type = json_get_str(cmd_json, "return_type");
	Array *args = json_get_array(cmd_json, "args");

	MESSAGE *return_msg = _core_call_array(module_id, function_id, return_type, args);

	/* get the result back to the component */
	if(return_msg != NULL)
	{
		return_msg->msg_id = (char*) malloc(11 * sizeof(char));
		strncpy(return_msg->msg_id, msg->msg_id, 10);
		state_send_message(app_state, return_msg);
	}

	array_free(args);
	message_free(return_msg);
	message_free(msg);
	json_free(cmd_json);
}


/* This function is called only for the app to register to the core */
void core_on_first_message(STATE* state_ptr, MESSAGE* msg)
{
	if(state_ptr != app_state)
	{
		slog(SLOG_FATAL, SLOG_FATAL, "CORE: core_on_first_message: invalid state\n"
				"\tstate should be app state");
		exit(EXIT_FAILURE);
	}
	if(state_ptr->state != STATE_FIRST_MSG)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: core_on_first_message: invalid message sequence; ignoring");
		return;
	}
	slog(SLOG_INFO, SLOG_INFO, "CORE: on first message; checking if app: *%s*", msg->msg);

	/*if (!strcmp(data, app_key))
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: core_on_first_message: invalid key; ignoring");
		return;
	}*/

	/* no errors */
	state_ptr->state = STATE_APP_MSG;
	state_ptr->on_message = &core_on_component_message;
}



/* does just the hello + mapping for now */
void call_external_command_handler(STATE* state_ptr, MESSAGE* msg)
{
	/*
	 * it is called from the core so the following should not arise:
	 * if(state_ptr== NULL)
	 * if (msg == NULL)
	 * if(state_ptr->lep == NULL)
	 * if(state_ptr->lep->ep == NULL) // never!!!
	 */

	if(msg->status == MSG_UNMAP)
	{
		slog(SLOG_WARN, SLOG_WARN, "CORE: STATUS is UNMAP %s", state_ptr->lep->id);
		ep_unmap_recv(state_ptr->lep, state_ptr); //TODO:state
		(*(state_ptr->module->fc_connection_close))(state_ptr->conn);
		return;
	}
	else if (state_ptr->lep->ep->type == EP_STR_SNK &&
			state_ptr->lep->fifo != 0)
		    //(msg->status == MSG_STREAM || sta)
	{
		slog(SLOG_INFO, SLOG_INFO, "CORE: STATUS is MSG_STREAM %s", state_ptr->lep->id);
		fifo_send_message(state_ptr->lep->fifo, msg->msg);
		return;
	}


	MESSAGE *in_msg = message_parse(msg->msg);
	if(in_msg == NULL)
	{
		slog(SLOG_ERROR, SLOG_ERROR, "CORE: received empty message on fd (%s:%d)", state_ptr->module, state_ptr->conn);
		//message_free(msg);
		return;
	}
	in_msg->ep = state_ptr->lep->ep;
	in_msg->conn = state_ptr->conn;
	in_msg->module = strdup_null(state_ptr->module->name);

	/* apply the handler of the ep for incoming messages */
	if(state_ptr->lep->ep->handler != NULL)
	{
		(*state_ptr->lep->ep->handler)(in_msg);
	}
	else
	{
		state_send_message(app_state, in_msg);
	}

	//message_free(in_msg);
	//message_free(msg);
}

void recv_stream_cmd(MESSAGE* msg)
{
	if (msg->status != MSG_STREAM_CMD)
		return;

	if (msg->ep == NULL)
		return;

	if (msg->ep->type != EP_STR_SRC && msg->ep->type != EP_STR_SNK )
		return;


	slog(SLOG_INFO, SLOG_INFO, "CORE: STATUS is MSG_STREAM_CMD:\n"
			"\tep_id %s", msg->ep->id);

	LOCAL_EP* lep = (LOCAL_EP*)msg->ep->data;

	JSON* msg_json = json_new(msg->msg);
	int command = json_get_int(msg_json, "command");
	if(command == 1 && lep->fifo <= 0)
	{
		sprintf(lep->fifo_name, "/tmp/%s", randstring(5));
		lep->fifo = fifo_init_server(lep->fifo_name);
		msg->msg = lep->fifo_name;
		state_send_message(app_state, msg);
		return;
	}
	if(command == 0 && lep->fifo != 0)
	{
		fifo_close(lep->fifo);
		//unlink(lep->fifo_name);
	}
}

void recv_stream_msg(MESSAGE* msg)
{
	if (msg->status != MSG_STREAM)
		return;

	if (msg->ep == NULL)
		return;

	if (msg->ep->type != EP_STR_SNK )
		return;

	LOCAL_EP* lep = (LOCAL_EP*)msg->ep->data;
	fifo_send_message(lep->fifo, msg->msg);

}


void send_error_message(STATE* state_ptr, char *msg)
{
	slog(SLOG_ERROR, SLOG_ERROR, "CORE: send error %s to (%s:%d)", msg, state_ptr->module->name, state_ptr->conn);
	//conn_send_message(conn, msg);
}
