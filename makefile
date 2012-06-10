# SID: 310182212
# Name: Tianyu Pu

CC = gcc
CFLAGS = -Wall -Werror -g

BINARY = hostd
MAIN_SRC = hostd.c 
NONMAIN_SRC = pcb.c mab.c rsrc.c
REPORT = report
PROCESS = process
PROCESS_SRC = sigtrap.c

.PHONY: all clean

all:
	$(CC) -c $(NONMAIN_SRC) $(CFLAGS)
	$(CC) -o $(BINARY) $(MAIN_SRC) *.o $(CFLAGS)
	$(CC) -o $(PROCESS) $(PROCESS_SRC) $(CFLAGS)

$(REPORT):
	pdflatex $(REPORT)
	bibtex $(REPORT)
	pdflatex $(REPORT)
	pdflatex $(REPORT)

$(BINARY):
	$(CC) -c $(NONMAIN_SRC) $(CFLAGS)
	$(CC) -o $(BINARY) $(MAIN_SRC) *.o $(CFLAGS)
	$(CC) -o $(PROCESS) $(PROCESS_SRC) $(CFLAGS)

clean:
	-rm -f $(BINARY) $(PROCESS) *.o
	rm *.aux *.bbl *.blg *.log
