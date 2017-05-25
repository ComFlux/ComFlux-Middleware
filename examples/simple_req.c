/*
 * simple_sink.c
 *
 *  Created on: 7 Sep 2016
 *      Author: Raluca Diaconu
 */

#include <endpoint.h>
#include <middleware.h>
#include <load_mw_config.h>

#include <stdio.h>
#include <unistd.h>
#include <time.h>


void print_callback(MESSAGE *msg)
{
	ENDPOINT *ep = msg->ep;

	/* parsing the message and extracting the values */
	JSON* elem_msg = json_new(msg->msg);
	int value = json_get_int(elem_msg, "value");
	char* datetime = json_get_str(elem_msg, "datetime");

	/* process the extracted values */
	printf("Sink ep handler:\n "
			"\t-- number: %d \n "
			"\t-- string: %s\n", value, datetime);
}

int main(int argc, char* argv[])
{
	char *mw_cfg_path = NULL;
	char *src_addr = NULL;

	if (argc < 3) {
		printf("Usage: ./simple_req [mw_cfg_path] addr\n"
			   "\tmw_cfg_path     is the path to the config file for the middleware;\n"
			   "\t                default mw_cfg.json\n"
			   "\tsink_addr       is the address of the response component\n");

		mw_cfg_path = "mw_cfg.json";
		src_addr = argv[1];

	}
	else
	{
		mw_cfg_path = argv[2];
		src_addr = argv[1];
	}

	/* load and apply configuration */
	int load_cfg_result = load_mw_config(mw_cfg_path);
	printf("Loading configuration: %s\n", load_cfg_result==0?"ok":"error");
	printf("\tApp log level: %d\n", config_get_app_log_lvl());

	/* start core */
	char* app_name = mw_init("request_cpt", config_get_core_log_lvl(), 1);
	printf("Initialising core: %s\n", app_name!=NULL?"ok":"error");
	printf("\tApp name: %s\n", app_name);

	/* load coms modules for the core */
	config_load_com_libs();
	//printf("Load coms module result: %s\n", load_cfg_result==0?"ok":"error");

	/* load access modules for the core */
	load_cfg_result = mw_load_access_module(
			"/Users/admin/dev/middleware/build/lib/libmoduleaclplain.so",
			"/Users/admin/dev/middleware/modules/access_modules/plain/aclplain.cfg.json");
	printf("Loading access module: %s\n", load_cfg_result==0?"ok":"error");


	/* Declare and register endpoints */
	ENDPOINT* ep_req = endpoint_new_req_file(
			"ep_req",
			"example req endpoint",
			"example_schemata/datetime_value.json", /* request schemata */
			"example_schemata/datetime_value.json", /* response schemata */
			&print_callback);

	/* build the query */
	Array* ep_query_array = array_new(ELEM_TYPE_STR);
	array_add(ep_query_array, "ep_name = 'ep_response'");
	JSON* ep_query_json = json_new(NULL);
	json_set_array(ep_query_json, NULL, ep_query_array);
	char* ep_query_str = json_to_str(ep_query_json);
	char* cpt_query_str = "";

	printf("\n\nQuery string: *%s*\n\n", ep_query_str);
	/* map according to the query */
	int map_result = endpoint_map_to(ep_req, src_addr, ep_query_str, cpt_query_str);
	printf("Map result: %d \n", map_result);

	time_t rawtime;
	struct tm* timeinfo;

	JSON* msg_json;
	char* message;
	char* msgid;

	int i = 0;
	while (i < 1000)
	{
		i++;
		sleep(3);

		time ( &rawtime );
		timeinfo = localtime ( &rawtime );

		msg_json = json_new(NULL);
		json_set_int(msg_json, "value", rand() % 10);
		json_set_str(msg_json, "datetime", asctime(timeinfo));

		message = json_to_str(msg_json);
		msgid = endpoint_send_request(ep_req, message);

		printf("Request %s: \n%s\n", msgid, message);

		free(message);
		free(msgid);
        json_free(msg_json);
	}

	return 0;
}
