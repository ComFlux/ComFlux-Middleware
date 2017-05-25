/*
 * json_builds.c
 *
 *  Created on: 29 May 2016
 *      Author: Raluca Diaconu
 */

#include "json_builds.h"

#include "json.h"
#include "environment.h" /* for VERSION */

#include <stdio.h>

JSON* ep_def_schema;
JSON* src_snk_def_schema;
JSON* req_resp_def_schema;

JSON* cmd_schema;
JSON* hello_schema;
JSON* hello_ack_schema;
JSON* auth_schema;
JSON* auth_ack_schema;
JSON* map_schema;
JSON* map_ack_schema;
JSON* unmap_schema;
JSON* unmap_ack_schema;
JSON* ep_reg_schema;
JSON* fc_call_schema;
JSON* fc_return_schema;

JSON* disconnect_schema;
JSON* disconnect_ack_schema;



int json_load_all_file_schemas()
{
	int ret = 0;

	ep_def_schema 		= json_load_from_file("ep_def.schema.json");
	src_snk_def_schema 	= json_load_from_file("src_snk_def.schema.json");
	req_resp_def_schema = json_load_from_file("req_resp_def.schema.json");

	cmd_schema 		= json_load_from_file("command.schema.json");
	hello_schema 	= json_load_from_file("preloaded_schemata/hello.schema.json");
	hello_ack_schema= json_load_from_file("preloaded_schemata/hello_ack.schema.json");
	auth_schema 	= json_load_from_file("preloaded_schemata/auth.schema.json");
	auth_ack_schema = json_load_from_file("preloaded_schemata/auth_ack.schema.json");
	map_schema 		= json_load_from_file("preloaded_schemata/map.schema.json");
	map_ack_schema 	= json_load_from_file("preloaded_schemata/map_ack.schema.json");
	unmap_schema 	= json_load_from_file("unmap.schema.json");
	unmap_ack_schema= json_load_from_file("unmap_ack.schema.json");
	ep_reg_schema 	= json_load_from_file("general_endpoint.schema.json");
	fc_call_schema 	= json_load_from_file("function_call.schema.json");
	fc_return_schema= json_load_from_file("function_return.schema.json");

	return ret;
	/*hello_schema == NULL || hello_ack_schema == NULL ||
		access_schema == NULL | access_ack_schema == NULL ||
		map_schema == NULL || map_ack_schema == NULL ||
		unmap_schema == NULL || unmap_ack_schema == NULL */
}



/* build protocol messages */

JSON * json_build_hello( JSON * other_data)
{
	JSON * hello_json = json_new(NULL);
	json_set_str(hello_json, "version", VERSION);
	json_set_json(hello_json, "manifest", other_data);

	//if(other_data != NULL)
	//	json_set_json(hello_json, "msg", other_data);

	return hello_json;
}

JSON * json_build_hello_ack( int code, JSON * other_data)
{
	JSON * hello_ack_json = json_build_hello(other_data);
	json_set_int(hello_ack_json, "ack_code", code);

	return hello_ack_json;
}


JSON * json_build_auth(Array* auth_credentials, JSON * other_data)
{
	JSON* auth_msg_json = json_new(NULL);
	json_set_array(auth_msg_json, "credentials", auth_credentials);
	//printf("\n\n **** %s ****\n\n", json_to_str_pretty(auth_credentials));
	//printf("\n\n **** %s ****\n\n", json_to_str_pretty(access_json));

	return auth_msg_json;
}

JSON * json_build_auth_ack(int code, const char* key, JSON * other_data)
{
	JSON* auth_ack_msg_json = json_new(NULL);
	json_set_int(auth_ack_msg_json, "ack_code", code);
	json_set_str(auth_ack_msg_json, "key", key);

	return auth_ack_msg_json;
}


JSON * json_build_map(LOCAL_EP *lep, JSON * ep_query, JSON * cpt_query)
{
	JSON * map_json = json_new(NULL);

	if(lep != NULL) /* && ep_query != NULL && cpt_query != NULL */
	{
		Array* ep_query_array = json_get_array(ep_query, NULL);
		Array* cpt_query_array = json_get_array(cpt_query, NULL);

		/* add new ep specific query: ep_type and hashes */
		char ep_type_query[50], msg_hash_query[50], resp_hash_query[50];
		sprintf(ep_type_query, "ep_type = \'%s\'", get_ep_type_matching_str(lep->ep->type));
		array_add(ep_query_array, ep_type_query);
		sprintf(msg_hash_query, "msg_hash = \'%s\'", lep->ep->msg);
		array_add(ep_query_array, msg_hash_query);
		if(lep->ep->resp)
		{
			sprintf(resp_hash_query, "resp_hash = \'%s\'", lep->ep->resp);
			array_add(ep_query_array, resp_hash_query);
		}

		/* set the parameters in the json */
		json_set_array(map_json, "ep_query", ep_query_array);
		json_set_array(map_json, "cpt_query", cpt_query_array);
		json_set_json(map_json, "ep_metadata", ep_to_json(lep->ep));
	}

	return map_json;
}

JSON * json_build_map_ack(LOCAL_EP *lep, int code, JSON * other_data)
{
	//JSON * map_ack_json = json_build_map(lep, NULL, NULL);
	JSON * map_ack_json = json_new(NULL);

	json_set_int(map_ack_json, "ack_code", code);
	if(lep != NULL && code == 0)
	{
		json_set_json(map_ack_json, "ep_metadata", ep_to_json(lep->ep));
	}

	return map_ack_json;
}

JSON * json_build_unmap(JSON * other_data)
{
	JSON * unmap_json = json_new(NULL);

	if(other_data != NULL)
		json_set_json(unmap_json, "msg", other_data);

	return unmap_json;
}

JSON * json_build_unmap_ack(int code, JSON * other_data)
{
	JSON * unmap_ack_json = json_new(NULL);

	if(other_data != NULL)
		json_set_json(unmap_ack_json, "msg", other_data);

	return unmap_ack_json;
}

JSON * json_build_disconnect(JSON * other_data)
{
	JSON * hello_json = json_new(NULL);

	if(other_data != NULL)
		json_set_json(hello_json, "msg", other_data);

	return hello_json;
}

JSON * json_build_disconnect_ack(int code, JSON * other_data)
{
	JSON * hello_json = json_new(NULL);

	if(other_data != NULL)
		json_set_json(hello_json, "msg", other_data);

	return hello_json;
}

/* validate ep defs */

int json_validate_ep_def(JSON * msg)
{
	if (json_validate(ep_def_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating ep_schema schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}
	return JSON_OK;
}
int json_validate_ep_remote_def(JSON * msg)
{
	if (json_validate(ep_def_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating ep_remote schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}
	return JSON_OK;
}
int json_validate_req_resp(JSON * msg)
{
	if (json_validate(req_resp_def_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating ep_req_resp_def schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}
	return JSON_OK;
}
int json_validate_src_snk(JSON * msg)
{
	if (json_validate(src_snk_def_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating ep_src_snk_def schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}
	return JSON_OK;
}

/* validate protocol messages */

int json_validate_hello(JSON * msg)
{
	if (json_validate(hello_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating hello message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}
	char *version, *session_id;
	session_id = json_get_str(msg, "session_id");
	version = json_get_str(msg, "version");

	if (!validate_version(version))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error incompatible version: %s ... %s", version, json_to_str(msg));
		return -1;
	}

	return JSON_OK;
}

int json_validate_hello_ack(JSON * msg)
{
	if (json_validate(hello_ack_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating hello ack message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}
	char *session_id;
	int code;
	session_id = json_get_str(msg, "session_id");
	code = json_get_int(msg, "ack_code");

	return code;
}

/* 0 if ok, code!=0 invalid */
int json_validate_auth(JSON * msg)
{
	return JSON_OK;
	if (json_validate(auth_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating access message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}

	return JSON_OK;
}

/* 0 if ok, code!=0 invalid */
int json_validate_auth_ack(JSON * msg)
{
	if (json_validate(auth_ack_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating access ack message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}
	int code;
	code = json_get_int(msg, "ack_code");

	return code;
}

int json_validate_map(JSON * msg)
{
	if (json_validate(map_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating map message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}

	return JSON_OK;
}

int json_validate_map_ack(JSON * msg)
{
	if (json_validate(map_ack_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating map ack message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}

	int map_code = json_get_int(msg, "ack_code");
	return map_code;
}

int json_validate_unmap(JSON * msg)
{
	if (json_validate(unmap_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating unmap message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}

	return JSON_OK;
}

int json_validate_unmap_ack(JSON * msg)
{
	if (json_validate(unmap_ack_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating unmap ack message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}

	int unmap_code = json_get_int(msg, "ack_code");
	return unmap_code;
}

int json_validate_disconnect(JSON * msg)
{
	if ( json_validate(disconnect_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating disconnect message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}

	return JSON_OK;
}

int json_validate_disconnect_ack(JSON * msg)
{
	if (json_validate(disconnect_ack_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating disconnect ack message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}

	int disconnect_code = json_get_int(msg, "ack_code");
	return disconnect_code;
}


int json_validate_message(LOCAL_EP *lep, JSON * msg)
{
	if (json_validate( lep->msg_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating\n"
				"\tmessage: %s\n"
				"\twith ep schema: %s", json_to_str(msg), json_to_str(lep->msg_schema));
		return 0; //JSON_NOT_VALID;
	}

	return JSON_OK;
}
int json_validate_response(LOCAL_EP *lep, JSON * msg)
{
	if ( json_validate(lep->resp_schema, msg))
	{
		slog(1, SLOG_ERROR, "PROTO JSON: error validating disconnect message schema: %s", json_to_str(msg));
		return JSON_NOT_VALID;
	}

	return JSON_OK;

}
