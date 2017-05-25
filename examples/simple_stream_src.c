/*
 * simple_stream_src.c
 *
 *  Created on: 21 Feb 2017
 *      Author: Raluca Diaconu
 */



#include <endpoint.h>
#include <middleware.h>

#include <stdio.h>
#include <unistd.h>
#include <time.h>


int main(int argc, char *argv[])
{
	/* check if the source has been called with the right number of args */
	char *lib_path = NULL, *cfg_path = NULL;

	if(argc<3)
	{
		printf("Usage: ./simple_stream_source lib_path cfg_path\n"
				"\tlib_path     is the path to the com library; default /Users/admin/dev/middleware/modules/com_modules/libtcpmodule.so \n"
				"\tcfg_path		is the path to the config file for the com lib; default /Users/admin/dev/middleware/modules/com_modules/tcp_snk.cfg.json\n");

		lib_path = "/Users/admin/dev/middleware/modules/com_modules/libtcpmodule.so";
		cfg_path = "/Users/admin/dev/middleware/modules/com_modules/tcp_src.cfg.json";
		//exit(0);
	}
	else
	{
		lib_path = argv[1];
		cfg_path = argv[2];
	}

	/* start server */
	char* app_name = mw_init("stream_source_cpt", 6, 1);
	printf("core init: %s\n", app_name);

	int load_result = mw_load_com_module(lib_path, cfg_path);
	printf("Load com module result: %d\n", load_result);
	//sleep(2);
/*
	load_result = mw_load_auth_module(
			"/Users/admin/dev/middleware/modules/auth_modules/libmoduleaclcred.so",
			"/Users/admin/dev/middleware/modules/auth_modules/auth.cfg.json");
	printf("Load auth module result: %d\n", load_result);
	sleep(2);

	return 0;
*/
	JSON* metadata = json_new(NULL);
	json_set_str(metadata, "app_name", "simple_stream_cpt");
	json_set_str(metadata, "author", "raluca");

	mw_add_manifest(json_to_str(metadata));


	/* Instantiate an ep. Generates core function calls */
	ENDPOINT *ep_src = endpoint_new_stream_src("stream_source", "example src endpoint");

	/* seeding random number generator */
	srand(time(NULL));

	/* initialise time struct */
	time_t rawtime;
	struct tm * timeinfo;
	char buf[1024];

	/* forever: send data */
	while(1)
	{
		sleep(3);

		time ( &rawtime );
		timeinfo = localtime ( &rawtime );
		sprintf(buf, "<<%d, %s>>\n", rand()%10, asctime (timeinfo));

		printf("Sending message: \n%s\n", buf);
		endpoint_send_message(ep_src, buf);

		//MESSAGE* msg_start = message_new(NULL, MSG_STREAM_CMD);
		endpoint_start_stream(ep_src);

	}

	return 0;
}
