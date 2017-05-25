/*
 * on_data.h
 *
 *  Created on: 20 Mar 2017
 *      Author: Raluca Diaconu
 */

#ifndef SRC_CORE_CORE_CALLBACKS_H_
#define SRC_CORE_CORE_CALLBACKS_H_

#include <message.h>
#include <state.h>
#include <com_wrapper.h>

/* generic callbacks for com modules */

/**
 * The callback for all com  modules.
 * Parse a data stream into messages and then calls the core_on_message.
 */
void core_on_data(COM_MODULE* module, int conn, const char* data);

void core_on_connect(COM_MODULE* module, int conn);

void core_on_disconnect(COM_MODULE* module, int conn);


/* higher-level callbacks for message and state */

void core_on_proto_message(STATE* state, MESSAGE* msg);

/**
 * The callback for all states when a message has been parsed.
 * Parse a generic data stream into messages and then calls the core_on_message.
 */
void core_on_message(STATE* state, MESSAGE* msg);

/**
 * callback from component;
 * state is always app_state, but will keep the signature
 * message type should be msg_cmd
 */
void core_on_component_message(STATE* state, MESSAGE* msg);


/* before connecting to the app */
void core_on_first_message(STATE* state, MESSAGE* msg);

/* app conn handler */
void call_module_function_handler(COM_MODULE* module, int conn, const char *data);
/* external command */
void call_external_command_handler(STATE* state, MESSAGE* msg);

void recv_stream_cmd(MESSAGE* msg);
void recv_stream_msg(MESSAGE* msg);

/* to send an error message to a state *
 * maybe move in state.c ? */
void send_error_message(STATE* state, char *msg);


#endif /* SRC_CORE_CORE_CALLBACKS_H_ */
