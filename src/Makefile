CFLAGS = -g

build: imonitor

imonitor: imonitor.o hashtable.o sock_util.o

imonitor.o: imonitor.c

sock_util.o: sock_util.c

clean:
	rm *.o imonitor auth.log
run:
	./imonitor

memcheck:
	valgrind ./imonitor

.PHONY:
	memcheck, build, clean, run 
