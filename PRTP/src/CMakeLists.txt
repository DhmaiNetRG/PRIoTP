set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(libpubsub_SOURCES 
    logger.c utils.c messages.c bson_parser.c bson_msg.c sensor_types.c validator.c 
    sensor_logger.c transport.c fragment_buffer.c congestion_control.c 
    ascon.c security.c
    security/blake2s/blake2s.c
    security/x25519/x25519.c
    security/ecies/ecies.c
    security/seh/seh.c
    security/identity/identity.c
    security/session/session.c
    security/handshake/handshake.c
    security/security_core/security_core.c
)
set(libpubsub_HEADERS logger.h utils.h messages.h bson_parser.h bson_msg.h sensor_types.h validator.h sensor_logger.h transport.h fragment_buffer.h void_data.h ascon.h security.h)
set(iotserver_SOURCES iotserver.c sensors.c clients.c sensor_parser.c subscriptions.c clients_config.c)
set(iotclient_SOURCES iotclient.c client_module.c active_flow.c)

set(CMAKE_BUILD_TYPE Debug)

add_library(
    pubsub
    ${libpubsub_SOURCES}
    ${libpubsub_HEADERS}
)

add_library(
    iotserver_testlib
    ${iotserver_SOURCES}
)

target_link_libraries(
    iotserver_testlib
    pubsub
)

add_library(
    iotclient_testlib
    ${iotclient_SOURCES}
)

target_link_libraries(
    iotclient_testlib
    pubsub
)

add_executable(iotserver ${iotserver_SOURCES})
target_link_libraries(
    iotserver
    pubsub
    ${MATH_LIB}
)

add_executable(iotclient ${iotclient_SOURCES})
target_link_libraries(
    iotclient
    pubsub
)
