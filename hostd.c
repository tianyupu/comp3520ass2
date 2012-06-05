/*
 * SID: 310182212
 * Name: Tianyu Pu
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "pcb.h"
#include "mab.h"
#include "rsrc.h"

#define BUFFSIZE 1024
#define NO_RRQS 3
#define NPRINTERS 2
#define NSCANNERS 1
#define NMODEMS 1
#define NCDS 2
#define REALTIMEMEM 64
#define USERPROCMEM 960

void exitwithmsg(const char *msg) {
  // TODO: needs to clean up
  printf(msg);
  exit(-1);
}

void getjobs(struct pcb **q, char *filename) {
  char line[BUFFSIZE];
  FILE *in_fd = fopen(filename, "r");
  while (fgets(line, BUFFSIZE, in_fd)) {
    int arrtime, priority, cputime, mbytes, printer, scanner, modem, cd;
    int ret = sscanf(line,"%d, %d, %d, %d, %d, %d, %d, %d\n",&arrtime,&priority,&cputime,&mbytes,&printer,&scanner,&modem,&cd);
    if (ret != 8) {
      exitwithmsg("hostd: error in input file format\n");
    }
    struct rsrcb *res;
    if (priority != 0) {
      res = (struct rsrcb *)malloc(sizeof(struct rsrcb));
      res->rsrcs[PRINTER] = printer;
      res->rsrcs[SCANNER] = scanner;
      res->rsrcs[MODEM] = modem;
      res->rsrcs[CD] = cd;
      res->allocated = FALSE;
    }
    else {
      res = NULL;
    }
    struct pcb *newpcb = createnullpcb();
    newpcb->args[0] = "./process";
    newpcb->args[1] = NULL;
    newpcb->arrivaltime = arrtime;
    newpcb->remainingcputime = cputime;
    newpcb->priority = priority;
    newpcb->mbytes = mbytes;
    newpcb->procmem = NULL;
    newpcb->resources = res;
    newpcb->status = PCB_NEW;
    enqpcb(q, newpcb);
  }
  fclose(in_fd);
}

int count_nonempty(struct pcb ***q_arr) {
  // counts the number of non-empty round robin queues
  int i = 0;
  int count = 0;
  for (i=0; i<NO_RRQS; ++i) {
    if (*q_arr[i]) {
      ++count;
    }
  }
  return count;
}

int main(int argc, char **argv) {
  // initialise dispatcher queues (input, user and feedback/round robin)
  struct pcb **input_head = (struct pcb **)malloc(sizeof(struct pcb *));
  struct pcb **realtimeq = (struct pcb **)malloc(sizeof(struct pcb *));
  struct pcb **user_queue = (struct pcb **)malloc(sizeof(struct pcb *));
  struct pcb ***rrqs = (struct pcb ***)malloc(sizeof(struct pcb **)*NO_RRQS);
  int i = 0;
  for (i=0; i<NO_RRQS; ++i) {
    rrqs[i] = (struct pcb **)malloc(sizeof(struct pcb *));
  }

  // initialise the process memory blocks
  struct mab *memblock = (struct mab *)malloc(sizeof(struct mab));
  struct mab *realtimeblk = (struct mab *)malloc(sizeof(struct mab));
  if (!memblock || !realtimeblk) {
    exitwithmsg("hostd: error allocating memory block\n");
  }
  memblock->size = USERPROCMEM;
  memblock->allocated = FALSE;
  memblock->prev = NULL;
  memblock->next = NULL;
  memblock->offset = REALTIMEMEM;
  realtimeblk->size = REALTIMEMEM;
  realtimeblk->allocated = FALSE;
  realtimeblk->prev = NULL;
  realtimeblk->next = NULL;
  realtimeblk->offset = 0;

  // initialise master resource block
  struct rsrcb *host = (struct rsrcb *)malloc(sizeof(struct rsrcb));
  host->rsrcs[PRINTER] = NPRINTERS;
  host->rsrcs[SCANNER] = NSCANNERS;
  host->rsrcs[MODEM] = NMODEMS;
  host->rsrcs[CD] = NCDS;

  // initialise dispatcher timer
  int disp_timer = 0;

  // pointer to current process
  struct pcb **curr_proc = (struct pcb **)malloc(sizeof(struct pcb *));
  *curr_proc = NULL; // initially, no currently running process

  // if we've provided a file name, load all jobs into the input queue
  if (argv[1]) {
    getjobs(input_head, argv[1]);
  }
  else {
    exitwithmsg("hostd: no input file specified\n");
  }

  while (*input_head || *realtimeq || *user_queue || *curr_proc || count_nonempty(rrqs) > 0) {
    // unload pending processes from input queue
    while (*input_head && (*input_head)->arrivaltime <= disp_timer) {
      struct pcb *temp = deqpcb(input_head);
      if (temp->priority == 0) { // priority 0 processes are realtime
        if (memchk(realtimeblk, temp->mbytes)) {
          temp->procmem = memalloc(realtimeblk, temp->mbytes);
        }
        enqpcb(realtimeq, temp);
        continue;
      }
      enqpcb(user_queue, temp);
    }
    while (*user_queue && memchk(memblock, (*user_queue)->mbytes) && rsrcchk(host, (*user_queue)->resources)) {
      struct pcb *temp = deqpcb(user_queue);
      temp->procmem = memalloc(memblock, temp->mbytes);
      //temp->resources = rsrcalloc(host, temp->resources);
      rsrcalloc(host, temp->resources);
      enqpcb(rrqs[temp->priority-1], temp);
    }

    if (*curr_proc) { // if there's a process currently running
      (*curr_proc)->remainingcputime--;
      if ((*curr_proc)->remainingcputime <= 0) { // time's up
        // terminate the process and free the associated memory
        // also free the process's allocated memory
        if (terminatepcb(*curr_proc)) {
          memfree((*curr_proc)->procmem);
          rsrcfree(host, (*curr_proc)->resources);
          free(*curr_proc);
          *curr_proc = NULL;
        }
      }
      else if ((*curr_proc)->priority != 0 && (count_nonempty(rrqs) > 0 || *user_queue || *input_head || *realtimeq)) {
        if (suspendpcb(*curr_proc)) { // suspend current process
          if ((*curr_proc)->priority < NO_RRQS) { // if possible, decrement the priority (higher value => lower priority)
            (*curr_proc)->priority++;
          }
          enqpcb(rrqs[(*curr_proc)->priority-1], *curr_proc); // put it back on the RR queue
          *curr_proc = NULL;
        }
      }
    }
    if (!*curr_proc && (*realtimeq || count_nonempty(rrqs) > 0)) {
      struct pcb *proc = NULL;
      if (*realtimeq) {
        proc = deqpcb(realtimeq);
      }
      else {
        i = 0;
        while (!proc) {
          proc = deqpcb(rrqs[i]);
          i++;
        }
      }
      if (proc->status == PCB_NEW) {
        startpcb(proc);
      }
      else {
        if (proc->status == PCB_SUSPENDED) {
          restartpcb(proc);
        }
      }
      *curr_proc = proc;
    }
    sleep(1);
    disp_timer++;
  }

  free(memblock);
  free(realtimeblk);
  free(host);
  free(input_head);
  free(realtimeq);
  free(user_queue);
  free(curr_proc);
  for (i=0; i<NO_RRQS; ++i) {
    free(rrqs[i]);
  }
  free(rrqs);
  return 0;
}
