## Process this file with automake to produce Makefile.in

noinst_LTLIBRARIES = libpubsub.la
libpubsub_la_CFLAGS  = -Wall -pedantic
libpubsub_la_SOURCES = ../src/logger.c ../src/utils.c ../src/messages.c ../src/bson_parser.c ../src/bson_msg.c ../src/sensor_types.c \
					  ../src/validator.c ../src/sensor_logger.c ../src/transport.c ../src/fragment_buffer.c ../src/congestion_control.c \
					  ../src/ascon.c ../src/security.c \
					  ../src/security/blake2s/blake2s.c \
					  ../src/security/x25519/x25519.c \
					  ../src/security/ecies/ecies.c \
					  ../src/security/seh/seh.c \
					  ../src/security/identity/identity.c \
					  ../src/security/session/session.c \
					  ../src/security/handshake/handshake.c \
					  ../src/security/security_core/security_core.c

bin_PROGRAMS = PRTP_server PRTP_client q_agent_test

PRTP_server_SOURCES = PRTP_server.c ../src/sensors.h ../src/sensors.c ../src/clients.h ../src/clients.c \
	../src/sensor_parser.h ../src/sensor_parser.c ../src/subscriptions.h ../src/subscriptions.c ../src/clients_config.h ../src/clients_config.c \
	../src/q_agent.c
PRTP_server_CFLAGS  = -Wall -pedantic
PRTP_server_LDADD = libpubsub.la

PRTP_client_SOURCES = PRTP_client.c ../src/client_module.c ../src/active_flow.c
PRTP_client_LDADD = libpubsub.la
PRTP_client_CFLAGS  = -Wall -pedantic

q_agent_test_SOURCES = ../src/q_agent_test.c ../src/q_agent.c
q_agent_test_CFLAGS  = -Wall -pedantic
q_agent_test_LDADD = libpubsub.la