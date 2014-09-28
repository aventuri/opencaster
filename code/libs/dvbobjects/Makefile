
#DESTDIR ?= /usr/local/bin
#INSTHOME ?= /usr

all:
	./setup.py build

install: all
	./setup.py install --root=$(INSTHOME)
#install: all
#	./setup.py install 

clean:
	./setup.py clean
	rm -rf build/*

-include $(wildcard *.d) dummy

