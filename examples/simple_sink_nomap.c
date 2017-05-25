/*
 * lookup_sink.c
 *
 *  Created on: 6 Oct 2016
 *      Author: Raluca Diaconu
 */

#include <endpoint.h>
#include <middleware.h>

#include <stdio.h>
#include <unistd.h>


void print_callback(MESSAGE *msg)
{
	ENDPOINT *ep = msg->ep;

	/* parsing the message and extracting the values */
	JSON* elem_msg = json_new(msg->msg);
	int the_number = json_get_int(elem_msg, "the_number");
	char* the_string = json_get_str(elem_msg, "the_string");

	/* process the extracted values */
	printf("Sink ep handler:\n "
			"\t-- number: %d \n "
			"\t-- string: %s\n", the_number, the_string);
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
	char* app_name = mw_init("sink_cpt", 6, 1);

	/* API calls with attached callback for sinks*/
	ENDPOINT *ep_snk = endpoint_new_snk("sink", "example snk endpoint with no map",
			"example_schemata/msg_example.json", &print_callback);

	int load_result = mw_load_com_module(lib_path, cfg_path);
	printf("Load com module result: %d\n", load_result);

	/*sleep(2);

	load_result = mw_load_auth_module(
			"/Users/admin/dev/middleware/modules/auth_modules/libmoduleaclcred.so",
			"/Users/admin/dev/middleware/modules/auth_modules/auth.cfg.json");
	printf("Load auth module result: %d\n", load_result);
*/
	//keeps the sink alive
	while(1)
		sleep(1);

	return 0;
}
