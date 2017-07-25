#include <endpoint.h>
#include <middleware.h>
#include <load_mw_config.h>

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <file.h>


unsigned int nb_msg = 500;

unsigned int time_total = 0;
unsigned int count_msg = 0;

int main(int argc, char *argv[])
{
	char *mw_cfg_path = NULL;
	char *src_addr = NULL;

	if(argc<2)
	{
		printf("Usage: ./test_tcp_sender [mw_cfg_path]\n"
				"\tmw_cfg_path       is the path to the config file for the middleware;\n"
				"\t                  default mw_cfg.json\n"
				"\treceiver_addr     is the address of the receiver we want to map to\n");

		mw_cfg_path = "2src_mw_cfg.json";
	}
	else
	{
		mw_cfg_path=argv[1];
	}

	/* load and apply configuration */
	int load_cfg_result = load_mw_config(mw_cfg_path);
	printf("Loading configuration: %s\n", load_cfg_result==0?"ok":"error");
	printf("\tApp log level: %d\n", config_get_app_log_lvl());

	/* start core */
	char* app_name = mw_init("sender_tcp_cpt", config_get_core_log_lvl(), 1);
	printf("Initialising core: %s\n", app_name!=NULL?"ok":"error");
	printf("\tApp name: %s\n", app_name);

	/* load coms modules for the core */
	load_cfg_result = config_load_com_libs();
	printf("Load coms module result: %s\n", load_cfg_result==0?"ok":"error");

	sleep(1);


	/* Declare and register endpoints */

	ENDPOINT *ep_src = endpoint_new_src_file(
			"ep_source",
			"example src endpoint",
			"example_schemata/datetime_value.json");


	/* build the query */
	Array *ep_query_array = array_new(ELEM_TYPE_STR);
	array_add(ep_query_array, "ep_name = 'ep_sink'");
	JSON *ep_query_json = json_new(NULL);
	json_set_array(ep_query_json, NULL, ep_query_array);
	char* ep_query_str = json_to_str(ep_query_json);
	char* cpt_query_str = "";


	/* build a message */
	JSON* msg_json = json_new(NULL);
	json_set_int(msg_json, "value", rand() % 10);
	json_set_str(msg_json, "date", "today");

	char* message = json_to_str(msg_json);


	char* addr = text_load_from_file("src_mqtt.cfg.json");
	int map_result = endpoint_map_to(ep_src,
				"127.0.0.1:1505", ep_query_str, cpt_query_str);
	printf("Map result: %d \n", map_result);

	unsigned int i;

	for(i=0; i<nb_msg; i++)
	{
		endpoint_send_message(ep_src, message);
		count_msg += 1;
		time_total += clock();
	}

	sleep(1);

	return 0;
}
