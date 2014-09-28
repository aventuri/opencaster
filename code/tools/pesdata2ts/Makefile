C      = gcc
CFLAGS  += -g -MD -Wall -I. -I../../include $(CPPFLAGS)
LDFLAGS  += -lc

OBJS = pesdata2ts.o
TARGET = pesdata2ts
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

