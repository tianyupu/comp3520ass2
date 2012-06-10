/*
 * SID: 310182212
 * Name: Tianyu Pu
 */
#include "types.h"

#define PCB_NEW 0
#define PCB_TERMINATED 1
#define PCB_SUSPENDED 2

struct pcb *startpcb(struct pcb *process);

struct pcb *terminatepcb(struct pcb *process);

struct pcb *suspendpcb(struct pcb *process);

struct pcb *restartpcb(struct pcb *process);

struct pcb *createnullpcb(void);

struct pcb *enqpcb(struct pcb **head, struct pcb *process);

struct pcb *deqpcb(struct pcb **head);

void printq(struct pcb **head);
