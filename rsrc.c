/*
 * SID: 310182212
 * Name: Tianyu Pu
 */
#include <stdlib.h>
#include "rsrc.h"

struct rsrcb *rsrcchk(struct rsrcb *host, struct rsrcb *r) {
  if (!host || !r) {
    return NULL;
  }
  if (r->allocated) {
    return NULL;
  }
  int total = 0;
  int i;
  for (i=0; i<NUMRSRCTYPES; ++i) {
    if (host->rsrcs[i] >= r->rsrcs[i]) {
      total = total + 1;
    }
  }
  if (total == NUMRSRCTYPES) {
    return r;
  }
  return NULL;
}

struct rsrcb *rsrcalloc(struct rsrcb *host, struct rsrcb *r) {
  if (!rsrcchk(host, r)) {
    return NULL;
  }
  int i;
  for (i=0; i<NUMRSRCTYPES; ++i) {
    host->rsrcs[i] = host->rsrcs[i] - r->rsrcs[i];
  }
  r->allocated = TRUE;
  return r;
}

void rsrcfree(struct rsrcb *host, struct rsrcb *r) {
  if (!host || !r) {
    return;
  }
  if (!r->allocated) {
    return;
  }
  int i;
  for (i=0; i<NUMRSRCTYPES; ++i) {
    host->rsrcs[i] = host->rsrcs[i] + r->rsrcs[i];
  }
  free(r);
}
