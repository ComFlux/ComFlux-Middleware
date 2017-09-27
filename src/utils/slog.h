/*
 * slog.h
 *
 *  Created on: 21 Sep 2016
 *      Author: Raluca Diaconu
 */

#ifndef SRC_UTILS_SLOG_H_
#define SRC_UTILS_SLOG_H_

#include <stdlib.h>

/* Loging flags */
#define SLOG_NONE   0
#define SLOG_LIVE   1
#define SLOG_INFO   2
#define SLOG_WARN   3
#define SLOG_DEBUG  4
#define SLOG_ERROR  5
#define SLOG_FATAL  6
#define SLOG_PANIC  7

/*
 * parses a json config file;
 */
void slog_init_file(const char* cfg_filename);

/*
 * init with all params passed as args
 */
void slog_init_args(int log_level_console, int log_level_file,
		int log_timestamp, int log_colors,
		const char* log_filename);
/*
 * only console, log_level and output file
 */
void slog_init_simple(int log_level, const char* log_filename);

void slog(int lvl, const char* format, ...);

#endif /* SRC_UTILS_SLOG_H_ */
