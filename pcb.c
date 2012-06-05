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
  int a = process->priority;
  int b = process->remainingcputime;
  int c = process->procmem->offset;
  int d = process->procmem->size;
  int e,f,g,h;
  if (a == 0) {
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
  printf("pid\tpty\trem\toffset\tsize\tprinter\tscanner\tmodem\tcd\n");
  printf("%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",pid,a,b,c,d,e,f,g,h);
  process->pid = pid;
  return process;
}

struct pcb *terminatepcb(struct pcb *process) {
  int status;
  kill(process->pid, SIGINT);
  waitpid(process->pid, &status, WUNTRACED);
  process->status = PCB_TERMINATED;
  return process;
}

struct pcb *suspendpcb(struct pcb *process) {
  int status;
  kill(process->pid, SIGTSTP);
  waitpid(process->pid, &status, WUNTRACED);
  process->status = PCB_SUSPENDED;
  return process;
}

struct pcb *restartpcb(struct pcb *process) {
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
  if (*head == NULL) {
    *head = process; // if the head is empty, set head to be the process
    return *head;
  }
  struct pcb *curr = *head;
  while (curr->next) {
    curr = curr->next;
  }
  curr->next = process;
  return *head;
}

struct pcb *deqpcb(struct pcb **head) {
  if (*head == NULL) {
    return NULL;
  }
  struct pcb *old = *head;
  struct pcb *new = (*head)->next;
  *head = new;
  old->next = NULL;
  return old;
}

void printq(struct pcb **head) {
  if (!*head) {
    return;
  }
  struct pcb *curr = *head;
  while (curr->next) {
    printf("ID: %d, arr time: %d, mbytes: %d\n", curr->pid, curr->arrivaltime, curr->mbytes);
    curr = curr->next;
  }
  printf("ID: %d, arr time: %d, mbytes: %d\n", curr->pid, curr->arrivaltime, curr->mbytes);
}
