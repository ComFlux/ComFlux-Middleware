/*
 * core.h
 *
 *  Created on: 13 Jun 2016
 *      Author: rad
 */

#ifndef CORE_CORE_H_
#define CORE_CORE_H_


/*
 * Initialises core. Starts app server.
 * Sets up the addr variables.
 * starts the server and the client. (because the api is already running)
 */
int core_init(const char* app_name, const char* app_key);
int core_init_fd(int fd, const char* app_name, const char* app_key);

#endif /* CORE_CORE_H_ */
