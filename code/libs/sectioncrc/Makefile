C      = gcc
CFLAGS  = -g -MD -Wall -I. -I../../include 

OBJ = sectioncrc.o 

all: $(OBJ)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LFLAGS)

install: all

clean:
	rm -f $(OBJS) core* *~ *.d *.o

-include $(wildcard *.d) dummy

