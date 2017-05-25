/*
 * environment.h
 *
 *  Created on: 29 May 2016
 *      Author: Raluca Diaconu
 *
 *  This file contains declarationsfor the environment and version.
 */

#ifndef COMMON_ENVIRONMENT_H_
#define COMMON_ENVIRONMENT_H_

/* define some macros */
#ifndef NULL
#define NULL ((void*)0)
#endif

#define VERSION "0.0.1"  /* protocol version */

#define APP_KEY_SIZE 10  /* number of chars in session_id */
#define EP_UID_SIZE  10  /* number of chars in ep->id */

/* OS specific environmental variables */
#ifdef __ANDROID__

#elif __APPLE__ // __ANDROID_
#define ETC "/usr/local/etc/middleware/"
#define BIN "/usr/local/bin/"

#else // __APPLE__
#define ETC "/usr/local/etc/middleware/"
#define BIN "/usr/local/bin/"
#endif // __ANDROID__


/* True if the same version */
#define validate_version(version) !strcmp(version, VERSION)


#endif /* COMMON_ENVIRONMENT_H_ */
