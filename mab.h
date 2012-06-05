#include "types.h"

struct mab *memchk(struct mab *m, int size);

struct mab *memalloc(struct mab *m, int size);

struct mab *memfree(struct mab *m);

struct mab *memmerge(struct mab *m);

struct mab *mergeleft(struct mab *m);

struct mab *mergeright(struct mab *m);

struct mab *memsplit(struct mab *m, int size);

void printmem(struct mab *head);
