AUTOMAKE_OPTIONS = subdir-objects
bin_PROGRAMS = imonitor
imonitor_SOURCES = imonitor.c sock_util.c utils.c http-parser/http_parser.c
AM_CFLAGS = -g -Wall -Wextra
AM_CPPFLAGS = -I./http-parser
LIBS += -laio