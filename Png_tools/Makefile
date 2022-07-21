# Makefile 


CC = gcc       # compiler
CFLAGS = -Wall -g -std=c99 # compilation flags
LD = gcc       # linker
LDFLAGS = -g   # debugging symbols in build
LDLIBS = -lz   # link with libz

# For students 
LIB_UTIL = zutil.o crc.o
SRCS   = pnginfo.c findpng.c catpng.c crc.c zutil.c
OBJS   = pnginfo.o findpng.o catpng.o $(LIB_UTIL) 

TARGETS= pnginfo findpng catpng

all: pnginfo findpng catpng

pnginfo: pnginfo.o crc.o 
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

findpng: findpng.o
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 
	
catpng: catpng.o crc.o zutil.o
	$(LD) -o $@ $^ $(LDLIBS) $(LDFLAGS) 

%.o: %.c 
	$(CC) $(CFLAGS) -c $< 



.PHONY: clean
clean:
	rm -f *.d *.o $(TARGETS) 
