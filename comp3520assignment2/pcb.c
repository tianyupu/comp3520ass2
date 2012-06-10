/*
 * SID: 310182212
 * Name: Tianyu Pu
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "pcb.h"

struct pcb *startpcb(struct pcb *process) {
  if (process == NULL) {
    return NULL;
  }
  pid_t pid = 0;
  switch (pid = fork()) {
    case -1:
      printf("hostd: error forking a child process\n");
      return NULL;
    case 0:
      execvp(process->args[0], process->args);
      printf("hostd: error executing process\n");
      return NULL;
  }
  // retrieve the fields for printing
  int a = process->priority;
  int b = process->remainingcputime;
  int c = process->procmem->offset;
  int d = process->procmem->size;
  int e,f,g,h;
  if (a == 0) { // realtime processes have priority 0 and no resources
    e = 0;
    f = 0;
    g = 0;
    h = 0;
  }
  else {
    e = process->resources->rsrcs[PRINTER];
    f = process->resources->rsrcs[SCANNER];
    g = process->resources->rsrcs[MODEM];
    h = process->resources->rsrcs[CD];
  }
  printf("pid\tprty\trem\toffset\tsize\tprinter\tscanner\tmodem\tcd\tstatus\n");
  printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\tRUNNING\n",pid,a,b,c,d,e,f,g,h);
  process->pid = pid;
  return process;
}

struct pcb *terminatepcb(struct pcb *process) {
  if (process == NULL) {
    return NULL;
  }
  int status;
  kill(process->pid, SIGINT);
  waitpid(process->pid, &status, WUNTRACED);
  process->status = PCB_TERMINATED;
  return process;
}

struct pcb *suspendpcb(struct pcb *process) {
  if (process == NULL) {
    return NULL;
  }
  int status;
  kill(process->pid, SIGTSTP);
  waitpid(process->pid, &status, WUNTRACED);
  process->status = PCB_SUSPENDED;
  return process;
}

struct pcb *restartpcb(struct pcb *process) {
  if (process == NULL) {
    return NULL;
  }
  if (kill(process->pid, SIGCONT)) {
    return NULL;
  }
  return process;
}

struct pcb *createnullpcb(void) {
  struct pcb *newpcb = (struct pcb *)malloc(sizeof(struct pcb));
  if (newpcb) {
    newpcb->pid = 0;
    return newpcb;
  }
  else {
    return NULL;
  }
}

struct pcb *enqpcb(struct pcb **head, struct pcb *process) {
  if (process == NULL || head == NULL) {
    return NULL;
  }
  if (*head == NULL) {
    *head = process; // if the head is empty, set head to be the process
    return *head;
  }
  // else, traverse the linked list until we come to the end
  // then we queue the given process onto the end
  struct pcb *curr = *head;
  while (curr->next) {
    curr = curr->next;
  }
  curr->next = process;
  return *head;
}

struct pcb *deqpcb(struct pcb **head) {
  if (head == NULL) {
    return NULL;
  }
  if (*head == NULL) { // if there's nothing at the head, we can't dequeue anything
    return NULL;
  }
  // else, remove the current head and update the head pointer
  struct pcb *old = *head;
  struct pcb *new = (*head)->next;
  *head = new;
  old->next = NULL;
  return old; // return the process we just removed
}

void printq(struct pcb **head) {
  if (head == NULL) {
    return;
  }
  if (!*head) { // there's nothing at the head, so the list is empty
    return;
  }
  struct pcb *curr = *head;
  while (curr->next) { 
  // traverse the linked list, printing out various process fields for debugging
    printf("ID: %d, arr time: %d, mbytes: %d\n", curr->pid, curr->arrivaltime, curr->mbytes);
    curr = curr->next;
  }
  printf("ID: %d, arr time: %d, mbytes: %d\n", curr->pid, curr->arrivaltime, curr->mbytes);
}
