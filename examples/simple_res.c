/*
 * simple_source.c
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


ENDPOINT* ep_resp;


void print_callback(MESSAGE* msg) {
	/* seeding random number generator */
	srand(time(NULL));

	/* initialise time struct */
	time_t rawtime;
	struct tm* timeinfo;
	char* reqid = "";

	if (msg != NULL)
	{
		printf("Recved req: %s\n", json_to_str(msg->_msg_json));
		reqid = msg->msg_id;
	}

	/* build and send response */

	JSON* msg_json;
	char* message;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	msg_json = json_new(NULL);
	json_set_int(msg_json, "value", rand() % 10);
	json_set_str(msg_json, "datetime", asctime(timeinfo));

	message = json_to_str(msg_json);
	endpoint_send_response(ep_resp, msg->msg_id, message);

	free(message);
    json_free(msg_json);
}

int main(int argc, char* argv[]) {

	char *mw_cfg_path = NULL;

	if(argc<2)
	{
		printf("Usage: ./simple_response [mw_cfg_path]\n"
				"\tmw_cfg_path		is the path to the config file for the middleware;\n"
				"\t                 default mw_cfg.json\n"
				"\tsink_addr        is the address of the sink\n");

		mw_cfg_path = "mw_cfg.json";
	}
	else
	{
		mw_cfg_path = argv[1];
	}

	/* load and apply configuration */
	int load_cfg_result = load_mw_config(mw_cfg_path);
	printf("Loading configuration: %s\n", load_cfg_result==0?"ok":"error");
	printf("\tApp log level: %d\n", config_get_app_log_lvl());

	/* start core */
	char* app_name = mw_init("response_cpt", config_get_core_log_lvl(), 1);
	printf("Initialising core: %s\n", app_name!=NULL?"ok":"error");
	printf("\tApp name: %s\n", app_name);

	/* load coms modules for the core */
	load_cfg_result = config_load_com_libs();
	printf("Load coms module result: %s\n", load_cfg_result==0?"ok":"error");

	/* load access modules for the core */
	load_cfg_result = mw_load_access_module(
			"/Users/admin/dev/middleware/build/lib/libmoduleaclplain.so",
			"/Users/admin/dev/middleware/modules/access_modules/plain/aclplain.cfg.json");
	printf("Loading access module: %s\n", load_cfg_result==0?"ok":"error");


	/* Instantiate an ep. Generates core function calls */
	ep_resp = endpoint_new_resp_file("ep_response",
			"example res endpoint",
			"example_schemata/datetime_value.json", /* request schemata */
			"example_schemata/datetime_value.json", /* response schemata */
			&print_callback);

	/* forever: wait for reqiests */
	while (1)
		sleep(1);

	return 0;
}
