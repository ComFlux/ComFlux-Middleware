#include <endpoint.h>
#include <middleware.h>
#include <load_mw_config.h>

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <file.h>


char receiver_addr[200] = "34.253.191.67";
unsigned int receiver_port = 1505;

unsigned int nb_msg = 500;

unsigned int time_total = 0;
unsigned int count_msg = 0;
char* file_to_str(const char* filename) {

	FILE* _file = fopen(filename, "r");

	if (_file == NULL)
		return NULL;

	/* get size of file */
	fseek(_file, 0L, SEEK_END);
	int size = (int) ftell(_file);
	rewind(_file);

	/* read file */
	char* var;
	var = (char*) malloc((size_t) size + 1);
	memset(var, 0, (size_t) size + 1);

	fread(var, 1, (size_t) size, _file);
	var[size] = '\0';
	fclose(_file);

	/* return text */
	return var;
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
		nb_msg=atoi(argv[1]);
		break;
	}
	case 3:
	{
		strcpy(receiver_addr, argv[1]);
		receiver_port = atoi(argv[2]);
		break;
	}
	case 4:
	{
		strcpy(receiver_addr, argv[1]);
		receiver_port = atoi(argv[2]);
		nb_msg=atoi(argv[3]);
		break;
	}
	default:
	{
		printf("Usage: ./test_tcp_sender [receiver_addr receiver_port] [nbmsg] \n"
				"\treceiver_addr      default 34.229.95.129;\n"
				"\treceiver_port      default 1505\n"
				"\tnbmsg              default 500\n");

		return -1;
	}
	}

	printf("\treceiver_addr: %s\n"
			"\treceiver_port: %d\n"
			"\tnbmsg    %d\n", receiver_addr, receiver_port, nb_msg);
	mw_cfg_path = "2src_mw_cfg.json";
	char receiver_full[110];
	sprintf(receiver_full, "%s:%d", receiver_addr, receiver_port);


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
	char* cpt_query_str = "[]";


	/* build a message */
	JSON* msg_json = json_new(NULL);
	json_set_str(msg_json, "value",  "41.24\'12.2\"N 2.10'26.5\"E");
	json_set_str(msg_json, "date",   "2012-04-23T18:25:43.511Z");

//	char* lorem = file_to_str("lorem.txt");
//	json_set_str(msg_json, "lorem", lorem);

//	printf("%d: %s\n", strlen(lorem), lorem);


	/* sleep */
	 struct timespec sleep_time;
	 sleep_time.tv_sec = 0;
	 sleep_time.tv_nsec=1000000L;

	//char* addr = text_load_from_file("src_mqtt.cfg.json");
	int map_result = endpoint_map_to(ep_src,
			receiver_full, ep_query_str, cpt_query_str);
	printf("Map result: %d \n", map_result);

	sleep(3);
	endpoint_send_message_json(ep_src, msg_json);

	unsigned int i;

	for(i=0; i<nb_msg; i++)
	{
		nanosleep(&sleep_time, NULL);
		endpoint_send_message_json(ep_src, msg_json);
		count_msg += 1;
		//time_total += clock();
	}

	sleep(1);

	mw_terminate_core();

	return 0;
}
