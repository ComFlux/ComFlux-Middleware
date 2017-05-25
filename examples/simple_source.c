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

#include <stdlib.h>

int main(int argc, char *argv[])
{
	/* check if the source has been called with the right number of args */
	char *mw_cfg_path = NULL;

	if(argc<2)
	{
		printf("Usage: ./simple_source mw_cfg_path\n"
				"\tmw_cfg_path		is the path to the config file for the middleware;\n"
				"\t                 default mw_cfg.json\n");

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
	char* app_name = mw_init(
			"source_cpt", /* name of the app */
			config_get_app_log_lvl(), /* log level for app */
			1); /* use sockpair, only available option atm */
	printf("Initialising core: %s\n", app_name!=NULL?"ok":"error");
	printf("\tApp name: %s\n", app_name);

	/* load coms modules for the core */
	load_cfg_result = config_load_com_libs();
	printf("Load coms module result: %s\n", load_cfg_result==0?"ok":"error");

	/* load access modules for the core */
/*	load_cfg_result = mw_load_access_module(
			"/Users/admin/dev/middleware/build/lib/libmoduleaclplain.so",
			"/Users/admin/dev/middleware/modules/access_modules/plain/aclplain.cfg.json");
	printf("Loading access module: %s\n", load_cfg_result==0?"ok":"error");
*/
	/* Declare and register endpoints */
	ENDPOINT *ep_src = endpoint_new_src_file(
			"ep_source",
			"example src endpoint",
			"example_schemata/datetime_value.json");

	/* seeding random number generator */
	srand(time(NULL));

	/* build and send messages every 3 secs */
	time_t rawtime;
	struct tm * timeinfo;

	JSON* msg_json;
	char* message;

	Array* connections = NULL;
    int i;
    JSON* conn_json;
    char* module;
    int conn;
    char* metadata;

	/* forever: send data */
	while(1)
	{
		sleep(3);

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

        /* get and mapped components */
        connections = ep_get_all_connections(ep_src);
        printf("Number of connections = %d\n", array_size(connections));

        for (i=0; i<array_size(connections); i++)
        {
        	conn_json = array_get(connections, i);
        	module = json_get_str(conn_json, "module");
        	conn = json_get_int(conn_json, "conn");
        	metadata = mw_get_remote_metdata(module, conn);
        	printf("\t#%d: connection %d, module %s, manifest: %s\n",
        			i, conn, module, metadata);

        	free(metadata);
        	free(module);
        	json_free(conn_json);
        }

        array_free(connections);
	}

	return 0;
}
