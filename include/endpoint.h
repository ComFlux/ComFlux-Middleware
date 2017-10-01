/**
 * @file endpoint.h
 * @author Raluca Diaconu
 * @brief API for creating and manipulating endpoints.
 */

#ifndef APIMW_ENDPOINT_H_
#define APIMW_ENDPOINT_H_

#include "endpoint_base.h"
#include "message.h"


/**
 * @brief Create a new source endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param type
 *		Type of endpoint (source, sink, request(+), resp(+), stream src, stream sink).
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_str
 *		String for a message or request schema JSON.
 *
 * @param resp_str
 *		String for a response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a message is received on this endpoint.
 *
 * @param id
 *		Endpoint identifier.
 *
 * @return
 *		Pointer to the newly created endpoint struct.
 */
ENDPOINT* endpoint_new(const char* name, const char* description, int type,
                       const char* msg_str, const char* resp_str,
                       void (*callback_function)(MESSAGE*),
                       const char* id);

/**
 * @brief Create a new source endpoint in the core where schemas are red from file.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param type
 *		Type of endpoint (source, sink, request(+), resp(+), stream src, stream sink).
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_path
 *		Path-to to the message or request schema JSON.
 *
 * @param resp_path
 *		Path-to to the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a message is received on this endpoint.
 *
 * @param id
 *		Endpoint identifier.
 *
 * @return
 *		Pointer to the newly created endpoint struct.
 */
ENDPOINT* endpoint_new_file(const char* name, const char* description, int type,
                       const char* msg_path, const char* resp_path,
                       void (*callback_function)(MESSAGE*),
                       const char* id);

/**
 * @brief Create a new source endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_str
 *		String for the message schema JSON.
 *
 * @return
 *		Pointer to the newly created endpoint struct.
 */
ENDPOINT* endpoint_new_src(const char* name, const char* description,
		const char* msg_str);

/**
 * @brief Create a new source endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_path
 *		Path-to to the message schema JSON.
 *
 * @return
 *		Pointer to the newly created endpoint struct.
 */
ENDPOINT* endpoint_new_src_file(const char* name, const char* description,
		const char* msg_path);

/**
 * @brief Create a new sink endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_str
 *		Path to the message schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a message is received on this endpoint.
 *
 * @return
 *		Pointer to the newly created endpoint struct.
 */
ENDPOINT* endpoint_new_snk(const char* name, const char* description,
		                   const char* msg_str,
                           void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new sink endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_path
 *		Path to the message schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a message is received on this endpoint.
 *
 * @return
 *		Pointer to the newly created endpoint struct.
 */
ENDPOINT* endpoint_new_snk_file(const char* name, const char* description,
		                   const char* msg_path,
                           void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new source-sink endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_str
 *		String of the message schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a message is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_ss(const char* name, const char* description,
		                  const char* msg_str,
                          void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new source-sink endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_path
 *		Path to the message schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a message is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_ss_file(const char* name, const char* description,
		                  const char* msg_path,
                          void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new request endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_str
 *		String of the request schema JSON.
 *
 * @param resp_str
 *		String of the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_req(const char* name, const char* description,
                           const char* msg_str, const char* resp_str,
                           void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new request endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_path
 *		Path to the request schema JSON.
 *
 * @param resp_path
 *		Path to the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_req_file(const char* name, const char* description,
                           const char* msg_path, const char* resp_path,
                           void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new response endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_str
 *		String of the message schema JSON.
 *
 * @param resp_str
 *		String of the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_resp(const char* name, const char* description,
                            const char* msg_str, const char* resp_str,
                            void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new response endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_path
 *		Path to the message schema JSON.
 *
 * @param resp_path
 *		Path to the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_resp_file(const char* name, const char* description,
                            const char* msg_path, const char* resp_path,
                            void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new request+ endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_str
 *		String of the message schema JSON.
 *
 * @param resp_str
 *		String of the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_req_p(const char* name, const char* description,
                             const char* msg_str, const char* resp_str,
                             void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new request+ endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_path
 *		Path to the message or request schema JSON.
 *
 * @param resp_path
 *		Path to the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_req_p_file(const char* name, const char* description,
                             const char* msg_path, const char* resp_path,
                             void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new response+ endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_str
 *		String of the request schema JSON.
 *
 * @param resp_str
 *		String of the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_resp_p(const char* name, const char* description,
                              const char* msg_str, const char* resp_str,
                              void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new response+ endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_path
 *		Path to the message schema JSON.
 *
 * @param resp_path
 *		Path to the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */

ENDPOINT* endpoint_new_resp_p_file(const char* name, const char* description,
                             const char* msg_path, const char* resp_path,
                             void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new request-response endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_str
 *		String of the request schema JSON.
 *
 * @param resp_str
 *		String of the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_rr(const char* name, const char* description,
                          const char* msg_str, const char* resp_str,
                          void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new request-response endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_path
 *		Path to the request schema JSON.
 *
 * @param resp_path
 *		Path to the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_rr_file(const char* name, const char* description,
                          const char* msg_path, const char* resp_path,
                          void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new request-response+ endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_str
 *		String of the request schema JSON.
 *
 * @param resp_str
 *		String of the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_rr_p(const char* name, const char* description,
                            const char* msg_str, const char* resp_str,
                            void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new request-response+ endpoint in the core.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param msg_path
 *		Path to the request schema JSON.
 *
 * @param resp_path
 *		Path to the response schema JSON.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_rr_p_file(const char* name, const char* description,
                            const char* msg_path, const char* resp_path,
                            void (*callback_function)(MESSAGE*));

/**
 * @brief Create a new stream src endpoint in the core.
 * Note that it does not need a filename of the schema JSON.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_stream_src(const char* name, const char *description);

/**
 * @brief Create a new stream sink endpoint in the core.
 * Note that it does not need a filename of the schema JSON.
 *
 * @param name
 *		A simple name to describe the endpoint. This will be available to mapping queries.
 *
 * @param description
 *		A more detailed description of the endpoint's purpose.
 *
 * @param callback_function
 *		Function to be invoked by the mw when a response is received on this endpoint.
 *
 * @return Pointer to the newly created endpoint struct.
 *
 */
ENDPOINT* endpoint_new_stream_snk(const char* name, const char *description,
                           void (*callback_function)(MESSAGE*));
/**
 * @brief Unregister an endpoint from the core, remove it from the local endpoint list, and free it.
 * 
 * @param endpoint
 *		Endpoint to remove.
 *
 */
void endpoint_remove(ENDPOINT* endpoint);

/**
 * @brief Send a text message over an endpoint, to all mapped remotes.
 *
 * @param endpoint
 *		Endpoint over which to send message.
 *
 * @param msg
 *		Message to send.
 *
 */
void endpoint_send_message(ENDPOINT* endpoint, const char* msg);

/**
 * @brief Send a text message over an endpoint, to all mapped remotes.
 *
 * @param endpoint
 *		Endpoint over which to send message.
 *
 * @param msg_json
 *		JSON message to send.
 *
 */
void endpoint_send_message_json(ENDPOINT* endpoint, JSON* msg_json);

/**
 * @brief Send a text request over an endpoint, to all mapped remotes.
 *
 * @param endpoint
 *		Endpoint over which to send request.
 *
 * @param msg
 *		Request to send.
 *
 * @return The request message id, immediately, so response demultiplexing may
 * be handled by the developer if they choose not use a handler.
 */
char* endpoint_send_request(ENDPOINT* endpoint, const char* msg);

/**
 * @brief Send a text request over an endpoint, to all mapped remotes.
 *
 * @param endpoint
 *		Endpoint over which to send request.
 *
 * @param msg
 *		JSON request to send.
 *
 * @return The request message id, immediately, so response demultiplexing may
 * be handled by the developer if they choose not use a handler.
 */
char* endpoint_send_request_json(ENDPOINT* endpoint, JSON* msg);

/**
 * @brief Send a text request over an endpoint, to all mapped remotes, then block
 * until a response is received (or a timeout is triggered).  **Note:** Cannot
 * be called on non-queueing (i.e. handler) endpoints.
 *
 * @param endpoint
 *		Endpoint over which to send request.
 *
 * @param msg
 *		Request to send.
 *
 * @return The received response. If no response is received before a timeout, NULL is returned.
 *
 */
MESSAGE* endpoint_send_request_blocking(ENDPOINT* endpoint, const char* msg);

/**
 * @brief Send a text request over an endpoint, to all mapped remotes, then block
 * until a response is received (or a timeout is triggered).  **Note:** Cannot
 * be called on non-queueing (i.e. handler) endpoints.
 *
 * @param endpoint
 *		Endpoint over which to send request.
 *
 * @param msg
 *		JSON request to send.
 *
 * @return The received response. If no response is received before a timeout, NULL is returned.
 *
 */
MESSAGE* endpoint_send_request_json_blocking(ENDPOINT* endpoint, JSON* msg);

/**
 * @brief Send a text response over an endpoint.
 *
 * @param endpoint
 *		Endpoint over which to send response.
 *
 * @param req_id
 *		The id of the request we are responding to.
 *
 * @param msg
 *		Response to send.
 *
 */
void endpoint_send_response(ENDPOINT* endpoint, const char* req_id, const char* msg);

/**
 * @brief Send a text response over an endpoint.
 *
 * @param endpoint
 *		Endpoint over which to send response.
 *
 * @param req_id
 *		The id of the request we are responding to.
 *
 * @param msg
 *		JSON response to send.
 *
 */
void endpoint_send_response_json(ENDPOINT* endpoint, const char* req_id, JSON* msg);

/**
 * @brief Send the final text response over a response+ endpoint.
 *
 * @param endpoint
 *		Endpoint over which to send response.
 *
 * @param req_id
 *		The id of the request we are responding to.
 *
 * @param msg
 *		Response to send.
 *
 */
void endpoint_send_last_response(ENDPOINT* endpoint, const char* req_id, const char* msg);

/**
 * @brief Send the final text response over a response+ endpoint.
 *
 * @param endpoint
 *		Endpoint over which to send response.
 *
 * @param req_id
 *		The id of the request we are responding to.
 *
 * @param msg
 *		JSON response to send.
 *
 */
void endpoint_send_last_response_json(ENDPOINT* endpoint, const char* req_id, JSON* msg);


/**
 * @brief Start streaming on the endpoint by opening a fifo with the incomming socket.
 *
 * @param endpoint
 *		Streaming endpoint.
 *
 */
void endpoint_start_stream(ENDPOINT* endpoint);

/**
 * @brief Stop streaming on the endpoint.
 *
 * @param endpoint
 *		Streaming endpoint.
 *
 */
void endpoint_stop_stream(ENDPOINT* endpoint);

/**
 * @brief Send a chink of the stream on the endpoint.
 *
 * @param endpoint
 *		Streaming endpoint.
 *
 */
void endpoint_send_stream(ENDPOINT* endpoint, char* msg);


/**
 * @brief Send a message over an endpoint. This is an internal function, and its
 * use should be avoided where possible.
 *
 * @param endpoint
 *		Endpoint over which to send message.
 *
 * @param msg
 *		Message to send.
 *
 */
void endpoint_send(ENDPOINT* endpoint, MESSAGE* msg);

/**
 * @brief Returns the number of messages queued for a particular endpoint in the core.
 *
 * @param endpoint
 *		Endpoint from which to request number of messages queued.
 *
 * @return The number of messages queued for a particular endpoint in the core.
 *
 */
int endpoint_more_messages(ENDPOINT* endpoint);

/**
 * @brief Return the number of requests queued for a particular endpoint in the core.
 *
 * @param endpoint
 *		Endpoint from whice to get number of requests queued.
 * 
 * @return Number of requests queued for given endpoint.
 */
int endpoint_more_requests(ENDPOINT* endpoint);

/**
 * @brief Return the number of responses queued for a particular endpoint in the core.
 *
 * @param endpoint
 *		Endpoint from which to request number of responses queued.
 *
 * @param req_id
 *		The id of the request from which we want the number of responses queued.
 *		
 */
int endpoint_more_responses(ENDPOINT* endpoint, const char* req_id);

/**
 * @brief Retrieve a single queued message from the core.
 *
 * @param endpoint
 *		Endpoint from which to get a queued message.
 *
 * @return Pointer to (previously queued) message.
 *
 */
MESSAGE* endpoint_fetch_message(ENDPOINT* endpoint);

/**
 * @brief Retrieve a single queued request from the core.
 *
 * @param endpoint
 *		Endpoint from which to get a queued request.
 *
 * @return Pointer to (previously queued) request.
 *
 */
MESSAGE* endpoint_fetch_request(ENDPOINT* endpoint);

/**
 * @brief Retrieve a single queued response from the core.
 *
 * @param endpoint
 *		Endpoint from which to get a queued response.
 *
 * @param req_id
 *		Id of request for which to get response.
 *
 * @return Pointer to (previously queued) response.
 *
 */
MESSAGE* endpoint_fetch_response(ENDPOINT* endpoint, const char* req_id);

/**
 * @brief Add a message filter to an endpoint.
 *
 * @param endpoint
 *		Endpoint to which to add a new message filter.
 *
 * @param filter
 *
 */
void endpoint_add_filter(ENDPOINT* endpoint, const char* filter);

/**
 * @brief Set the message filters for an endpoint.
 *
 * @param endpoint
 *		Endpoint for which to set filters.
 *
 * @param filter_json
 *		**Note:** Set filter_json to NULL to remove all filters.
 */
void endpoint_set_filters(ENDPOINT* endpoint, const char* filter_json);

/**
 * @brief Set mapping access on an endpoint.
 *
 * @param endpoint
 *		Endpoint object for which to set access control.
 *
 * @param subject
 *		The name of a remote endpoint.
 */
void endpoint_set_accesss(ENDPOINT* endpoint, const char* subject);

/**
 * @brief Remove mapping access on an endpoint.
 *
 * @param endpoint
 *		Endpoint object for which to set access control.
 *
 * @param subject
 *		The name of a remote endpoint.
 */
void endpoint_reset_accesss(ENDPOINT* endpoint, const char* subject);

/**
 * @brief Return a JSON description of the endpoint.
 *
 * @param endpoint
 *		Endpoint for which to get JSON description.
 *
 * @return JSON description of given endpoint.
 *
 */
JSON* ep_to_json(ENDPOINT* endpoint);

/**
 * @brief Return a string based on the endpoint's JSON description.
 *
 * @param endpoint
 *		Endpoint for which to get stringified JSON description.
 *
 * @return Stringified JSON description of given endpoint.
 *
 */
char* ep_to_str(ENDPOINT* endpoint);


Array* ep_get_all_connections(ENDPOINT* endpoint);

/**
 * @brief Map a local endpoint to a remote endpoint associated with a component
 * at a known address. The remote endpoint is selected by the remote component,
 * according to the given query.
 *
 * **Note:** an empty query will match all endpoints (of the correct type).
 *
 * @param endpoint
 *		Endpoint to which to map the result of the given query to.
 *
 * @param address
 *		Address of the remote components whose endpoints will be matched against
 *		the given query.
 *
 * @param ep_query
 *		A string containing a filter on the endpoint metadata
 *
 *@param cpt_query
 *		A string containing a filter on the component manifest
 *
 * @return Status code (0 = OK), depending on whether mapping was successful.
 */
int endpoint_map_to(ENDPOINT* endpoint, const char* address,  const char* ep_query, const char* cpt_query);


/**
 * @brief Map a local endpoint to a remote endpoint associated with a component
 * at a known address. The remote endpoint is selected by the remote component,
 * according to the given query.
 *
 * **Note:** an empty query will match all endpoints (of the correct type).
 *
 * @param endpoint
 *		Endpoint to which to map the result of the given query to.
 *
 * @param address
 *		Address of the remote components whose endpoints will be matched against
 *		the given query.
 *
 * @param ep_query
 *		A string containing a filter on the endpoint metadata
 *
 *@param cpt_query
 *		A string containing a filter on the component manifest
 *
 * @return Status code (0 = OK), depending on whether mapping was successful.
 */
int endpoint_map_module(ENDPOINT* endpoint, const char* module, const char* address, const char* ep_query, const char* cpt_query);

/**
 * @brief Map a local endpoint to many remote endpoints associated with
 * components on unknown addresses.
 *
 * The components are listed by each added RDC in turn, according to the query,
 * and the remote endpoints are selected by the remote components.
 *
 * @param endpoint
 *		Endpoint to which the resulting endpoints of the query on each RDC will
 *		be mapped.
 *
 * @param ep_query
 *		A string containing a filter on the endpoint metadata
 *
 *@param cpt_query
 *		A string containing a filter on the component manifest
 *
 * @param max_maps
 *		Maximum number of endpoints which should map to the given one. **Note:**
 *		Set max_maps to 0 to map to all eligible endpoints.
 */
void endpoint_map_lookup(ENDPOINT* endpoint, const char* ep_query, const char* cpt_query, int max_maps);

/**
 * @brief Remove the mapping(s) from a local endpoint to all remote endpoints on
 * a particular remote component at a known address.
 *
 * @param endpoint
 *		Endpoint from which to remove mappings.
 *
 * @param address
 *		Address at which endpoints mapped to the given local endpoint are unmapped.
 *
 * @return Status code (0 = OK), depending on whether unmapping was successful.
 */
int endpoint_unmap_from(ENDPOINT* endpoint, const char* address);

/**
 * @brief Remove all mappings from a local endpoint.
 *
 * @param endpoint
 *		Endpoint from which to remove all mappings.
 *
 * @return Status code (0 = OK), depending on whether unmapping was successful.
 */
int endpoint_unmap_all(ENDPOINT* endpoint);


#endif /* APIMW_ENDPOINT_H_ */
