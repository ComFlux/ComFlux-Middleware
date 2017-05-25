/*
 * main.c
 *
 *  Created on: 15 Aug 2016
 *      Author: Raluca Diaconu
 */


#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <middleware.h>
#include <load_mw_config.h>
#include <endpoint.h>

#include "toolbox.h"
#include "environment.h"
#include <json.h>

#define A_MAP			1
#define A_MAP_LOOKUP	2
#define A_TERMINATE		3
#define A_ADD_RDC		4
#define A_RDC_LIST		5

#define A_REQ			6
#define A_MSG			7

#define A_HELP			0

int action = 0; // default get help action
char *target_addr = NULL;
char *target_query = NULL;

char *remote_addr = NULL;
char *remote_query = NULL;

char* msg_schema = NULL;
char* resp_schema = NULL;

char* msg = NULL;

FILE* f_out = NULL;


char* result = NULL;
char* error = NULL;

void print_error_args()
{
	if (isprint (optopt))
		printf("Invalid option received %c\n", optopt);
}

void print_help_args()
{
	printf("Usage: \n");
	printf(" > swiss_knife \n"
			"\t-a {msg | req | map | map_lookup | terminate | add_rdc | rdc_list} \n"
			"\t-T {target_addr | rdc_addr}"
			"\t-t <target ep query> \n"
			"\t-R <remote_addr>\t"
			"\t-r <remote ep query> \n"
			"\t-m <request schema filename> "
			"\t-n <response schema filename> \n"
			"\t-M <json message> \n"
			"\t-f <filename> \n");

	printf("Examples: \n");
	printf(" > swiss_knife -a msg -T \"127.0.0.1:1502\" -m msg_term.json -t \"[\\\"ep_name = 'TERMINATE'\\\"]\" -M {} \n");
	printf(" > swiss_knife -a terminate -T \"127.0.0.1:1502\" \n");
	printf(" > swiss_knife -a add_rdc -T \"127.0.0.1:1502\" -R \"127.0.0.1:1508\" \n");
}


void execute_cmd()
{
	switch(action)
	{
		case A_MAP:
			map(target_addr, target_query, remote_addr, remote_query);
			break;

		case A_MAP_LOOKUP:
			map_lookup(target_addr, target_query, remote_query);
			break;

		case A_TERMINATE:
			terminate(target_addr);
			break;

		case A_ADD_RDC:
			add_rdc(target_addr, remote_addr);
			break;

		case A_RDC_LIST:
			// TODO: refactoring!!
			result = malloc(7000*sizeof(char));
			rdc_list(target_addr, result);
			break;

		case A_REQ:
			one_shot_req(target_addr, msg_schema, resp_schema, target_query, msg);
			break;

		case A_MSG:
			one_shot_msg(target_addr, msg_schema, target_query, msg);
			break;

		case A_HELP:
			print_help_args();
	}

}

int parse_opt(int argc, char **argv)
{
	int opt = 0;
	//char *in_fname = NULL;
	//char *out_fname = NULL;

	while ((opt = getopt(argc, argv, "a:T:t:R:r:m:n:M:f:")) != -1)
	{
		switch(opt)
		{
			case 'a':
				if(strcmp(optarg, "map")==0)
					action = A_MAP;
				else if(strcmp(optarg, "map_lookup")==0)
					action = A_MAP_LOOKUP;
				else if(strcmp(optarg, "terminate")==0)
					action = A_TERMINATE;
				else if(strcmp(optarg, "add_rdc")==0)
					action = A_ADD_RDC;
				else if(strcmp(optarg, "rdc_list")==0)
					action = A_RDC_LIST;
				else if(strcmp(optarg, "req")==0)
					action = A_REQ;
				else if(strcmp(optarg, "msg")==0)
					action = A_MSG;
				else
				{
					action = A_HELP;
					print_error_args();
					return 1;
				}

				printf("Action=%d\n", action);
				break;

			case 'T':
				target_addr = optarg;
				printf("Target addr=%s\n", target_addr);
			break;

			case 't':
				target_query = optarg;
				printf("Target query=%s\n", target_query);
				break;

			case 'R':
				remote_addr = optarg;
				printf("Remote addr=%s\n", remote_addr);
				break;

			case 'r':
				remote_query = optarg;
				printf("Remote query=%s\n", remote_query);
				break;

			case 'm':
				msg_schema = optarg;
				printf("Message schema=%s\n", msg_schema);
				break;

			case 'n':
				resp_schema = optarg;
				printf("Response schema=%s\n", resp_schema);
				break;

			case 'M':
				msg = optarg;
				printf("Message=%s\n", msg);
				break;

			case 'f':
				f_out = fopen(optarg, "w");
				printf("File=%s\n", optarg);
				break;

			case '?':
				/* Case when user enters the command as
				 * $ ./cmd_exe -i
				 * # ./cmd_exe -o
				 *  etc
				 */
				if (optopt == 'a')
				{
					printf("Missing mandatory action option\n");
				}
				else if (optopt == 'T')
				{
					printf("Missing mandatory target addr option\n");
				}
				else if (optopt == 'R' && action == A_MAP)
				{
					printf("Missing mandatory remote option for map action\n");
				}
				else
				{
					print_error_args();
				}
				break;
		}
	}


	return 0;
}

int main(int argc, char **argv)
{
	char* sk_cfg_file;
	if (chdir(ETC) == -1)
	{
	    printf("Failed to change directory.\n");
	    return -1;  /* No use continuing */
	}

#ifdef __linux__
	sk_cfg_file = "/usr/local/etc/middleware/swiss_knife/swiss_knife.cfg.json";
#elif __APPLE__
	sk_cfg_file = "/usr/local/etc/middleware/swiss_knife/swiss_knife.cfg.json";
#endif

	load_mw_config(sk_cfg_file);
	char* app_name = mw_init(""
			"SK",
			config_get_app_log_lvl(),
			true);

	printf("Initialising core: %s\n", app_name!=NULL?"ok":"error");
	printf("\tApp name: %s\n", app_name);

	int load_cfg_result = config_load_com_libs();
	printf("Load coms module result: %s\n", load_cfg_result==0?"ok":"error");

	//printf("good ...\n");
	terminate("127.0.0.1:1508");
	//add_rdc("127.0.0.1:1505", "127.0.0.1:1508");
	//map("127.0.0.1:1505", "[\"ep_name = 'sink'\"]", "127.0.0.1:1503", "");
	//map_lookup("127.0.0.1:1501", "[\"ep_name = 'sink'\"]", "ep_name = 'source'");
	//rdc_list("127.0.0.1:1508");
	//sleep(10);

sleep(1);
	// Argument parsing
	parse_opt(argc, argv);

	// Do the job
	execute_cmd();

	if(result)
	{
		if(f_out != NULL)
		{
			fprintf(f_out, "%s\n", result);
			fclose(f_out);
		}
		else
			printf("%s", result);
	}

	mw_terminate_core();
	return 0;
}

// cc -o swiss_knife swiss_knife.c toolbox.c ../common/array.c ../common/json.c -I"../apilib" -I"../common" -lpthread -L/usr/local/lib -lwjelement -lwjreader -lslog -L"../apilib" -lapilib

