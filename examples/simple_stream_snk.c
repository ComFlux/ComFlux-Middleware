/*
 * simple_stream_snk.c
 *
 *  Created on: 22 Feb 2017
 *      Author: Raluca Diaconu
 */

#include <endpoint.h>
#include <middleware.h>

#include <stdio.h>
#include <unistd.h>

void print_callback(MESSAGE *msg)
{
	/* process the extracted values */
	printf("Sink ep handler: %s\n ", msg->msg);
}


int main(int argc, char *argv[])
{
	char *lib_path = NULL, *cfg_path = NULL;
	char *src_addr = NULL;

	if(argc<4)
	{
		printf("Usage: ./simple_sink lib_path cfg_path\n"
				"\tlib_path     is the path to the com library; default /Users/admin/dev/middleware/modules/com_modules/libtcpmodule.so \n"
				"\tcfg_path		is the path to the config file for the com lib; default /Users/admin/dev/middleware/modules/com_modules/tcp_snk.cfg.json\n");

		lib_path = "/Users/admin/dev/middleware/modules/com_modules/libtcpmodule.so";
		cfg_path = "/Users/admin/dev/middleware/modules/com_modules/tcp_snk.cfg.json";

		src_addr = argv[1];
		//exit(0);
	}
	else
	{
		lib_path = argv[1];
		cfg_path = argv[2];
		src_addr = argv[3];
	}

	/* start server */
	char* app_name = mw_init("stream_sink_cpt", 6, 1);

	/* API calls with attached callback for sinks*/
	ENDPOINT *ep_snk = endpoint_new_stream_snk("sink", "example stream snk endpoint",
			 &print_callback);

	//sleep(1);

	Array *ep_query_array = array_new(ELEM_TYPE_STR);
	array_add(ep_query_array, "ep_name = 'stream_source'");
	JSON *ep_query_json = json_new(NULL);
	json_set_array(ep_query_json, "", ep_query_array);
	char* ep_query_str = json_to_str(ep_query_json);
	char* cpt_query_str = "";

	printf("\n\n*%s*\n\n", ep_query_str);

	int load_result = mw_load_com_module(lib_path, cfg_path);
	printf("Load com module result: %d\n", load_result);
/*	sleep(2);

	load_result = mw_load_auth_module(
			"/Users/admin/dev/middleware/modules/auth_modules/libmoduleaclcred.so",
			"/Users/admin/dev/middleware/modules/auth_modules/auth.cfg.json");
	printf("Load auth module result: %d\n", load_result);
	sleep(2);
*/
	int map_result = endpoint_map_to(ep_snk, src_addr, ep_query_str, cpt_query_str);
	printf("Map result: %d \n", map_result);
	extern int stream_fd_global;
	//keeps the sink alive
	while(1)
	{
		printf("****%d\n", stream_fd_global);
		sleep(1);
	}

	//endpoint_add_filter(ep_snk, "the_number > 5");
	//endpoint_reset_filter(ep_snk, NULL);

	return 0;
}
