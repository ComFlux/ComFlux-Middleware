/*
 * core.c
 *
 *  Created on: 13 Jun 2016
 *      Author: Raluca Diaconu
 */

#include "core.h"

#include "hashmap.h"
#include "message.h"
#include "endpoint.h"
#include "core_module.h"

#include "core_module_api.h"
#include "json_builds.h"
#include "state.h"
#include "protocol.h"
#include "default_eps.h"
#include "com_wrapper.h"
#include "access_wrapper.h"
#include "rdcs.h"
#include "slog.h"
#include "utils.h"

#include "manifest.h"
#include "core_callbacks.h"

#include <signal.h>
#include <pthread.h>
#include <dlfcn.h>

#include <sys/types.h>
#include <sys/socket.h>

/* passing messages between threads to sync during mapping protocol */
int map_sync_pipe[2];
int rdc_register_pipe[2];

char *app_name, *app_key;

STATE* app_state;

void register_default_endpoints();

void int_handler(int sig)
{
	core_terminate();
}

int core_init(const char* _app_name, const char* _app_key)
{
	struct sigaction int_action;
	memset(&int_action, 0, sizeof(int_action));
	int_action.sa_handler = int_handler;
	sigaction(SIGINT, &int_action, NULL);

	/* init state functionality */
	init_states();

	/* init endpoints */
	eps_init();

	/* init rdc container and functionality */
	rdcs_init();

	/* inits module "core" function tables with all fcs in core_module */
	_core_init();

	json_load_all_file_schemas();
	register_default_endpoints();

	/* connection module */
	init_com_wrapper();

	/* access control and auth modules */
	init_access_wrapper();

	app_name = strdup_null(_app_key);
	app_key = strdup_null(_app_key);

	JSON * app_md = json_new(NULL);
	json_set_str(app_md, "app_name", app_name);
	manifest_update(app_md);

	return EXIT_SUCCESS;
}

int core_init_fd(int fd, const char* _app_name, const char* _app_key)
{
	slog(SLOG_INFO, SLOG_INFO, "CORE: init_fd: %d, %s", fd, _app_name);

	COM_MODULE* fd_module=NULL;

	// Run default set up.
	int ret = core_init(_app_name, _app_key);
	if (ret != EXIT_SUCCESS)
		return ret;

	char sockpair_config[100];
	sprintf(sockpair_config, "{\"is_server\":0, \"fd\": %d }", fd);

#ifdef __linux__
	fd_module = com_module_new(
			"/usr/local/etc/middleware/com_modules/libcommodulesockpair.so",
			sockpair_config);
#elif __APPLE__
	fd_module = com_module_new(
			"/usr/local/etc/middleware/com_modules/libcommodulesockpair.so",
			sockpair_config);
#endif

	(*(fd_module->fc_set_on_data))((void (*)(void *, int, const char *))core_on_data);
	(*(fd_module->fc_set_on_connect))((void (*)(void *, int))core_on_connect);
	(*(fd_module->fc_set_on_disconnect))((void (*)(void *, int))core_on_disconnect);


	STATE* state_ptr = state_new(fd_module, fd, STATE_FIRST_MSG);
	state_ptr->on_message = &core_on_first_message;

	app_state = state_ptr;

	states_set(fd_module, fd, state_ptr);

	sprintf(sockpair_config, "{%s}", app_name);
	(*(fd_module->fc_send_data))(fd, sockpair_config);

	return EXIT_SUCCESS;
}

