/*
 * json_builds.h
 *
 *  Created on: 29 May 2016
 *      Author: Raluca Diaconu
 */

#ifndef COMMON_JSON_BUILDS_H_
#define COMMON_JSON_BUILDS_H_

#include "json.h"
#include "endpoint.h"


/* load all schemas */
int json_load_all_file_schemas();

/* loads schema from file */
//JSON *json_load_from_file(char *filename);


/* builds protocol messages */

JSON * json_build_hello(JSON * other_data);
JSON * json_build_hello_ack(int code, JSON * other_data);

JSON * json_build_auth(Array* auth_credentials, JSON * other_data);
JSON * json_build_auth_ack(int code, const char* key, JSON * other_data);

JSON * json_build_map(LOCAL_EP *lep, JSON * ep_query, JSON * cpt_query);
JSON * json_build_map_ack(LOCAL_EP *lep, int code, JSON * other_data);

JSON * json_build_unmap(JSON * other_data);
JSON * json_build_unmap_ack(int code, JSON * other_data);

JSON * json_build_disconnect(JSON * other_data);
JSON * json_build_disconnect_ack(int code, JSON * other_data);


/* validation of various schemas in the protocol */

/*
 * ep definition schemas
 */
int json_validate_ep_def(JSON * msg);
int json_validate_ep_remote_def(JSON * msg);
int json_validate_req_resp(JSON * msg);
int json_validate_src_snk(JSON * msg);

/*
 * Validates @msg against hello schema and checks version.
 */
int json_validate_hello(JSON * msg);
int json_validate_hello_ack(JSON * msg);

/* validate access modules */
int json_validate_auth(JSON * msg);
int json_validate_auth_ack(JSON * msg);

/*
 * Validates @msg against map schema and checks version.
 */
int json_validate_map(JSON * msg);
int json_validate_map_ack(JSON * msg);

/*
 * Validates @msg against UNmap schema.
 */
int json_validate_unmap(JSON * msg);
int json_validate_unmap_ack(JSON * msg);

/*
 * Validates @msg against disconnect schema.
 */
int json_validate_disconnect(JSON * msg);
int json_validate_disconnect_ack(JSON * msg);

/* validates messages */
int json_validate_message(LOCAL_EP *lep, JSON * msg);
int json_validate_response(LOCAL_EP *lep, JSON * msg);


#endif /* COMMON_JSON_BUILDS_H_ */
