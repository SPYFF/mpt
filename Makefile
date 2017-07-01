SHELL=/bin/bash
SUBDIRS=src
CLEANDIRS=$(SUBDIRS:%=clean-%)

TARGETDIR ?= .
VER = $(date +"%F")

.PHONY: subdirs $(SUBDIRS)
.PHONY: clean

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@echo "  Chosen architecture: default"
	@$(MAKE) -C $@

clean: $(CLEANDIRS)
	find . -name "*~" -delete

$(CLEANDIRS):
	@$(MAKE) -C $(@:clean-%=%) clean

pack: subdirs pack-clean pack-src pack-lib

pack-src: subdirs
	@echo "  BUILD source package"
	@tar vczf $(TARGETDIR)/mpt-gre-src-$(VER).tar.gz \
	    -C .. \
	    --exclude-vcs \
	    --exclude "*.o" \
	    --exclude "*.a" \
	    --exclude "usermanual-*" \
	    --exclude "nat_conf_doc-*" \
	    --exclude "*.docx" \
	    mpt/bin/ \
	    mpt/conf/ \
	    mpt/doc/ \
	    mpt/src/ \
	    mpt/nat_conf/ \
	    mpt/Makefile \
	    mpt/Makefile.m32 \
	    mpt/Makefile.m64

pack-lib: subdirs
	@echo "  BUILD library package"
	@test ! -e $(TARGETDIR)/mpt -o ! -e $(TARGETDIR)/mptsrv || \
	    tar czf $(TARGETDIR)/mpt-gre-lib-$(VER).tar.gz \
		-C .. \
		--exclude-vcs \
		--exclude "*.o" \
		--exclude "usermanual-*" \
		--exclude "nat_conf_doc-*" \
		--exclude "*.docx" \
		mpt/bin/ \
		mpt/conf/ \
		mpt/doc/usermanual/ \
		mpt/src/mpt/ \
		mpt/src/mptsrv/ \
		mpt/src/include/ \
		mpt/src/Makefile \
		mpt/src/Makefile.m32 \
		mpt/src/Makefile.m64 \
		mpt/Makefile \
		mpt/mpt \
		mpt/mptsrv

pack-clean:
	@rm $(TARGETDIR)/mpt-*.tar.gz || true
