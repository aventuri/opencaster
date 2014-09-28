SUBDIRS = libs tools 

subdirs:        $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@
.PHONY:		subdirs $(SUBDIRS)

all: subdirs

install: all
	@for T in $(SUBDIRS); do make -C $$T $@; done

clean:
	@for T in $(SUBDIRS); do make -C $$T $@; done
