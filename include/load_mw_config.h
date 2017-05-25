/*
 * load_mw_config.h
 *
 *  Created on: 1 Mar 2017
 *      Author: Raluca Diaconu
 *
 * @author Raluca Diaconu
 * @brief Configuration for the MW API lib and core
 */

#ifndef SRC_API_LOAD_MW_CONFIG_H_
#define SRC_API_LOAD_MW_CONFIG_H_

#include <array.h>

/**
 * @brief Load and apply configurations from a config file.
 *
 * @param cfg_file_path
 * 		Relative/absolute path to the json configuration file.
 *
 * @return
 * 		0 in case of success or error code otherwise.
 */
int load_mw_config(const char* cfg_file_path);

/**
 * @brief Change the path at which the application is running.
 *
 * @param path
 * 		Relative/absolute path to the json configuration file.
 *
 * @return
 * 		0 in case of success or error code otherwise.
 */
int change_home_dir(const char* path);

/**
 * @brief Load an array of libs.
 *
 * @param libs_array
 * 		Relative/absolute path to the json configuration file.
 *
 * @return
 * 		0 in case of success or error code otherwise.
 */
int load_libs_array(Array* libs_array, int (*load_module)(const char*, const char*));


int config_get_app_log_lvl();

int config_get_core_log_lvl();

char* config_get_app_log_file();

char* config_get_core_log_file();

Array* config_get_com_libs_array();

Array* config_get_access_libs_array();

char* config_get_absolute_path();

int config_load_com_libs();

int config_load_access_libs();

#endif /* SRC_API_LOAD_MW_CONFIG_H_ */
