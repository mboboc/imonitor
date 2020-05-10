build: imonitor

imonitor: imonitor.o hashtable.o

imonitor.o: imonitor.c

hashtable.o: hashtable.c

clean:
	rm *.o imonitor
run:
	./imonitor

memcheck:
	valgrind ./imonitor

.PHONY:
	memcheck, build, clean, run
