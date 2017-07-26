/*
 * test_mqtt_receiver.c
 *
 *  Created on: 25 Jul 2017
 *      Author: Raluca Diaconu
 */



#include <endpoint.h>
#include <middleware.h>
#include <load_mw_config.h>

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <file.h>


unsigned int total_msg = 500;

unsigned int started_flag = 0;
unsigned int stopped_flag = 0;

unsigned int time_start = 0;
unsigned int time_total = 0;
unsigned int count_msg = 0;


void print_callback(MESSAGE *msg)
{
	//printf("%s -- %s\n", msg->msg_id, msg->msg);
	if(started_flag == 0 && stopped_flag ==0)
	{
		started_flag = 1;
		time_start = clock();
		//return;
	}

	if(started_flag == 1 && stopped_flag == 0
			&& count_msg>=total_msg)
	{
		stopped_flag = 1;
	}

	else if(started_flag == 1 && stopped_flag ==0)
	{
		count_msg += 1;
		time_total += (clock() - time_start);
	}
}

int main(int argc, char *argv[])
{
	char *mw_cfg_path = NULL;


	printf("argc: %d\n", argc);
	switch (argc)
	{
	case 1: break;
	case 2:
	{
		total_msg=atoi(argv[1]);
		break;
	}
	default:
	{
		printf("Usage: ./test_mqtt_receiver [nbmsg] \n"
				"\tnbmsg              default 500\n");

		return -1;
	}
	}
	printf("\tnbmsg    %d\n", total_msg);
	mw_cfg_path = "1src_mw_cfg.json";


	/* load and apply configuration */
	int load_cfg_result = load_mw_config(mw_cfg_path);
	printf("Loading configuration: %s\n", load_cfg_result==0?"ok":"error");
	printf("\tApp log level: %d\n", config_get_app_log_lvl());

	/* start core */
	char* app_name = mw_init("sink_cpt", config_get_core_log_lvl(), 1);
	printf("Initialising core: %s\n", app_name!=NULL?"ok":"error");
	printf("\tApp name: %s\n", app_name);

	/* load coms modules for the core */
	load_cfg_result = config_load_com_libs();
	printf("Load coms module result: %s\n", load_cfg_result==0?"ok":"error");

	sleep(1);


	/* Declare and register endpoints */

	ENDPOINT *ep_snk = endpoint_new_snk_file(
			"ep_sink", /* name */
			"example snk endpoint", /* description */
			"example_schemata/datetime_value.json", /* message schemata */
			&print_callback); /* handler for incoming messages */


	/* build the query */
	Array *ep_query_array = array_new(ELEM_TYPE_STR);
	array_add(ep_query_array, "ep_name = 'ep_sink'");
	JSON *ep_query_json = json_new(NULL);
	json_set_array(ep_query_json, NULL, ep_query_array);
	char* ep_query_str = json_to_str(ep_query_json);
	char* cpt_query_str = "";

	char* addr = text_load_from_file("snk_mqtt.cfg.json");
	/* map according to the query */
	int map_result = endpoint_map_to(ep_snk,
			addr, ep_query_str, cpt_query_str);
	printf("Map result: %d \n", map_result);


    while(stopped_flag == 0)
    {
    	sleep(2);

    	printf("\n\n nb msg received: %d \ntotal time received %d \n", count_msg, time_total - time_start);
    	printf("avg:  %f\n",  (time_total/(float)count_msg)/ CLOCKS_PER_SEC);
    }

	sleep(1);
	printf("Total: ");
	printf("\n\n nb msg received: %d \ntotal time received %d \n", count_msg, time_total - time_start);
	printf("avg:  %f\n",  (time_total/(float)count_msg)/ CLOCKS_PER_SEC);

	return 0;
}
