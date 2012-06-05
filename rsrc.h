#include "types.h"

struct rsrcb *rsrcchk(struct rsrcb *host, struct rsrcb *r);

struct rsrcb *rsrcalloc(struct rsrcb *host, struct rsrcb *r);

void rsrcfree(struct rsrcb *host, struct rsrcb *r);
