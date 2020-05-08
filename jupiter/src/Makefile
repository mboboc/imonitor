src/jupiter: src/main.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ src/main.c

#depends on all, can't teste the product if is not up-to-date
check: all
	./src/jupiter | grep "Hello from .*jupiter!"
	@echo "*** ALL TESTS PASSED ***"

install:
	# cp jupiter /usr/bin
	# chown root:root /usr/bin/jupiter
	#chmod +x /usr/bin/jupiter

	# export from top-level Makefile  exports the variable to the shell
	# that make spawns when it executes itself in the src dir
	# DESTDIR extends to "" if is not user defined - which is ok
	install -d $(DESTDIR)$(bindir)
	install -m 0755 src/jupiter $(DESTDIR)$(bindir)

uninstall:
	-rm -f $(DESTDIR)$(bindir)/jupiter
	-rmdir $(DESTDIR)$(bindir) > /dev/null 2>&1

clean:
	rm -f src/jupiter
