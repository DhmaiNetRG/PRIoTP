## Process this file with automake to produce Makefile.in

TESTS = check_sensor_parser check_bson_parser check_clients check_transport check_fragmentation
check_PROGRAMS = check_sensor_parser check_bson_parser check_clients check_transport check_fragmentation
check_sensor_parser_SOURCES = check_sensor_parser.c $(top_builddir)/src/sensor_parser.c
check_sensor_parser_CFLAGS = @CHECK_CFLAGS@
check_sensor_parser_LDADD = @CHECK_LIBS@ @top_builddir@/application/libpubsub.la

check_bson_parser_SOURCES = check_bson_parser.c
check_bson_parser_CFLAGS = @CHECK_CFLAGS@
check_bson_parser_LDADD = @CHECK_LIBS@ @top_builddir@/application/libpubsub.la

check_clients_SOURCES = check_clients.c $(top_builddir)/src/clients_config.h $(top_builddir)/src/clients_config.c i\
			$(top_builddir)/src/clients.h $(top_builddir)/src/clients.c $(top_builddir)/src/subscriptions.h $(top_builddir)/src/subscriptions.c
check_clients_CFLAGS = @CHECK_CFLAGS@
check_clients_LDFLAGS = @CHECK_CFLAGS@
check_clients_LDADD = @CHECK_LIBS@ @top_builddir@/application/libpubsub.la

check_transport_SOURCES = check_transport.c $(top_builddir)/src/transport.h $(top_builddir)/src/transport.c
check_transport_CFLAGS = @CHECK_CFLAGS@
check_transport_LDADD = @CHECK_LIBS@ @top_builddir@/application/libpubsub.la

check_fragmentation_SOURCES = check_fragmentation.c $(top_builddir)/src/fragment_buffer.h $(top_builddir)/src/fragment_buffer.c
check_fragmentation_CFLAGS = @CHECK_CFLAGS@
check_fragmentation_LDADD = @CHECK_LIBS@ @top_builddir@/application/libpubsub.la
