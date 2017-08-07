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

unsigned int total_msg = 1000000;

unsigned int started_flag = 0;
unsigned int stopped_flag = 0;

struct timeval time_start;
double time_total = 0;
unsigned int count_msg = 0;

int totalSize=0;

void print_callback(MESSAGE *msg)
{
	started_flag = 1;
	gettimeofday(&time_start, NULL);

	int stream_fd = fifo_init_client(msg->msg_id);
	printf("init:%d  stream: %s\n", stream_fd, msg->msg_id);
	int rcvSize;
	char buf[1001];
	while(1)
	{
		rcvSize = read(stream_fd, &buf, 1000);
		if(rcvSize>0){
			totalSize+=rcvSize;
			//printf("recv size: %d\n", rcvSize);
		}
		if(totalSize>total_msg)
			break;
	}


	stopped_flag = 1;

	struct timeval t1;
	gettimeofday(&t1, NULL);
	time_total = (t1.tv_sec - time_start.tv_sec) * 1000.0;      // sec to ms
	time_total += (t1.tv_usec - time_start.tv_usec) / 1000.0;   // us to ms

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

	ENDPOINT *ep_snk = endpoint_new_stream_snk(
			"ep_stream_sink", /* name */
			"example snk endpoint", /* description */
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
    	sleep(5);

    	printf("\n\n total size received: %d \ntotal time received %lf \n", totalSize, ((double)time_total));
    	//printf("avg:  %lf\n", (time_total/(double)count_msg));
    }

	sleep(1);
	printf("Total: ");
	printf("\n\n total size received: %d \ntotal time received %lf \n", totalSize, ((double)time_total));
	//printf("avg:  %lf\n", (time_total/(double)count_msg));

	return 0;
}
