/*
 * protocol.h
 *
 *  Created on: 14 Apr 2016
 *      Author: Raluca Diaconu
 */

#ifndef COREMW_PROTOCOL_H_
#define COREMW_PROTOCOL_H_

/* todo: replace with endpoint.h" */
#include "endpoint.h"

#include <hashmap.h>
#include <state.h>



/*
 * Initialises the protocol function table.
 * This table contains the functions to be applied in each protocol state.
 */
//int proto_init_state_functions();


/* recv hello msg, send hello ack */
void core_proto_hello(STATE *state_ptr, MESSAGE *hello_msg);

/* recv hello ack */
void core_proto_hello_ack(STATE *state_ptr, MESSAGE *hello_ack_msg);

/* send own credentials: each party */
void core_proto_send_auth(STATE *state_ptr);

/* receive and check credentials for component and sends ACK */
void core_proto_check_auth(STATE *state_ptr, MESSAGE *auth_msg);

/* receive credentials ACK for component */
void core_proto_check_auth_ack(STATE *state_ptr, MESSAGE *auth_Ack_msg);

/*
 * executes a map command on an existing connection.
 * needs to be preceded by a core_hello call.
 * @return: error code
 * TODO: merge with core_map
 */
int core_proto_map_to(STATE *state_ptr, LOCAL_EP *lep, JSON* ep_query, JSON* cpt_query);

/* awaits for map msg */
void core_proto_map(STATE *state_ptr, MESSAGE *data);
/* awaits for map_ack */
void core_proto_map_ack(STATE *state_ptr, MESSAGE *map_ack_msg);



#endif /* COREMW_PROTOCOL_H_ */
