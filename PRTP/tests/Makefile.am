## Process this file with automake to produce Makefile.in

TESTS = check_sensor_parser
check_PROGRAMS = check_sensor_parser
check_sensor_parser_SOURCES = check_sensor_parser.c $(top_builddir)/src/sensor_parser.c
check_sensor_parser_CFLAGS = @CHECK_CFLAGS@
check_sensor_parser_LDADD = @CHECK_LIBS@ @top_builddir@/application/libpubsub.la
