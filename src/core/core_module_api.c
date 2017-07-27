/*
 * core_module_api.c
 *
 *  Created on: 13 Aug 2016
 *      Author: Raluca Diaconu
 */

#include "core_module_api.h"

#include "core_module.h"

//extern HashMap *endpoints;
extern HashMap *locales;

/* all functionality accessible from the lib with function_call */
HashMap *void_function_table_array;
HashMap *int_function_table_array;
HashMap *float_function_table_array;
HashMap *string_function_table_array;
HashMap *message_function_table_array;


int core_register_endpoint_array(Array* argv)
{
	if (array_size(argv) < 1)
		return -1;

	char* json_str = (char*)array_get( argv, 0 );
	return core_register_endpoint(json_str);
}

void core_remove_endpoint_array(Array* argv)
{
	if (array_size(argv) < 1)
		return;

	char *ep_id = (char*)array_get( argv, 0 );
	return core_remove_endpoint(ep_id);
}

int core_map_all_modules_array(Array* argv)
{
	if (array_size(argv) < 4)
		return -1;

	char* local_ep_id = (char*)array_get( argv, 0 );
	char* addr = (char*)array_get( argv, 1 );

	char* ep_query_str = (char*)array_get( argv, 2 );
	char* cpt_query_str = (char*)array_get( argv, 3 );

	LOCAL_EP* lep = map_get(locales, local_ep_id);
	JSON* ep_query = json_new(ep_query_str);
	JSON* cpt_query = json_new(cpt_query_str);

	int result = core_map_all_modules(lep, addr, ep_query, cpt_query);

	json_free(ep_query);
	json_free(cpt_query);

	return result;
}

int core_map_module_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_map_module_array");
	if (array_size(argv) < 4)
		return -1;

	char* local_ep_id = (char*)array_get( argv, 0 );
	char* module = (char*)array_get( argv, 1 );
	char* addr = (char*)array_get( argv, 2 );

	char* ep_query_str = (char*)array_get( argv, 3 );
	char* cpt_query_str = (char*)array_get( argv, 4 );

	LOCAL_EP* lep = map_get(locales, local_ep_id);
	JSON* ep_query = json_new(ep_query_str);
	JSON* cpt_query = json_new(cpt_query_str);

	int result = -1;
	int i;
	COM_MODULE *com_module;

	com_module = map_get(com_modules, module);

	result = core_map(lep, com_module, addr, ep_query, cpt_query);

	json_free(ep_query);
	json_free(cpt_query);
	return result;
}

void core_map_lookup_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: map lookup array");
	if (array_size(argv) < 3)
		return ;

	char* local_ep_id = (char*)array_get( argv, 0 );
	char* ep_query_str = (char*)array_get( argv, 1 );
	char* cpt_query_str = (char*)array_get( argv, 2 );
	char* max_nb_str = (char*)array_get( argv, 3 );

	LOCAL_EP* lep = map_get(locales, local_ep_id);
	JSON* ep_query = json_new(ep_query_str);
	JSON* cpt_query = json_new(cpt_query_str);

	int max_nb;
	sscanf(max_nb_str, "%d", &max_nb);
	core_map_lookup(lep, ep_query, cpt_query, max_nb);
}

int core_unmap_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: unmap array");

	if (array_size(argv) < 2)
		return -1;

	char* local_ep_id = (char*)array_get( argv, 0 );
	char* addr = (char*)array_get( argv, 1 );

	LOCAL_EP* lep = map_get(locales, local_ep_id);
	if (!lep)
		return -2;

	return core_unmap(lep, addr);
}

int core_unmap_connection_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: unmap connection array");

	if (array_size(argv) < 3)
		return -1;

	char* local_ep_id = (char*)array_get( argv, 0 );
	LOCAL_EP* lep = map_get(locales, local_ep_id);
	char* module_name = (char*)array_get( argv, 1 );
	char* conn_str = (char*)array_get( argv, 2 );
	COM_MODULE* module = com_get_module(module_name);
	int conn;
	sscanf(conn_str, "%d", &conn);

	if (!lep || !module || conn<=0)
		return -2;

	return core_unmap_connection(lep, module, conn);
}

int core_unmap_all_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: unmap all array");

	if (array_size(argv) < 1)
		return -1;

	char* local_ep_id = (char*)array_get( argv, 0 );
	LOCAL_EP* lep = map_get(locales, local_ep_id);
	if (!lep)
		return -2;

	return core_unmap_all(lep);
}

int core_divert_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core divert array");

	if (array_size(argv) < 3)
		return -1;

	char* ep_id = (char*)array_get( argv, 0 );
	char* ep_name_from = (char*)array_get( argv, 1 );
	char* addr = (char*)array_get( argv, 2 );

	LOCAL_EP* lep = map_get(locales, ep_id);
	if (!lep)
		return -2;

	return core_divert(lep, ep_name_from, addr);
}

int core_ep_send_message_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core send message array");

	if (array_size(argv) < 3)
		return -1;

	char* ep_id = (char*)array_get( argv, 0 );
	char* msg_id = (char*)array_get( argv, 1 );
	char* msg = (char*)array_get( argv, 2 );

	LOCAL_EP* lep = map_get(locales, ep_id);
	if (!lep)
		return -2;

	return core_ep_send_message(lep, msg_id, msg);
}

int core_ep_send_request_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core send request array");

	if (array_size(argv) < 3)
		return -1;

	char* ep_id = (char*)array_get( argv, 0 );
	char* req_id = (char*)array_get( argv, 1 );
	char* msg = (char*)array_get( argv, 2 );

	LOCAL_EP* lep = map_get(locales, ep_id);
	if (!lep)
		return -2;

	return core_ep_send_request(lep, req_id, msg);
}

int core_ep_send_response_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core send response array");

	if (array_size(argv) < 3)
		return -1;

	char* ep_id = (char*)array_get( argv, 0 );
	char* req_id = (char*)array_get( argv, 1 );
	char* msg = (char*)array_get( argv, 2 );

	LOCAL_EP* lep = map_get(locales, ep_id);
	if (!lep)
		return -2;

	return core_ep_send_response(lep, req_id, msg);
}

int core_ep_more_messages_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_more_messages array");

	if (array_size(argv) < 1)
		return -1;

	char* ep_id = (char*)array_get( argv, 0 );

	LOCAL_EP* lep = map_get(locales, ep_id);
	if (!lep)
		return -2;

	return core_ep_more_messages(lep);
}

int core_ep_more_requests_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_more_requests array");

	if (array_size(argv) < 1)
		return -1;

	char* ep_id = (char*)array_get( argv, 0 );

	LOCAL_EP* lep = map_get(locales, ep_id);
	if (!lep)
		return -2;

	return core_ep_more_requests(lep);
}

int core_ep_more_responses_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_more_responses array");

	if (array_size(argv) < 2)
		return -1;

	char* ep_id = (char*) array_get(argv, 0);
	char* req_id = (char*) array_get(argv, 1);

	LOCAL_EP* lep = map_get(locales, ep_id);
	if (!lep)
		return -2;

	return core_ep_more_responses(lep, req_id);
}

MESSAGE* core_ep_fetch_message_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_receive_message array");
	if (array_size(argv) < 1)
		return NULL;

	char* ep_id = (char*) array_get(argv, 0);

	LOCAL_EP* lep = map_get(locales, ep_id);
	if (!lep)
		return NULL;

	return core_ep_fetch_message(lep);
}

MESSAGE* core_ep_fetch_request_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_receive_request_array");

	if (array_size(argv) < 1)
		return NULL;

	char* ep_id = (char*) array_get(argv, 0);

	LOCAL_EP* lep = map_get(locales, ep_id);
	if (!lep)
		return NULL;

	return core_ep_fetch_request(lep);
}

MESSAGE* core_ep_fetch_response_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_receive_response_array");

	if (array_size(argv) < 2)
		return NULL;

	char* ep_id = (char*)array_get( argv, 0 );
	char* req_id = (char*)array_get( argv, 1 );

	LOCAL_EP* lep = map_get(locales, ep_id);
	if (!lep)
		return NULL;

	return core_ep_fetch_response(lep, req_id);
}

void core_ep_stream_start_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_stream_start_Array");

	if (array_size(argv) < 1)
		return;

	char* ep_id = (char*)array_get( argv, 0 );

	LOCAL_EP *lep = map_get(locales, ep_id);
	if (!lep)
		return;

	core_ep_stream_start(lep);
}

void core_ep_stream_stop_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_stream_stop_array");

	if (array_size(argv) < 1)
		return ;

	char* ep_id = (char*)array_get( argv, 0 );
	LOCAL_EP *lep = map_get(locales, ep_id);

	if (!lep)
		return;

	core_ep_stream_stop(lep);
}

void core_ep_stream_send_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_stream_send_array");

	if (array_size(argv) < 2)
		return ;

	char* ep_id = (char*)array_get( argv, 0 );
	char* msg = (char*)array_get( argv, 1 );
	LOCAL_EP *lep = map_get(locales, ep_id);

	if (!lep)
		return;

	core_ep_stream_send(lep, msg);
}

int core_add_manifest_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_add_manifest_array");

	if (array_size(argv) < 1)
		return -1;

	char* msg = (char*)array_get( argv, 0 );

	return core_add_manifest(msg);
}

char* core_get_manifest_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_get_manifest_array");

	return core_get_manifest();
}

void core_add_rdc_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_add_rdc_array");

	if (array_size(argv) < 2)
		return ;

	char* module_name = (char*)array_get( argv, 0 );
	char* addr = (char*)array_get( argv, 1 );

	COM_MODULE* module = com_get_module(module_name);
	if(module == NULL)
	{
		//slog(SLOG_ERROR, SLOG_ERROR, "CORE FUNC: core_add_rdc_array:\n"
		//	"\tcould not find COM module %s",
		//	module_name);
	}
	else
	{
		core_add_rdc(module, addr);
	}
}

void core_rdc_register_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_rdc_deregister_array");

	if (array_size(argv) < 2)
		core_rdc_register(NULL, NULL) ;
	else
	{
		char* addr = (char*)array_get( argv, 0 );
		char* module_name = (char*)array_get( argv, 1 );
		COM_MODULE* module = com_get_module(module_name);

		core_rdc_register(module, addr);
	}
}

void core_rdc_unregister_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_rdc_deregister_array");

	if (array_size(argv) < 1)
		core_rdc_unregister(NULL) ;
	else
	{
		char* addr = (char*)array_get( argv, 0 );
		core_rdc_unregister(addr);
	}
}

void core_add_filter_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_add_filter_array");

	if (array_size(argv) < 2)
		return ;

	char* ep_id = (char*)array_get( argv, 0 );
	char* filter = (char*)array_get( argv, 1 );

	LOCAL_EP *lep = map_get(locales, ep_id);

	if (!lep)
		return;

	core_add_filter(lep, filter);
}

void core_reset_filter_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_reset_filter_array");

	if (array_size(argv) < 1)
		return ;

	char* ep_id = (char*)array_get( argv, 0 );

	JSON* new_filters_json = NULL;
	Array* new_filters = NULL;

	if (array_size(argv) >= 2)
	{
		new_filters_json = json_new((char*)array_get(argv, 2));
		new_filters = json_get_array(new_filters_json, NULL);
	}

	LOCAL_EP *lep = map_get(locales, ep_id);

	if (!lep)
		return;

	core_reset_filter(lep, new_filters);

	json_free(new_filters_json);
}

void core_ep_set_access_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_set_access_array");

	if (array_size(argv) < 2)
		return;

	char* ep_id = (char*)array_get( argv, 0 );
	char* subject = (char*)array_get( argv, 1 );

	LOCAL_EP *lep = map_get(locales, ep_id);

	core_ep_set_access(lep, subject);
}

void core_ep_reset_access_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_reset_access_array");

	if (array_size(argv) < 2)
		return;

	char* ep_id = (char*)array_get( argv, 0 );
	char* subject = (char*)array_get( argv, 1 );

	LOCAL_EP *lep = map_get(locales, ep_id);

	core_ep_reset_access(lep, subject);
}


char* core_ep_get_all_connections_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_ep_get_all_conections_array");

	if (array_size(argv) < 1)
		return NULL;

	char* ep_id = (char*)array_get( argv, 0 );
	LOCAL_EP *lep = map_get(locales, ep_id);

	char* result = core_ep_get_all_connections(lep);

	return (result);
}

char* core_get_remote_metdata_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_get_remote_metdata_array");

	if (array_size(argv) < 2)
		return NULL;

	char* module_str = (char*)array_get( argv, 0 );
	char* conn_str = (char*)array_get( argv, 1 );
	int conn;
	sscanf(conn_str, "%d", &conn);
	COM_MODULE *module = com_get_module(module_str);
	char* result = core_get_remote_manifest(module, conn);
	return (result);
}

void core_terminate_array(Array* argv)
{
	core_terminate();
}

int core_load_com_module_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: core_load_com_module_array");

	if (array_size(argv) < 1)
		return -1;

	if (array_size(argv) >=2 )
		return core_load_com_module( (char*)array_get( argv, 0 ), (char*)array_get( argv, 1 ));
	else
		return core_load_com_module( (char*)array_get( argv, 0 ), NULL);
}

int core_load_access_module_array(Array* argv)
{
	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: %s", __func__);

	if (array_size(argv) < 1)
		return -1;

	if (array_size(argv) >=2 )
		return core_load_access_module( (char*)array_get( argv, 0 ), (char*)array_get( argv, 1 ));
	else
		return core_load_access_module( (char*)array_get( argv, 0 ), NULL);
}

MESSAGE* _core_call_array(const char* module_id, const char* function_id, const char* return_type, Array* args)
{
	char *fc_id = _core_get_id(module_id, function_id, return_type);

	//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: function call %s %s %s (%s)", module_id, function_id, return_type, fc_id);
	//char *fc_res_str = NULL;
	JSON *fc_res_json = json_new(NULL);
	/* the following are reduntant, just for debug */
	json_set_str(fc_res_json, "function call", fc_id);
	json_set_str(fc_res_json, "return_type", return_type);
	MESSAGE *msg =  NULL;

	if( strcmp(return_type, "void") == 0 )
	{
		void (*fc)(Array*) = map_get(void_function_table_array, fc_id);
		if (fc == NULL)
		{
			//slog(1, SLOG_ERROR, "CORE FUNC: Could not find function id: %s ", fc_id);
			free(fc_id);
			json_free(fc_res_json);
			return NULL;
		}
		(*fc)(args);
		//slog(SLOG_DEBUG, SLOG_DEBUG, "CORE FUNC: function call %s done ",fc_id);

		free(fc_id);
		json_free(fc_res_json);
		return NULL;
	}
	else if(strcmp(return_type, "int") == 0 )
	{
		int (*fc)(Array*) = map_get(int_function_table_array, fc_id);
		if (fc == NULL)
		{
			//slog(1, SLOG_ERROR, "CORE FUNC: Could not find function id: %s", fc_id);
			free(fc_id);
			json_free(fc_res_json);
			return NULL;
		}
		int result = (*fc)(args);
		json_set_int(fc_res_json, "return_value", result);
	}
	else if(strcmp(return_type, "float") == 0 )
	{
		float (*fc)(Array*) = map_get(float_function_table_array, fc_id);
		float result = (*fc)(args);
		json_set_float(fc_res_json, "return_value", result);
	}
	else if(strcmp(return_type, "string") == 0 )
	{
		char* (*fc)(Array*) = map_get(string_function_table_array, fc_id);
		if (fc == NULL)
		{
			//slog(1, SLOG_ERROR, "CORE FUNC: Could not find function id: %s", fc_id);
			free(fc_id);
			json_free(fc_res_json);
			return NULL;
		}
		char* result = (char*) (*fc)(args);
		json_set_str(fc_res_json, "return_value", result);
	}
	else if(strcmp(return_type, "message") == 0 )
	{
		char* (*fc)(Array*) = map_get(message_function_table_array, fc_id);
		if (fc == NULL)
		{
			//slog(1, SLOG_ERROR, "CORE FUNC: Could not find function id: %s", fc_id);
			free(fc_id);
			json_free(fc_res_json);
			return NULL;
		}
		MESSAGE * result = (MESSAGE*) (*fc)(args);
		return result;
	}


	msg = message_new_json(fc_res_json, MSG_CMD);

	free(fc_id);
	json_free(fc_res_json);

	return (msg);
}




int _core_init()
{
	void_function_table_array 	= map_new(KEY_TYPE_STR);
	int_function_table_array 	= map_new(KEY_TYPE_STR);
	float_function_table_array 	= map_new(KEY_TYPE_STR);
	string_function_table_array = map_new(KEY_TYPE_STR);
	message_function_table_array = map_new(KEY_TYPE_STR);

	/************* ARRAY FUNCTIONS *************/

	map_insert(int_function_table_array, _core_get_id("core", "register_endpoint", "int"), core_register_endpoint_array);
	map_insert(void_function_table_array, _core_get_id("core", "remove_endpoint", "void"), core_remove_endpoint_array);

	map_insert(int_function_table_array, _core_get_id("core", "map", "int"), core_map_all_modules_array);
	map_insert(int_function_table_array, _core_get_id("core", "map_module", "int"), core_map_module_array);
	map_insert(void_function_table_array, _core_get_id("core", "map_lookup", "void"), core_map_lookup_array);
	map_insert(int_function_table_array, _core_get_id("core", "unmap", "int"), core_unmap_array);
	map_insert(int_function_table_array, _core_get_id("core", "unmap_connection", "int"), core_unmap_connection_array);
	map_insert(int_function_table_array, _core_get_id("core", "unmap_all", "int"), core_unmap_all_array);
	map_insert(int_function_table_array, _core_get_id("core", "divert", "int"), core_divert_array);

	map_insert(int_function_table_array, _core_get_id("core", "ep_more_messages", "int"), core_ep_more_messages_array);
	map_insert(int_function_table_array, _core_get_id("core", "ep_more_requests", "int"), core_ep_more_requests_array);
	map_insert(int_function_table_array, _core_get_id("core", "ep_more_responses", "int"), core_ep_more_responses_array);

	map_insert(void_function_table_array, _core_get_id("core", "ep_send_message", "void"), core_ep_send_message_array);
	map_insert(void_function_table_array, _core_get_id("core", "ep_send_request", "void"), core_ep_send_request_array);
	map_insert(void_function_table_array, _core_get_id("core", "ep_send_response", "void"), core_ep_send_response_array);

	map_insert(void_function_table_array, _core_get_id("core", "ep_stream_start", "void"), core_ep_stream_start_array);
	map_insert(void_function_table_array, _core_get_id("core", "ep_stream_stop", "void"), core_ep_stream_stop_array);
	map_insert(void_function_table_array, _core_get_id("core", "ep_stream_send", "void"), core_ep_stream_send_array);

	map_insert(message_function_table_array, _core_get_id("core", "ep_fetch_message", "message"), core_ep_fetch_message_array);
	map_insert(message_function_table_array, _core_get_id("core", "ep_fetch_request", "message"), core_ep_fetch_request_array);
	map_insert(message_function_table_array, _core_get_id("core", "ep_fetch_response", "message"), core_ep_fetch_response_array);

	map_insert(void_function_table_array, _core_get_id("core", "add_manifest", "void"), core_add_manifest_array);
	map_insert(string_function_table_array, _core_get_id("core", "get_manifest", "string"), core_get_manifest_array);

	map_insert(void_function_table_array, _core_get_id("core", "add_rdc", "void"), core_add_rdc_array);
	//map_insert(void_function_table_array, _core_get_id("core", "remove_rdc", "void"), core_remove_rdc_array);
	map_insert(void_function_table_array, _core_get_id("core", "rdc_register", "void"), core_rdc_register_array);
	map_insert(void_function_table_array, _core_get_id("core", "rdc_unregister", "void"), core_rdc_unregister_array);

	map_insert(void_function_table_array, _core_get_id("core", "ep_add_filter", "void"), core_add_filter_array);
	map_insert(void_function_table_array, _core_get_id("core", "ep_reset_filter", "void"), core_reset_filter_array);
	map_insert(void_function_table_array, _core_get_id("core", "ep_set_access", "void"), core_ep_set_access_array);
	map_insert(void_function_table_array, _core_get_id("core", "ep_reset_access", "void"), core_ep_reset_access_array);

	map_insert(string_function_table_array, _core_get_id("core", "ep_get_all_connections", "string"), core_ep_get_all_connections_array);
	map_insert(string_function_table_array, _core_get_id("core", "get_remote_metdata", "string"), core_get_remote_metdata_array);

	map_insert(void_function_table_array, _core_get_id("core", "terminate", "void"), core_terminate_array);

	map_insert(int_function_table_array, _core_get_id("core", "load_com_module", "int"), core_load_com_module_array);
	map_insert(int_function_table_array, _core_get_id("core", "load_access_module", "int"), core_load_access_module_array);

	return 0;
}

