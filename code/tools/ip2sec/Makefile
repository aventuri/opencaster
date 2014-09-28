CC      = gcc
CFLAGS  += -g -O2 -MD -Wall -I. -I../../include -D_FILE_OFFSET_BITS=64 $(CPPFLAGS)
LDFLAGS  += -lpcap

OBJS    = ip2sec.o ../../libs/sectioncrc/sectioncrc.o
TARGET  = ip2sec
DESTDIR = /usr/local/bin/

all: $(TARGET)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

install: all
	install -m 755 $(TARGET) $(DESTDIR) 

clean:
	rm -f $(TARGET) $(OBJS) core* *~ *.d

-include $(wildcard *.d) dummy
		
