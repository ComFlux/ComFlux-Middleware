/*
 * lookup_source.c
 *
 *  Created on: 6 Oct 2016
 *      Author: Raluca Diaconu
 */

#include <endpoint.h>
#include <middleware.h>
#include <load_mw_config.h>

#include <stdio.h>
#include <unistd.h>
#include <time.h>

int main(int argc, char *argv[])
{
	char *mw_cfg_path = NULL;

	if (argc < 2)
	{
		printf(
				"Usage: ./simple_source mw_cfg_path\n"
						"\tmw_cfg_path		is the path to the config file for the middleware;\n"
						"\t                 default mw_cfg.json\n");

		mw_cfg_path = "mw_cfg.json";
		//exit(0);
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
	char* app_name = mw_init(
			"source_lookup_cpt",
			config_get_app_log_lvl(),
			true);
	printf("Initialising core: %s\n", app_name!=NULL?"ok":"error");
	printf("\tApp name: %s\n", app_name);

	/* load coms modules for the core */
	load_cfg_result = config_load_com_libs();
	printf("Load coms module result: %s\n", load_cfg_result==0?"ok":"error");

	/* declare and upload a manifest
	 * this will be sent to the RDC and other connected components
	 */
	JSON* manifest = json_new(NULL);
	json_set_str(manifest, "app_name", app_name);
	json_set_str(manifest, "author", "raluca");
	mw_add_manifest(json_to_str(manifest));

	/* Instantiate an ep. Generates core function calls */
	ENDPOINT *ep_src = endpoint_new_src_file(
			"ep_source",
			"example src endpoint",
			"example_schemata/datetime_value.json");

	printf("\nAdding RDC at address 127.0.0.1:1508\n");
	mw_add_rdc("comtcp", "127.0.0.1:1508");
	mw_register_rdcs();
	sleep(5);

	/* seeding random number generator */
	srand(time(NULL));

	/* build and send messages every 3 secs */
	time_t rawtime;
	struct tm * timeinfo;

	JSON* msg_json;
	char* message;

	/* forever: send data */
	while (1)
	{
		sleep(13);

		time ( &rawtime );
		timeinfo = localtime ( &rawtime );

		msg_json = json_new(NULL);
		json_set_int(msg_json, "value", rand() % 10);
		json_set_str(msg_json, "datetime", asctime(timeinfo));

		message = json_to_str(msg_json);
		printf("Sending message: \n%s\n", message);
		endpoint_send_message(ep_src, message);

        free(message);
        json_free(msg_json);
	}

	return 0;
}
