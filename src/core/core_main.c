/*
 * core_main.c
 *
 *  Created on: 1 Mar 2016
 *	  Author: Raluca Diaconu
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include "core.h"
#include "environment.h"
#include "json.h"
#include <slog.h>
#include <limits.h> //for PATH_MAX

int fd;
char* app_name;
char* app_key;
char* config_file;

void print_usage_exit()
{
	printf("CORE: Bad arguments formatting. Exiting ...");
	printf("Usage: core -f SOCKET_FD -a APP_NAME -k KEY -c CONFIG_FILE");
	exit(1);
}

int load_core_config(const char* config_file)
{
	JSON* config_json = json_load_from_file(config_file);
	if (config_json == NULL)
		return -1;

	JSON* core_json = json_get_json(config_json, "core_config");
	if(core_json == NULL)// TODO: validate against a schema
		return -2;

	int log_lvl = json_get_int(core_json, "log_level");
	char* log_file = json_get_str(core_json, "log_file");
	if(log_file == NULL)
	{
		log_file = malloc(PATH_MAX+1);
		sprintf(log_file, "log/core_%s_%s", app_name, app_key);
	}

	slog_init_args(log_lvl, log_lvl, 1, 1, log_file);

	return 0;
}

#ifdef __ANDROID__ /* On Android we build as a library (and I can't
					  figure out how exclude this file). */
int main_(int argc, char *argv[])
#else // __ANDROID__
int main(int argc, char *argv[])
#endif //__ANDROID__
{
	if (chdir(ETC) == -1)
	{
		printf("Failed to change directory.\n");
		return -1;  /* No use continuing */
	}

	if( access( "log/", W_OK ) == -1) {
		mkdir ("log", 0777);
	}

	// Check if the core has been called with the right number of args.
	if(argc < 9)
		print_usage_exit();

	// Check arguments one by one.
	if (strcmp(argv[1], "-f"))
		print_usage_exit();
	if (sscanf(argv[2], "%d", &fd) != 1)
		print_usage_exit();

	if(strcmp(argv[3], "-a"))
		print_usage_exit();
	app_name = argv[4];

	if(strcmp(argv[5], "-k"))
		print_usage_exit();
	app_key = argv[6];

	if(strcmp(argv[7], "-c"))
		print_usage_exit();
	if(load_core_config(argv[8]))
		print_usage_exit();

	// We have at least the correct number of params.
	slog(SLOG_INFO, SLOG_INFO, "CORE MAIN: Starting core with args:\n"
			"\t%s %s \n"
			"\t%s %s \n"
			"\t%s %s \n"
			"\t%s %s",
			argv[1], argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);


	if(fd != 0)
	{
		if (core_init_fd(fd, argv[4], argv[6]))
			return EXIT_FAILURE;
	}

	// Don't stop: just do something.
	while(1)
		sleep(1);

	return EXIT_SUCCESS;
}
