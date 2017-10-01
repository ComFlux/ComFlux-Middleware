/**
 * @file middleware.h
 * @author Raluca Diaconu
 * @brief Middleware API
 */

#ifndef MIDDLEWARE_H_
#define MIDDLEWARE_H_

#include <stdbool.h>

/**
 * @brief Initialise values and communication threads, spawn the core layer in
 * another process and connect to it. Call this before using any other library functions.
 *
 * @param app_name
 *		A name that identifies the current app.
 *
 * @param log_level
 *		Log-level of the middleware, based on [slog log levels](https://github.com/kala13x/slog).
 *
 * @param use_socketpair
 *		false - Port is used.
 *		true  - Creates a pair of local UNIX sockets for IPC, and port is ignored.
 *
 * @return Address of the (running) middleware instance.
 *
 */
char* mw_init(const char* app_name, int log_level, bool use_socketpair);

/**
 * @brief Gracefully terminate the connected core. If this is not called, the
 * core will terminate itself with an error exit code on disconnecting from the
 * library. **Note:** fails silently if there is no connected core.
 *
 */
void mw_terminate_core();

/**
 * @brief Merge a new manifest JSON string with the middleware's copy,
 * overwriting where conflicts are found.
 *
 * @param manifest
 *		Metadata to merge with that of the middleware.
 */
void mw_add_manifest(const char* manifest);

/**
 * @brief Retrieve from the core and return a copy of the component manifest.
 */
const char* mw_get_manifest();

/**
 * @brief Invoke a function on the core, passing parameters.  Send **only**
 * strings as variable arguments.
 *
 * @param module_id
 *		Module name. Currently core.
 *
 * @param function_id 
 *		Function external identifier or name.
 *
 * @param return_type
 *		Expected type of the return value.
 *		Can be int, float, string, MESSAGE,
 *		or void, if no response is expected.
 *
 * @return Status code (0 = OK), if the message was sent successfully.
 */
int mw_call_module_function(
		const char* module_id,
		const char* function_id,
		const char* return_type,
		...);

/**
 * @brief Invoke a function on the core, passing parameters.  Send **only**
 * strings as variable arguments.
 *
 * @param module_id
 *		Module name. Currently core.
 *
 * @param function_id 
 *		Function external identifier or name.
 *
 * @param return_type
 *		Expected type of the return value.
 *		Can be int, float, string, MESSAGE,
 *		or void, if no response is expected.
 *
 * @return Status code (0 = OK), if the message was sent successfully.
 */
void* mw_call_module_function_blocking(
		const char* module_id,
		const char* function_id,
		const char* return_type,
		...);


/**
 * @brief Add the address of an RDC to the middleware. The middleware will then
 * use this RDC to discover remote components when performing mw_map_lookup().
 *
 * @param address
 *		Address of the RDC to add.
 */
void mw_add_rdc(const char* module, const char* address);

/**
 * @brief Register the metadata currently stored in the middleware with all
 * added RDCs, making this component discoverable.
 */
void mw_register_rdcs();

/**
 * @brief Tell a remote component at a known address to register with all its
 * added RDCs.
 *
 * @param address
 *		Address of remote component.
 *
 */
void mw_tell_register_rdcs(const char* module, const char* address);

/**
 * @brief Unregister this component from all added RDCs, preventing
 * this component from being discovered.
 */
void mw_unregister_rdcs();

/**
 * @brief Tell a remote component at a known address to unregister with all its
 * added RDCs.
 *
 * @param address
 *		Address of remote component.
 */
void mw_tell_unregister_rdcs(const char* module, const char* address);

/**
 * @brief Load a com module in the core.
 *
 * @param path
 * 		Relative/absolute or path to the dynamic library implementing the com interface.
 */
int mw_load_com_module(const char* lib_path, const char* cfg_path);//, const char* cfg_path

/**
 * @brief Load an access module in the core.
 *
 * @param path
 * 		Relative/absolute or path to the dynamic library implementing the access interface.
 */
int mw_load_access_module(const char* lib_path, const char* cfg_path);

/**
 * @brief Get manifest about a remote mapping.
 *
 * @param module
 * 		Com  module of the mapping.
 * @param conn
 * 		connection id.
 */
char* mw_get_remote_metdata(const char* module, int conn);


#endif /* MIDDLEWARE_H_ */
