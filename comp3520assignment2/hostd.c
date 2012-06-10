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

struct pcb **input_head; // queue for holding the input jobs
struct pcb **realtimeq; // FCFS for real-time processes
struct pcb **user_queue; // where the user jobs are stored before going to round-robin queues
struct pcb ***rrqs; // an array of round robin queues
struct mab *memblock; // the main memory block
struct mab *realtimeblk; // memory for real-time processes
struct rsrcb *host; // master resource monitor
struct pcb **curr_proc; // a pointer to the currently running process

void cleanup(void) {
  if (memblock) {
    free(memblock);
  }
  if (realtimeblk) {
    free(realtimeblk);
  }
  if (host) {
    free(host);
  }
  if (input_head) {
    free(input_head);
  }
  if (realtimeq) {
    free(realtimeq);
  }
  if (user_queue) {
    free(user_queue);
  }
  if (curr_proc) {
    free(curr_proc);
  }
  if (rrqs) {
    int i;
    for (i=0; i<NO_RRQS; ++i) {
      if (rrqs[i]) {
        free(rrqs[i]);
      }
    }
    free(rrqs);
  }
}

void exitwithmsg(const char *msg) {
  printf(msg);
  cleanup();
  exit(-1);
}

void getjobs(struct pcb **q, char *filename) {
  char line[BUFFSIZE];
  FILE *in_fd = fopen(filename, "r");
  while (fgets(line, BUFFSIZE, in_fd)) { // read a line from input
    int arrtime, priority, cputime, mbytes, printer, scanner, modem, cd;
    int ret = sscanf(line,"%d, %d, %d, %d, %d, %d, %d, %d\n",&arrtime,&priority,&cputime,&mbytes,&printer,&scanner,&modem,&cd);
    if (ret != 8) { // if the wrong number of fields is read
      exitwithmsg("hostd: error in input file format\n");
    }
    struct rsrcb *res;
    if (priority != 0) { // if it's a user job, allocate its processes
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
    // create the process structure and initialise its fields
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
  fclose(in_fd); // close the input file
}

int count_nonempty(struct pcb ***q_arr) {
  // counts the number of non-empty round robin queues
  int i = 0;
  int count = 0;
  for (i=0; i<NO_RRQS; ++i) {
    if (*q_arr[i]) { // if the head of the queue exists, update the count
      ++count;
    }
  }
  return count;
}

int main(int argc, char **argv) {
  // initialise dispatcher queues (input, user and feedback/round robin)
  input_head = (struct pcb **)malloc(sizeof(struct pcb *));
  realtimeq = (struct pcb **)malloc(sizeof(struct pcb *));
  user_queue = (struct pcb **)malloc(sizeof(struct pcb *));
  rrqs = (struct pcb ***)malloc(sizeof(struct pcb **)*NO_RRQS);
  int i = 0;
  for (i=0; i<NO_RRQS; ++i) {
    rrqs[i] = (struct pcb **)malloc(sizeof(struct pcb *));
  }

  // initialise the process memory blocks
  memblock = (struct mab *)malloc(sizeof(struct mab));
  realtimeblk = (struct mab *)malloc(sizeof(struct mab));
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
  host = (struct rsrcb *)malloc(sizeof(struct rsrcb));
  host->rsrcs[PRINTER] = NPRINTERS;
  host->rsrcs[SCANNER] = NSCANNERS;
  host->rsrcs[MODEM] = NMODEMS;
  host->rsrcs[CD] = NCDS;

  // initialise dispatcher timer
  int disp_timer = 0;

  // pointer to current process
  curr_proc = (struct pcb **)malloc(sizeof(struct pcb *));
  *curr_proc = NULL; // initially, no currently running process

  // if we've provided a file name, load all jobs into the input queue
  if (argv[1]) {
    getjobs(input_head, argv[1]);
  }
  else {
    exitwithmsg("hostd: no input file specified\n");
  }

  // while there's anything in any of the queues
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
    // while there's jobs in the user queue and memory/resources can be allocated
    while (*user_queue && memchk(memblock,(*user_queue)->mbytes) && rsrcchk(host,(*user_queue)->resources)) {
      struct pcb *temp = deqpcb(user_queue); // dequeue the topmost user job
      temp->procmem = memalloc(memblock, temp->mbytes); // allocated memory
      rsrcalloc(host, temp->resources); // allocate resources
      enqpcb(rrqs[temp->priority-1], temp); // enqueue it on the appropriate round-robin queue
    }

    if (*curr_proc) { // if there's a process currently running
      (*curr_proc)->remainingcputime--; // decrememnt its remaining time
      if ((*curr_proc)->remainingcputime <= 0) { // time's up
        // terminate the process and free the associated memory
        // also free the process's allocated memory and resources
        if (terminatepcb(*curr_proc)) {
          memfree((*curr_proc)->procmem);
          rsrcfree(host, (*curr_proc)->resources);
          free(*curr_proc);
          *curr_proc = NULL;
        }
      }
      // if the current job isn't a realtime job and there's jobs waiting in any of the other queues
      else if ((*curr_proc)->priority != 0 && (count_nonempty(rrqs) > 0 || *user_queue || *input_head || *realtimeq)) {
        if (suspendpcb(*curr_proc)) { // suspend current process
          if ((*curr_proc)->priority < NO_RRQS) { // if possible, decrement the priority (higher value => lower priority)
            (*curr_proc)->priority++;
          }
          enqpcb(rrqs[(*curr_proc)->priority-1], *curr_proc); // put it back on the appropriate RR queue
          *curr_proc = NULL;
        }
      }
    }
    // if there's no process running and there's either a realtime process or a user job waiting
    if (!*curr_proc && (*realtimeq || count_nonempty(rrqs) > 0)) {
      struct pcb *proc = NULL;
      if (*realtimeq) { // if there's a realtime job, run it
        proc = deqpcb(realtimeq);
      }
      else { // else find the highest priority user job and run that
        i = 0;
        while (!proc) {
          proc = deqpcb(rrqs[i]);
          i++;
        }
      }
      if (proc->status == PCB_NEW) { // if the process is new, start it
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

  cleanup();
  return 0;
}
