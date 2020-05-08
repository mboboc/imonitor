package = jupiter
version = 1.0
tarname = $(package)
distdir = $(tarname)-$(version)

# sudo make prefix=/usr install to change the var
prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
# export prefix
# export exec_prefix
# export bindir

# all clean check install uninstall jupiter:
# 	cd src && $(MAKE) $@

all jupiter: src/jupiter

#creates archive
dist: $(distdir).tar.gz

$(distdir).tar.gz: $(distdir)
	tar chof - $(distdir) | gzip -9 -c > $@
	rm -rf $(distdir)

# since we made FORCE as a dependency, any prev files and dirs will be deleted before make begins
# FORCE it's a PHONY target = it will always execute
$(distdir): FORCE
	mkdir -p $(distdir)/src
	cp Makefile $(distdir)
	cp src/Makefile $(distdir)/src
	cp src/main.c $(distdir)/src

FORCE:
	-rm -rf $(distdir).tar.gz
	-rm -rf $(distdir)

# depends on the tarball itself, so the rule that builds the tarball is executed first
distcheck: $(distdir).tar.gz
	gzip -cd $(distdir).tar.gz | tar xvf -
	cd $(distdir) && $(MAKE) all
	cd $(distdir) && $(MAKE) check
	cd $(distdir) && $(MAKE) DESTDIR=$${PWD}/_inst install
	cd $(distdir) && $(MAKE) DESTDIR=$${PWD}/_inst uninstall
	
	# @ - suppresses command echo of make
	@remaining="`find $(distdir)/_inst -type f | wc -l`"; \
	if test "$${remaining}" -ne 0; then \
		echo "*** $${remaining} file(s) remaining in the stage directory!"; \
		exit 1; \
	fi
	cd $(distdir) && $(MAKE) clean
	rm -rf $(distdir)
	@echo "*** Package $(distdir).tar.gz is ready for distirbutuion."

include src/Makefile

.PHONY: FORCE all clean dist distcheck install uninstall
