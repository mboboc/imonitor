IDIR=../http-parser
CFLAGS=-g -Wall -Wextra
CPPFLAGS= -I./http-parser
LDLIBS = -laio

build: imonitor

imonitor: imonitor.o sock_util.c http-parser/http_parser.o utils.o

imonitor.o: imonitor.c

sock_util.o: sock_util.c

http-parser/http_parser.o: http-parser/http_parser.c http-parser/http_parser.h

utils.o: utils.c

clean:
	rm -rf *.o imonitor *log static dynamic
	rm -f http-parser/*.o http-parser/test http-parser/test_fast http-parser/test_g http-parser/http_parser.tar http-parser/tags
.PHONY:
	clean, build
