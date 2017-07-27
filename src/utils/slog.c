/*
 * slog.c
 *
 *  Created on: 21 Sep 2016
 *      Author: Raluca Diaconu
 */

#include "slog.h"
#include <time.h>
#include <json.h>
#include <utils.h>

/* colors */
#define CLR_NONE    "\x1B[0m"
#define CLR_LIVE  	"\x1B[34m"
#define CLR_ERROR  	"\x1B[31m"
#define CLR_DEBUG  	"\x1B[32m"
#define CLR_WARN  	"\x1B[33m"
#define CLR_INFO  	"\x1B[34m"
#define CLR_FATAL  	"\x1B[35m"
#define CLR_PANIC  	"\x1B[36m"
#define CLR_WHT  	"\x1B[37m"


#define SLOG_MAX_MSG 2058

#define SLOG_ESC ": "


struct {
	int initilised;

	int log_level_console;
	int log_level_file;
	int log_timestamp;
	int log_colors;
	int log_console;

	char* log_filename;
} _slog_config = {1, 6, 6, 1, 1, 1, 0};

//char* log_filename = NULL;


/* timestamp */
char* slog_timestamp()
{
	static time_t rawtime;
	static struct tm * timeinfo;
	static char buffer[23]; //max size of date in this format

	time (&rawtime);
	timeinfo = localtime (&rawtime);

    strftime (buffer, 23,"%F:%T", timeinfo);
    return buffer;
}

const char* slog_color(int lvl)
{
	static char* slog_color[] = {CLR_NONE, CLR_LIVE, CLR_INFO, CLR_WARN, CLR_DEBUG, CLR_ERROR, CLR_FATAL, CLR_PANIC};
	if (lvl > 0 && lvl < 8)
		return slog_color[lvl];
	else
		return slog_color[0];
}

const char* slog_lvl_name(int lvl)
{
	static char* slog_names[] = {"", "[LIVE]", "[INFO]", "[WARN]", "[DEBUG]", "[ERROR]", "[FATAL]", "[PANIC]"};
	if (lvl > 0 && lvl < 8)
		return slog_names[lvl];
	else
		return slog_names[0];
}
void slog_init_file(const char* cfg_filename)
{
	JSON * cfg_json = json_load_from_file(cfg_filename);
	if (cfg_json == NULL)
		return;

	JSON* slog_cfg_json = json_get_json(cfg_json, "console_cfg");
	_slog_config.log_level_console = json_get_int(slog_cfg_json, "log_level_console");
	_slog_config.log_level_file = json_get_int(slog_cfg_json, "log_level_file");
	_slog_config.log_timestamp = json_get_int(slog_cfg_json, "log_timestamp");
	_slog_config.log_colors = json_get_int(slog_cfg_json, "log_colors");
	_slog_config.log_filename = json_get_str(slog_cfg_json, "log_filename");
	_slog_config.initilised = 1;
}

void slog_init_args(int log_level_console, int log_level_file,
		int log_timestamp, int log_colors,
		const char* log_filename)
{
	_slog_config.log_level_console = log_level_console;
	_slog_config.log_level_file = log_level_file;
	_slog_config.log_timestamp = log_timestamp;
	_slog_config.log_colors = log_colors;
	_slog_config.initilised = 1;

	_slog_config.log_filename = strdup_null(log_filename);

	slog(SLOG_INFO, SLOG_INFO,
			"Init with args : %d %d %d %d %s\n",log_level_console,  log_level_file,
			 log_timestamp,  log_colors,
			 log_filename);
}

void slog_init_simple(int log_level, const char* log_filename)
{
	_slog_config.log_level_console = log_level;
	_slog_config.log_filename = strdup_null(log_filename);
	if (log_filename == NULL)
	{
		_slog_config.log_level_file = log_level;
	}
	else
	{
		_slog_config.log_level_file = log_level;
	}

	slog(SLOG_INFO, SLOG_INFO,
			"Init simple : %d %s\n",log_level,
			 log_filename);
}

void slog(int lvl, int lvl_2, const char* format, ...)
{
}

