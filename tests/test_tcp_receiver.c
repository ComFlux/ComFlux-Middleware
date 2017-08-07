/*
 * test_tcp_receiver.c
 *
 *  Created on: 24 Jul 2017
 *      Author: rad
 */


#include <endpoint.h>
#include <middleware.h>
#include <load_mw_config.h>

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <file.h>

#include <sys/time.h>

unsigned int total_msg = 500;

unsigned int started_flag = 0;
unsigned int stopped_flag = 0;

struct timeval time_start;
double time_total = 0;
unsigned int count_msg = 0;


void print_callback(const MESSAGE* msg)
{

	//printf("%d \n", sizeof(message_to_str(msg)));
	if(started_flag == 0 && stopped_flag ==0)
	{
		started_flag = 1;
		//time_start = time(NULL);//clock();
		gettimeofday(&time_start, NULL);
		//return;
	}

	if(started_flag == 1 && stopped_flag == 0
			&& count_msg>=total_msg)
	{
		stopped_flag = 1;
		//time_total = (time(NULL) - time_start);
		struct timeval t1;
		gettimeofday(&t1, NULL);
		time_total = (t1.tv_sec - time_start.tv_sec) * 1000.0;      // sec to ms
		time_total += (t1.tv_usec - time_start.tv_usec) / 1000.0;   // us to ms

		printf("Total: ");
		printf("\n\n nb msg received: total time received: avg \n");
		printf(" %d\t%lf\t%lf\n\n", count_msg, time_total, time_total/count_msg);

	}

	else if(started_flag == 1 && stopped_flag ==0)
	{
		count_msg += 1;
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
		printf("Usage: ./test_tcp_receiver [nbmsg] \n"
				"\tnbmsg              default 500\n");

		return -1;
	}
	}

	printf("\ttotal msg    %d\n", total_msg);


	mw_cfg_path = "3src_mw_cfg.json";

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


	/* build the query *
	Array *ep_query_array = array_new(ELEM_TYPE_STR);
	array_add(ep_query_array, "ep_name = 'ep_sink'");
	JSON *ep_query_json = json_new(NULL);
	json_set_array(ep_query_json, NULL, ep_query_array);
	char* ep_query_str = json_to_str(ep_query_json);
	char* cpt_query_str = "";

	char* addr = text_load_from_file("snk_mqtt.cfg.json");
	/* map according to the query *
	int map_result = endpoint_map_to(ep_snk,
			addr, ep_query_str, cpt_query_str);
	printf("Map result: %d \n", map_result);
    */

    while(stopped_flag == 0)
    {
    	sleep(2);

    	//printf("\n\n nb msg received: %d \ntotal time received %lf \n", count_msg, ((double)time_total));
    	//printf("avg:  %lf\n", (time_total/(double)count_msg));
    }

	mw_terminate_core();

	printf("Total: ");
	printf("\n\n nb msg received: total time received: avg \n");
	printf(" %d\t%lf\t%lf\n\n", count_msg, time_total, time_total/count_msg);

	return 0;
}
