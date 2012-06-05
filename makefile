CC = gcc
CFLAGS = -Wall -Werror -g

BINARIES = hostd
MAIN_SRC = hostd.c 
NONMAIN_SRC = pcb.c mab.c rsrc.c

.PHONY: all clean

all:
	$(CC) -c $(NONMAIN_SRC) $(CFLAGS)
	$(CC) -o $(BINARIES) $(MAIN_SRC) *.o $(CFLAGS)

clean:
	-rm -f $(BINARIES) *.o
