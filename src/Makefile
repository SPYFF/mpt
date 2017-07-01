SUBDIRS=$(wildcard */)
CLEANDIRS=$(SUBDIRS:%=clean-%)

.PHONY: subdirs $(SUBDIRS)
.PHONY: clean

subdirs: $(SUBDIRS)

$(SUBDIRS):
	@$(MAKE) -C $@

clean: $(CLEANDIRS)

$(CLEANDIRS):
	@$(MAKE) -C $(@:clean-%=%) clean
