/*
 * rdc_app.c
 *
 *  Created on: 23 May 2016
 *      Author: Raluca Diaconu
 */

#include "environment.h"

#include <stdio.h>
#include <unistd.h>
#include <middleware.h>
#include <endpoint.h>
#include <hashmap.h>
#include <json_filter.h>
#include <pthread.h>

#include <load_mw_config.h>

#define RDC_REGISTER 	4
#define RDC_UNREGISTER 	5

ENDPOINT *ep_snk_register = NULL;
ENDPOINT *ep_req_getmd = NULL;
ENDPOINT *ep_resp_lookup = NULL;
ENDPOINT *ep_resp_list = NULL;

typedef struct _Cpt{
	JSON *cpt_md; /* hashtags */
	char* app_id; /* TODO: remove either app_id or module+addr */

	char* module;
	char* addr;

	Array *ep_md;

	Array *com_modules;
	Array *access_modules;
}Cpt;

//Cpt* cpt_new()
void cpt_free(Cpt* cpt)
{
	json_free(cpt->cpt_md);
	array_free(cpt->ep_md);
	free(cpt);
}

HashMap *cpts = NULL;

void at_exit()
{
	int i;
	char* key;
	Cpt* cpt;
	Array* keys = map_get_keys(cpts);
	for(i = 0; i<array_size(keys); i++)
	{
		key = array_get(keys, i);
		cpt = map_get(cpts, (void*)key);
		cpt_free(cpt);
	}

	map_free(cpts);
	exit(0);
}

void* rdc_getmd_request(void* closure)
{
	JSON* metadata_json = (JSON*) closure;

	Array* com_modules = json_get_jsonarray(metadata_json, "com_modules");
	JSON* module_json = array_get(com_modules, 0);
	char *module = json_get_str(module_json, "com_module");
	char *address = json_get_str(module_json, "address");


	int map_result = endpoint_map_module(ep_req_getmd, module, address, "", "");  // Add remote ep name.
	if (map_result == 0)
	{
		printf("mapping to get_md from app %s successful.\n", address);
		char* req_id = endpoint_send_request(ep_req_getmd, "[]");
		free(req_id);
	}
	else
		printf("mapping to get_md from app %s unsuccessful. code: %d\n", address, map_result);

	printf("Cpts size: %d, ",  map_size(cpts));

	//free(app_id);
	return NULL;
}

void rdc_register_handler(MESSAGE *msg)
{
	printf("\nRegistration message received:\n");

	JSON* registration_json = msg->_msg_json;

	char* reg_json_pretty = json_to_str_pretty(registration_json);
	printf("%s\n\n", reg_json_pretty);
	free(reg_json_pretty);

	int flag = json_get_int(registration_json, "flag");

	switch (flag) {
		case RDC_REGISTER: {
			printf("Register %s %d\n", msg->module, msg->conn);
			//printf("Remote md: %s\n", mw_get_remote_metdata(msg->module, msg->conn));
			JSON* metadata_json = json_get_json(registration_json, "manifest");
            JSON* component_json = json_get_json(metadata_json, "component");
			char *app_id = json_get_str(metadata_json, "app_name");
			char *module = json_get_str(metadata_json, "module");
			char *address = json_get_str(metadata_json, "address");
			endpoint_unmap_all(ep_snk_register);

			pthread_t getmd_thread;
			pthread_create(&getmd_thread, NULL, rdc_getmd_request, metadata_json);


            break;
		}

		case RDC_UNREGISTER: {
			printf("Unregister\n");
			JSON* metadata_json = json_get_json(registration_json, "manifest");
            JSON* component_json = json_get_json(metadata_json, "component");
            char *app_id = json_get_str(metadata_json, "app_name");


			printf("App id %s\n", app_id);
			map_remove(cpts, (void*)app_id);

            json_free(component_json);
            json_free(metadata_json);

			break;
		}

		default: {
			printf("Unrecognized flag\n");
		}
	}

	printf("Cpts size: %d\n", map_size(cpts));
	//json_free(registration_json);
	//mw_unmap_all(ep_snk_register);
}


void rdc_getmd_handler(MESSAGE *msg)
{
	printf("\nMetadata received:\n");
	JSON* manifest_json = msg->_msg_json;
	printf("%s\n\n", json_to_str_pretty(manifest_json));

	Cpt* cpt = (Cpt*)malloc(sizeof(Cpt));
	cpt->app_id = json_get_str(manifest_json, "app_name");
	cpt->cpt_md = json_get_json(manifest_json, "component");

	cpt->com_modules = json_get_jsonarray(manifest_json, "com_modules");
	cpt->access_modules = json_get_jsonarray(manifest_json, "access_modules");


	cpt->ep_md = json_get_jsonarray(manifest_json, "endpoints");

	//cpt->module = json_get_str(manifest_json, "module");
	//cpt->addr = json_get_str(manifest_json, "address");

	if(cpt->addr == NULL || cpt->ep_md == NULL || cpt->com_modules == NULL)
	{
		printf("Malformed message, no enpoints, addr or modules provided\n"
				"messafe is:\n"
				"%s\n", json_to_str_pretty(msg->_msg_json));
		return;
	}

	int j;
	JSON* ep_md = NULL;
	for(j = 0; j<array_size(cpt->ep_md); j++)
	{
		ep_md = array_get(cpt->ep_md, j);
		json_merge(ep_md, cpt->cpt_md);
	}

	map_update(cpts, (void*)cpt->app_id, (void*)cpt);
	printf("Added to cpt table: \naddr: %s \ncpt: %s \nepts:%d\n\n",
			cpt->app_id, json_to_str_pretty(cpt->cpt_md), array_size(cpt->ep_md));

	printf("Cpts size: %d\n", map_size(cpts));
}

void rdc_lookup_handler(MESSAGE *msg)
{
	JSON* lookup_json = msg->_msg_json;

	printf("\nLookup request received: %d\n", map_size(cpts));
	printf("%s\n\n", json_to_str_pretty(lookup_json));

	Array* lookup_array = json_get_array(lookup_json, "ep_query");

	int i, j;
	Array* keys = map_get_keys(cpts);
	char* key;
	Cpt* cpt;
	JSON* cpt_ep_md;
	Array* results = array_new(ELEM_TYPE_PTR); //ids

	Array* ep_com_modules_array;
	JSON* ep_com_modules_json;
	//for each component
	for(i = 0; i<array_size(keys); i++)
	{
		key = array_get(keys, i);
		cpt = map_get(cpts, key);
		//for each endpoint
		for(j = 0; j<array_size(cpt->ep_md); j++)
		{
			cpt_ep_md = array_get(cpt->ep_md, j);
			ep_com_modules_array = json_get_jsonarray(cpt_ep_md, "com_modules");
			ep_com_modules_json = json_new(NULL);
			json_set_array(ep_com_modules_json, "", ep_com_modules_array);
			if(json_filter_validate_array(cpt_ep_md, lookup_array))
			{
				printf("--- found %s \n", json_to_str_pretty(cpt_ep_md));
				printf(">>>>>> %s\n", json_to_str_pretty(ep_com_modules_json));
				array_add(results, cpt_ep_md);
				break;
			}
			else
			{
				printf("--- %s \n", json_to_str_pretty(cpt_ep_md));
				printf(">>>>>> %s\n", json_to_str_pretty(ep_com_modules_json));
			}
		}
	}

	JSON* result = json_new(NULL);
	json_set_array(result, "results", results);

	char* json_str = json_to_str(result);

	printf("\n\nResult: %s\n\n", json_str);
	endpoint_send_response(ep_resp_lookup, msg->msg_id, json_to_str(result));

	// Cleanup:
	free(json_str);
	//json_free(lookup_json);
	json_free(result);
	array_free(keys);
	array_free(results);
}


void rdc_list_handler(MESSAGE *msg)
{
	//check if all mappings have been cleared
	//Array* connections =
	//		ep_get_all_connections(ep_resp_list);
	//printf("all connections size = %d\n\n", array_size(connections));

	printf("\nList request received:\n");
	JSON* list_json = msg->_msg_json;

	char* list_str = json_to_str_pretty(list_json);
	printf("\n\n%s\n\n", list_str);
	free(list_str);

	int i;
	Array* keys = map_get_keys(cpts);
	char* key;
	Cpt* cpt;
	Array* results = array_new(ELEM_TYPE_PTR); //addresses
	JSON* cpt_json;
	printf("There are %d cpts.\n", array_size(keys));
	for (i = 0; i < array_size(keys); i++) {
		key = array_get(keys, i);
		cpt = map_get(cpts, (void*)key);

		cpt_json = json_new(NULL);
		json_set_json(cpt_json, "component", cpt->cpt_md);
		json_set_array(cpt_json, "endpoints", cpt->ep_md);
		array_add(results, cpt_json);
	}

	JSON* result = json_new(NULL);
	json_set_array(result, "", results);


	char* json_str = json_to_str(result);

	printf("\n\nResult: %s\n\n", json_str);
	endpoint_send_response(ep_resp_list, msg->msg_id, json_to_str(result));

	// Cleanup:
	free(json_str);
	//json_free(list_json);
	json_free(result);
	array_free(keys);
	array_free(results);
}

int parse_rdc_cfg_file(const char* cfg_file)
{
	printf("loading rdc cfg file: %s\n", cfg_file);
	JSON* config_json = json_load_from_file(cfg_file);
	if (config_json == NULL)
		return -1;


	return 0;
}

int main(int argc, char* argv[])
{

	if (chdir(ETC) == -1)
	{
	    printf("Failed to change directory.\n");
	    return -1;  /* No use continuing */
	}

	char* rdc_cfg_file;
	if(argc<2)
	{
		printf("Usage: ./rdc [rdc_cfg_file]\n"
				"\trdc_cfg_file  is the path to the RDC config file;\n"
				"\t              default /usr/local/etc/middleware/rdc/rdc.cfg.json\n");

#ifdef __linux__
		rdc_cfg_file = "/usr/local/etc/middleware/rdc/rdc.cfg.json";
#elif __APPLE__
		rdc_cfg_file = "/usr/local/etc/middleware/rdc/rdc.cfg.json";
#endif

	}
	else
	{
		rdc_cfg_file = argv[1];
	}

	int result = load_mw_config(rdc_cfg_file);
	cpts = map_new(KEY_TYPE_STR);

	/* start RDC MW */
	char* app_name = mw_init("RDC", 6, 1);
	printf("core init: %s\n", app_name);
	result = config_load_com_libs();
	printf(">>>%d\n\n", result);
	config_load_access_libs();

	/* apply all in config file */
	if(parse_rdc_cfg_file(rdc_cfg_file) != 0)
	{
		printf("Error parsing RDC config file; exiting\n");
		mw_terminate_core();
		at_exit();
		return -1;
	}

	/* instantiate endpoints */
	ep_snk_register = endpoint_new_snk_file(
			"register",
			"register cpt to rdc",
			"rdc_schemata/rdc_register.json",
			&rdc_register_handler);


	ep_req_getmd = endpoint_new_req_file(
			"rdc req",
			"rdc get md",
			"rdc_schemata/rdc_getmd_req.json",
			"rdc_schemata/rdc_getmd_resp.json",
			&rdc_getmd_handler);


	ep_resp_lookup = endpoint_new_resp_file(
			"lookup",
			"rdc lookup",
			"rdc_schemata/rdc_lookup_msg.json",
			"rdc_schemata/rdc_lookup_resp.json",
			&rdc_lookup_handler);

	ep_resp_list = endpoint_new_resp_file(
			"list", "rdc list",
			"rdc_schemata/list_msg.json",
			"rdc_schemata/list_resp.json",
			&rdc_list_handler);



	Array* connections =
			ep_get_all_connections(ep_resp_list);
	printf("all connections size = %d\n\n", array_size(connections));


	while(1)
		sleep(1);

	return 0;
}
