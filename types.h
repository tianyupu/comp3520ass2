/*
 * SID: 310182212
 * Name: Tianyu Pu
 */
#ifndef TYPES_H_
#define TYPES_H_

#include <sys/types.h>

#define TRUE 1
#define FALSE 0
#define MAXARGS 64
#define NUMRSRCTYPES 4

#define PRINTER 0
#define SCANNER 1
#define MODEM 2
#define CD 3

struct mab {
  int offset;
  int size;
  int allocated;
  struct mab *prev;
  struct mab *next;
};

struct pcb {
  pid_t pid;             // system process ID
  char *args[MAXARGS];  // program name and args
  int arrivaltime;
  int remainingcputime;
  int priority;
  int mbytes; // amount of memory the process needs
  struct mab *procmem; // the actual memory block allocated to the process
  struct rsrcb *resources;
  int status;
  struct pcb *next;     // links for pcb handlers
};

struct rsrcb {
  int rsrcs[NUMRSRCTYPES];
  int allocated;
};

#endif
