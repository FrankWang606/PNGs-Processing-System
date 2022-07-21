

CC = gcc       # compiler
CFLAGS = -Wall -g -std=gnu99# compilation flg 
LD = gcc       # linker
LDFLAGS = -g -std=gnu99   # debugging symbols in build
LDLIBS  =-lpthread -lcurl -pthread -lz # link with libpthread

# For students 
LIB_UTIL = 
SRCS   = paster2.c crc.c zutil.c useful.c
OBJS3  = paster2.o crc.o zutil.o useful.o

TARGETS= paster2

all: ${TARGETS}
paster2: ${OBJS3}
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 

%.d: %.c
	gcc -MM -MF $@ $<

-include $(SRCS:.c=.d)

.PHONY: clean
clean:
	rm -f *.d *.o *~ $(TARGETS) 
