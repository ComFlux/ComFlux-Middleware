/*
 * toolbox.c
 *
 *  Created on: 15 Aug 2016
 *      Author: Raluca Diaconu
 */


#include <unistd.h>
#include <array.h>
#include <middleware.h>
#include <endpoint.h>

#include "toolbox.h"


int operation_done = 0;
char* data = NULL;

void print_json_handler(MESSAGE* msg)
{
	JSON* resp_json = json_new(msg->msg);

	printf("\tresponse:*** %s\n***\n", json_to_str_pretty(resp_json));
	if(msg->msg)
		data = strdup(msg->msg);
	operation_done = 1;
}



void terminate(const char* addr)
{
	printf("\nAdd Terminate \n\tcpt: %s \n",
			addr);

	/* Instantiate ep. */
	ENDPOINT *ep_term = endpoint_new_src_file(
			"tool terminate",
			"example terminate endpoint",
			"swiss_schemata/msg_term.json");


	int map_result = endpoint_map_to(ep_term, addr, "[\"ep_name = 'TERMINATE'\"]", "");
	printf("\tMap result: %d \n", map_result);
	if(map_result != 0)
	{
		printf("\tMapping failed.\n");
		return;
	}

	/* Generates core function calls */
	endpoint_send_message(ep_term, "{}");

	//core_unmap_all(ep_term);
	endpoint_free(ep_term);
}

void add_rdc(const char* cpt_addr, const char* rdc_addr)
{
	printf("\nAdd RDC addr to \n\tcpt: %s \n\trdc: %s\n",
			cpt_addr, rdc_addr);

	/* Instantiate ep. */
	ENDPOINT *ep_add_rdc = endpoint_new_src_file(
			"tool add rdc",
			"example add rdc endpoint",
			"swiss_schemata/msg_add_rdc.json");

	int map_result = endpoint_map_to(ep_add_rdc, cpt_addr, "[\"ep_name = 'ADD_RDC'\"]", "");
	printf("\tMap result: %d \n", map_result);
	if(map_result != 0)
	{
		printf("\tMapping failed.\n");
		return;
	}

	/* Generates core function calls */
	char* add_rdc_json = (char*)malloc(20);
	sprintf(add_rdc_json, "{\"address\" : \"%s\"}", rdc_addr);

	endpoint_send_message(ep_add_rdc, add_rdc_json);

	//sleep(1);
	//core_unmap_all(ep_add_rdc);
	endpoint_free(ep_add_rdc);
}

void map(const char* from_addr, const char* from_query, const char* dest_addr, const char* dest_query)
{
	printf("\nMapping: \n\tcpt: %s \n\tquery: %s \n\tto: %s \n\tquery: %s\n",
			from_addr, from_query, dest_addr, dest_query);

	/* Instantiate ep. */
	ENDPOINT *map_ep = endpoint_new_req_file(
			"tool map",
			"connects to map endpoint",
			"rdc_schemata/map_msg_example.json",
			"rdc_schemata/map_resp_example.json",
			NULL);

	if(map_ep == NULL)
	{
		printf("ERROR: map ep can not be instantiated. Are the json files with the right path?\n");
		return;
	}

	int map_result = endpoint_map_to(map_ep, from_addr, "[\"ep_name = 'MAP'\"]", "");
	printf("\tMap result: %d \n", map_result);
	if(map_result != 0)
	{
		printf("\tMapping failed.\n");
		return;
	}


	/* build the map request */
	JSON *from_query_json = json_new(from_query);
	printf("\t** %s *** \n", json_to_str_pretty(from_query_json));
	Array* from_query_array = json_get_array(from_query_json, NULL);

	JSON *dest_query_json = json_new(dest_query);
	Array* dest_query_array = json_get_array(dest_query_json, NULL);

	JSON* map_json = json_new(NULL);
	json_set_array(map_json, "endpoint", from_query_array);
	json_set_str(map_json, "address", dest_addr);
	json_set_array(map_json, "query", dest_query_array);

	printf("request; %s\n", json_to_str(map_json));

	//MESSAGE* resp =
	endpoint_send_request_blocking(map_ep, json_to_str(map_json));
	//printf("\tresponse: %s\n", message_to_str(resp));

	//sleep(1);
	//core_unmap_all(map_ep);
}


void map_lookup(const char* from_addr, const char* from_query, const char* dest_query)
{
	printf("\nMapping: \n\tcpt: %s \n\tquery: %s \n\tdest query: %s",
			from_addr, from_query, dest_query);

	/* Instantiate ep. */
	ENDPOINT *map_ep = endpoint_new_req_file(
			"tool map lookup",
			"connects to map lookup endpoint",
			"swiss_schemata/map_lookup_msg_example.json",
			"swiss_schemata/map_lookup_resp_example.json",
			&print_json_handler);

	int map_result = endpoint_map_to(map_ep, from_addr, "[\"ep_name = 'MAP LOOKUP'\"]", "");
	printf("\tMap result: %d \n", map_result);
	if(map_result != 0)
	{
		printf("\tMapping failed.\n");
		return;
	}


	/* build the map request */
	JSON *from_query_json = json_new(from_query);
	printf("\t** %s *** \n", json_to_str_pretty(from_query_json));
	Array* from_query_array = json_get_array(from_query_json, "");

	JSON *dest_query_json = json_new(dest_query);
	Array* dest_query_array = json_get_array(dest_query_json, "");

	JSON* map_json = json_new(NULL);
	json_set_array(map_json, "endpoint", from_query_array);
	json_set_array(map_json, "query", dest_query_array);

	MESSAGE* resp = endpoint_send_request_blocking(map_ep, json_to_str(map_json));
	printf("\tresponse: %s\n", message_to_str(resp));

	//sleep(1);
	//core_unmap_all(map_ep);
}


// TODO: ep query/ cpt query
void unmap_all(const char* cpt_addr, const char* query)
{
	printf("\nUnapping: \n\tcpt: %s \n\tquery: %s\n",
			cpt_addr, query);

	/* Instantiate ep. */
	ENDPOINT *map_ep = endpoint_new_req_file(
			"tool unmap",
			"connects to unmap endpoint",
			"swiss_schemata/unmap_msg_example.json",
			"swiss_schemata/unmap_resp_example.json",
			NULL);

	int map_result = endpoint_map_to(map_ep, cpt_addr, query, "");
	printf("\tMap result: %d \n", map_result);
	if(map_result != 0)
	{
		printf("\tMapping failed.\n");
		return;
	}

}

void rdc_list(const char* rdc_addr, char* result)
{
	printf("\nRDC list: \n\trdc: %s \n",
				rdc_addr);

	/* Instantiate ep. */
	ENDPOINT *list_ep = endpoint_new_req_file(
			"tool list",
			"connects to rdc list endpoint",
			"swiss_schemata/list_msg.json",
			"swiss_schemata/list_resp.json",
			&print_json_handler);

	int map_result = endpoint_map_to(list_ep, rdc_addr, "[\"ep_name = 'list'\"]", "");
	printf("\tMap result: %d \n", map_result);
	if(map_result != 0)
	{
		printf("\tMapping failed.\n");
		return;
	}

	sleep(1);
	endpoint_send_request(list_ep, "{}");

	while(operation_done == 0)
	{
		sleep(1);
	}

	sprintf(result, "%s", data);

	endpoint_unmap_all(list_ep);
}

void swiss_load_com_module(const char* target_addr,
		const char* com_module_path, const char* com_module_config)
{
	printf("\nLoad com module \n"
			"\ttarget: %s \n"
			"\tname: %s \n"
			"\tconfig: %s\n",
			target_addr, com_module_path, com_module_config);

	/* Instantiate ep. */
	ENDPOINT *ep_load_com_module = endpoint_new_src_file(
			"tool load com module",
			"example load com module endpoint",
			"swiss_schemata/load_com_module_msg.json");

	int map_result = endpoint_map_to(ep_load_com_module, target_addr, "[\"ep_name = 'LOAD_COM_MODULE'\"]", "");
	printf("\tMap result: %d \n", map_result);
	if(map_result != 0)
	{
		printf("\tMapping failed.\n");
		return;
	}

	/* Generates core function calls */
	char* load_com_module_json = (char*)malloc(20);
	sprintf(load_com_module_json, "{\"com_module_path\" : \"%s\", \"com_module_config\" : \"%s\"}",
			com_module_path, com_module_config);

	endpoint_send_message(ep_load_com_module, load_com_module_json);

	//sleep(1);
	//core_unmap_all(ep_add_rdc);
	endpoint_free(ep_load_com_module);
}

void swiss_load_auth_module(const char* target_addr,
		const char* com_module_path, const char* com_module_config)
{}

void one_shot_req(
		const char* addr,
		const char* msg_schema_path, const char* resp_schema_path,
		const char* map_query,
		const char* req)
{
	//char* req_str = text_load_from_file(req_path);
	printf("\nGeneral req: \n\t"
			"addr: %s \n\t"
			"req schema: %s\n\t"
			"resp schema: %s\n\t"
			"map query: %s\n\t"
			"req: %s\n\n",
			addr, msg_schema_path, resp_schema_path, map_query, req);

	/* check if the file exists */
	/* Instantiate ep. */
	ENDPOINT *req_ep = endpoint_new_req_file(
			"tool req",
			"connects to a resp endpoint",
			msg_schema_path,
			resp_schema_path,
			&print_json_handler);

	int map_result = endpoint_map_to(req_ep, addr, map_query, "");
	printf("\tMap result: %d \n", map_result);
	if(map_result != 0)
	{
		printf("\tMapping failed.\n");
		return;
	}

	MESSAGE* resp = endpoint_send_request_blocking(req_ep, req);
	printf("\tresponse: %s\n", message_to_str(resp));

	//sleep(1);
	//core_unmap_all(req_ep);
}

void one_shot_msg(
		const char* addr,
		const char* msg_schema_path,
		const char* map_query,
		const char* msg)
{
	//char* req_str = text_load_from_file(req_path);
	printf("\nGeneral message: \n\t"
			"addr (-T): %s \n\t"
			"msg schema path (-m): %s\n\t"
			"map query (-t): %s\n\t"
			"msg (-M): %s\n\n",
			addr, msg_schema_path, map_query, msg);

	/* Instantiate ep. */
	ENDPOINT *req_ep = endpoint_new_src_file(
			"tool msg",
			"connects to a sink endpoint",
			msg_schema_path);

	int map_result = endpoint_map_to(req_ep, addr, map_query, "");
	printf("\tMap result: %d \n", map_result);
	if(map_result != 0)
	{
		printf("\tMapping failed.\n");
		return;
	}

	endpoint_send_message(req_ep, msg);
	endpoint_unmap_all(req_ep);
}
