cmake_minimum_required(VERSION 3.5)
project(middleware C)


# Set up the compiler.
if (NOT ${CMAKE_C_FLAGS} MATCHES "-std")
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -std=gnu99)
endif ()

if (APPLE)
    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} -DDARWIN)
endif ()

if (NOT OUTPUT_SET)
    set(ARCHIVE_OUTPUT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/build/lib)
    set(RUNTIME_OUTPUT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/build/bin)

    set(OUTPUT_SET TRUE)
endif ()

#enable ssl functions(ssl com module, ssl acl module)
set(SSLENABLED OFF CACHE BOOL "enable ssl commmodule")
set(KRBENABLED OFF CACHE BOOL "enable kerberos auth module")

# Glob the source files.
file(GLOB_RECURSE JSONSCHEMAC_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/jsonschema-c/*.c)
        
        
file(GLOB_RECURSE UTILS_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/*.c)

file(GLOB_RECURSE API_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/api/*.c)

file(GLOB_RECURSE COMMON_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/*.c)

file(GLOB_RECURSE CORE_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.c)

file(GLOB_RECURSE WRAPPERS_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/module_wrappers/*.c)
        

file(GLOB_RECURSE COM_MODULE_TCP_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/tcp/*.c)
   
if(MQTTENABLED)     
file(GLOB_RECURSE COM_MODULE_MQTT_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/mqtt/*.c)
       
file(GLOB_RECURSE COM_MODULE_MQTT_BRIDGE_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/mqtt_bridge/*.c)
endif()
        
file(GLOB_RECURSE COM_MODULE_SOCKPAIR_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/sockpair/*.c)
        
file(GLOB_RECURSE MODULE_COMM_UDP_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/udp/conn_udp.c)

file(GLOB_RECURSE MODULE_COMM_SSL_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/ssl/conn_ssl.c)

file(GLOB_RECURSE MODULE_COMM_REST_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/rest/*.c)

#source for simple access module
file(GLOB_RECURSE MODULE_ACL_SIMPLE
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/access_modules/plain/simple_acc.c)

file(GLOB_RECURSE MODULE_ACL_SSL
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/access_modules/ssl/ssl_acc.c)

file(GLOB_RECURSE MODULE_ACL_KRB
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/access_modules/krb5/krb5_acc.c)
######
# Glob the source files.
file(GLOB_RECURSE SWISS_KNIFE_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        src/swiss_knife/*.c)

file(GLOB_RECURSE RDC_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        src/rdc/*.c)
######

# Build the utility library.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_OUTPUT_ROOT})

add_library(middleware_utils STATIC ${UTILS_SRC} ${JSONSCHEMAC_SRC})
target_include_directories(middleware_utils PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/uthash/include
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/jsonschema-c)
target_link_libraries(middleware_utils pthread json-c)
set_property(TARGET middleware_utils PROPERTY POSITION_INDEPENDENT_CODE ON)

# Build the API library.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ARCHIVE_OUTPUT_ROOT})
add_library(middleware_api STATIC ${API_SRC} ${COMMON_SRC} ${WRAPPERS_SRC} ${UTILS_SRC})
#target_include_directories(middleware_api PRIVATE
#        ${CMAKE_CURRENT_SOURCE_DIR}/src/apilib)
target_include_directories(middleware_api PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common  # TODO: Find some way of making this private.
        ${CMAKE_CURRENT_SOURCE_DIR}/src/module_wrappers  # TODO: Find some way of making this private.
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(middleware_api middleware_utils dl json-c)

# Build the core executable.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_ROOT}/core)
add_executable(middleware_core ${CORE_SRC} ${COMMON_SRC} ${WRAPPERS_SRC})
set_target_properties(middleware_core PROPERTIES
        OUTPUT_NAME core)
target_include_directories(middleware_core PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}/src/module_wrappers)
target_link_libraries(middleware_core middleware_utils dl pthread)
add_custom_command(TARGET middleware_core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/json/default_eps
        ${RUNTIME_OUTPUT_ROOT}/core/default_eps)
add_custom_command(TARGET middleware_core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/json/preloaded_schemata
        ${RUNTIME_OUTPUT_ROOT}/core/preloaded_schemata)

install(TARGETS middleware_core EXPORT core DESTINATION bin)

# Build the Modules
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ARCHIVE_OUTPUT_ROOT})
add_library(commoduletcp MODULE ${COM_MODULE_TCP_SRC} ${UTILS_SRC})
set_property(TARGET commoduletcp PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(commoduletcp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils 
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/tcp)
target_link_libraries(commoduletcp middleware_utils pthread)
#copy config files
add_custom_command(TARGET commoduletcp POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/tcp/tcp_*.json
        ${ARCHIVE_OUTPUT_ROOT}/)
install(TARGETS commoduletcp DESTINATION bin)


if(MQTTENABLED)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ARCHIVE_OUTPUT_ROOT})
add_library(commodulemqtt MODULE ${COM_MODULE_MQTT_SRC} ${UTILS_SRC})
set_property(TARGET commodulemqtt PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(commodulemqtt PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils 
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/mqtt)
target_link_libraries(commodulemqtt middleware_utils mosquitto pthread)
#copy config files
add_custom_command(TARGET commodulemqtt POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/mqtt/*.json
        ${ARCHIVE_OUTPUT_ROOT}/)
install(TARGETS commodulemqtt DESTINATION bin)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ARCHIVE_OUTPUT_ROOT})
add_library(commodulemqttbridge MODULE ${COM_MODULE_MQTT_BRIDGE_SRC} ${UTILS_SRC})
set_property(TARGET commodulemqttbridge PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(commodulemqttbridge PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils 
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/mqtt_bridge)
target_link_libraries(commodulemqttbridge middleware_utils mosquitto pthread)
#copy config files
add_custom_command(TARGET commodulemqttbridge POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/mqtt_bridge/*.json
        ${ARCHIVE_OUTPUT_ROOT}/)
install(TARGETS commodulemqttbridge DESTINATION bin)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ARCHIVE_OUTPUT_ROOT})
add_library(commodulesockpair MODULE ${COM_MODULE_SOCKPAIR_SRC} ${UTILS_SRC})
set_property(TARGET commodulesockpair PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(commodulesockpair PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils 
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/sockpair)
target_link_libraries(commodulesockpair middleware_utils pthread)
#copy config files
add_custom_command(TARGET commodulesockpair POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/sockpair/sockpair*.json
        ${ARCHIVE_OUTPUT_ROOT}/)
install(TARGETS commodulesockpair DESTINATION bin)


#build the udp module
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ARCHIVE_OUTPUT_ROOT})
add_library(commodulecommudp MODULE ${MODULE_COMM_UDP_SRC} ${UTILS_SRC})

set_property(TARGET commodulecommudp PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(commodulecommudp PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils  
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/udp)
target_link_libraries(commodulecommudp middleware_utils pthread)
#copy config files
#add_custom_command(TARGET commodulecommudp POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/udp/udp_*.json
#        ${ARCHIVE_OUTPUT_ROOT}/)
install(TARGETS commodulecommudp DESTINATION bin)

#build ssl module

if(SSLENABLED)
 add_library(commodulecommssl MODULE ${MODULE_COMM_SSL_SRC} ${UTILS_SRC})
 set_property(TARGET commodulecommssl PROPERTY POSITION_INDEPENDENT_CODE ON)
 target_include_directories(commodulecommssl PUBLIC
         ${CMAKE_CURRENT_SOURCE_DIR}/src/utils 
         ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/ 
         ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/ssl)
 target_link_libraries(commodulecommssl middleware_utils pthread ssl crypto)
add_custom_command(TARGET commodulecommssl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/ssl/ssl_*.json
       ${ARCHIVE_OUTPUT_ROOT}/)
endif ()

#build rest module
 add_library(commodulecommrest MODULE ${MODULE_COMM_REST_SRC} ${WRAPPERS_SRC} ${UTILS_SRC})
 set_property(TARGET commodulecommrest PROPERTY POSITION_INDEPENDENT_CODE ON)
 target_include_directories(commodulecommrest PUBLIC
         ${CMAKE_CURRENT_SOURCE_DIR}/src/utils 
         ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules 
         ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/rest
         ${CMAKE_CURRENT_SOURCE_DIR}/src/module_wrappers/)
 target_link_libraries(commodulecommrest middleware_utils pthread dl)
 add_custom_command(TARGET commodulecommrest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_CURRENT_SOURCE_DIR}/modules/com_modules/rest/rest_*.json
       ${ARCHIVE_OUTPUT_ROOT}/)



#build simple acl module
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${ARCHIVE_OUTPUT_ROOT})
add_library(moduleaclplain MODULE ${MODULE_ACL_SIMPLE} ${UTILS_SRC})

set_property(TARGET moduleaclplain PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(moduleaclplain PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/utils 
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/access_modules
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/access_modules/plain)
target_link_libraries(moduleaclplain middleware_utils)
#copy config files
add_custom_command(TARGET moduleaclplain POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/modules/access_modules/plain/*.json
        ${ARCHIVE_OUTPUT_ROOT}/)
install(TARGETS moduleaclplain DESTINATION bin)

#ssl acl module
if(SSLENABLED)
 add_library(moduleaclssl MODULE ${MODULE_ACL_SSL} ${UTILS_SRC})
 set_property(TARGET moduleaclssl PROPERTY POSITION_INDEPENDENT_CODE ON)
 target_include_directories(moduleaclssl PUBLIC
         ${CMAKE_CURRENT_SOURCE_DIR}/src/utils  
         ${CMAKE_CURRENT_SOURCE_DIR}/modules/access_modules/ssl)
 target_link_libraries(moduleaclssl middleware_utils pthread ssl crypto)
add_custom_command(TARGET moduleaclssl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_CURRENT_SOURCE_DIR}/modules/access_modules/ssl/aclssl.cfg.json
       ${ARCHIVE_OUTPUT_ROOT}/)
endif ()

if(KRBENABLED)
 add_library(moduleaclkrb MODULE ${MODULE_ACL_SSL} ${UTILS_SRC})
 set_property(TARGET moduleaclkrb PROPERTY POSITION_INDEPENDENT_CODE ON)
 target_include_directories(moduleaclkrb PUBLIC
         ${CMAKE_CURRENT_SOURCE_DIR}/src/utils  
         ${CMAKE_CURRENT_SOURCE_DIR}/modules/access_modules/krb5
 	 ${CMAKE_CURRENT_SOURCE_DIR}/lib/krb5/src/include)
target_link_libraries(moduleaclkrb middleware_utils krb5 /usr/local/lib/libcom_err.so.3)
#target_link_libraries(moduleaclkrb middleware_utils krb5 com_err)
add_custom_command(TARGET moduleaclkrb POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
       ${CMAKE_CURRENT_SOURCE_DIR}/modules/access_modules/krb5/aclkrb.cfg.json
       ${ARCHIVE_OUTPUT_ROOT}/)
endif()
#######
# Build the RDC and Swiss.

# Build the swiss_knife executable.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_ROOT}/swiss_knife)
add_executable(swiss_knife ${SWISS_KNIFE_SRC})
target_include_directories(swiss_knife PRIVATE
        src/swiss_knife)
target_link_libraries(swiss_knife middleware_api)

# Build the RDC.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_ROOT}/rdc)
add_executable(rdc ${RDC_SRC})
target_include_directories(rdc PRIVATE
        src/rdc)
target_link_libraries(rdc middleware_api)

add_custom_command(TARGET rdc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/json/rdc_schemata
        ${RUNTIME_OUTPUT_ROOT}/rdc/rdc_schemata)




######

# add the install targets
# install ut
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/uthash/src/
  DESTINATION include
  FILES_MATCHING PATTERN "*.h" )
  
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/middleware.h ${CMAKE_CURRENT_SOURCE_DIR}/include/endpoint.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/common/endpoint_base.h 
  ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/array.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/json.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/common/message.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/common/environment.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/utils.h
  DESTINATION include)

# install middleware libs
install (TARGETS middleware_api middleware_utils
         ARCHIVE DESTINATION lib
         LIBRARY DESTINATION lib
         RUNTIME DESTINATION bin)

#install swiss and rdc
install(TARGETS swiss_knife EXPORT swiss_knife DESTINATION bin)
install(TARGETS rdc EXPORT rdc DESTINATION bin)
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/json/
  DESTINATION etc/middleware/ )
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/rdc/config/
  DESTINATION etc/middleware/rdc )
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/swiss_knife/config/
  DESTINATION etc/middleware/swiss_knife )

#install com modules
install(TARGETS commoduletcp        DESTINATION etc/middleware/com_modules)
install(TARGETS commodulesockpair   DESTINATION etc/middleware/com_modules)
if(MQTTENABLED)
install(TARGETS commodulemqtt       DESTINATION etc/middleware/com_modules)
install(TARGETS commodulemqttbridge DESTINATION etc/middleware/com_modules)
endif()
install(TARGETS commodulecommudp    DESTINATION etc/middleware/com_modules)
install(TARGETS commodulecommrest   DESTINATION etc/middleware/com_modules)
if(SSLENABLED)
install(TARGETS commodulecommssl    DESTINATION etc/middleware/com_modules)
endif()

#install access modules
install(TARGETS moduleaclplain      DESTINATION etc/middleware/access_modules)
if(SSLENABLED)
install(TARGETS moduleaclssl        DESTINATION etc/middleware/access_modules)
endif()
if(KRBENABLED)
install(TARGETS moduleaclkrb        DESTINATION etc/middleware/access_modules)
endif()

# Build the examples.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_ROOT}/examples)

add_executable(simple_source ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple_source.c)
target_link_libraries(simple_source middleware_api)
#target_include_directories(simple_source PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
        
add_executable(simple_sink ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple_sink.c)
target_link_libraries(simple_sink middleware_api)
#target_include_directories(simple_sink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(lookup_source ${CMAKE_CURRENT_SOURCE_DIR}/examples/lookup_source.c)
target_link_libraries(lookup_source middleware_api)
#target_include_directories(lookup_source PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(lookup_sink ${CMAKE_CURRENT_SOURCE_DIR}/examples/lookup_sink.c)
target_link_libraries(lookup_sink middleware_api)
#target_include_directories(lookup_sink PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(simple_sink_nomap ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple_sink_nomap.c)
target_link_libraries(simple_sink_nomap middleware_api)
#target_include_directories(simple_sink_nomap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


# Copy the example endpoints schemata  
add_executable(simple_res ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple_res.c)
target_link_libraries(simple_res middleware_api)

add_executable(simple_req ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple_req.c)
target_link_libraries(simple_req middleware_api)
        

add_custom_command(TARGET simple_source POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/example_schemata
        ${RUNTIME_OUTPUT_ROOT}/examples/example_schemata)
        
# Copy the example general config file with all configs: slog level, log filepath, libs, etc
add_custom_command(TARGET simple_source POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/*cfg.json
        ${RUNTIME_OUTPUT_ROOT}/examples/)



# Build the tests.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_ROOT}/tests)

add_executable(test_tcp_sender ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_tcp_sender.c)
target_link_libraries(test_tcp_sender middleware_api)
#target_include_directories(test_tcp_sender PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_tcp_receiver ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_tcp_receiver.c)
target_link_libraries(test_tcp_receiver middleware_api)
#target_include_directories(test_tcp_receiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_mqtt_sender ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mqtt_sender.c)
target_link_libraries(test_mqtt_sender middleware_api)
#target_include_directories(test_mqtt_sender PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_mqtt_receiver ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_mqtt_receiver.c)
target_link_libraries(test_mqtt_receiver middleware_api)
#target_include_directories(test_mqtt_receiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(stream_tcp_sender ${CMAKE_CURRENT_SOURCE_DIR}/tests/stream_tcp_sender.c)
target_link_libraries(stream_tcp_sender middleware_api)
#target_include_directories(stream_tcp_sender PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(stream_tcp_receiver ${CMAKE_CURRENT_SOURCE_DIR}/tests/stream_tcp_receiver.c)
target_link_libraries(stream_tcp_receiver middleware_api)
#target_include_directories(stream_tcp_receiver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)


add_custom_command(TARGET test_mqtt_receiver POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/example_schemata
        ${RUNTIME_OUTPUT_ROOT}/tests/example_schemata)
        
# Copy the example general config file with all configs: slog level, log filepath, libs, etc
add_custom_command(TARGET test_mqtt_receiver POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/*cfg.json
        ${RUNTIME_OUTPUT_ROOT}/tests/)
