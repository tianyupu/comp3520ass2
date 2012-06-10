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
  if (r->allocated) { // the resource is already allocated
    return NULL;
  }
  int total = 0;
  int i;
  for (i=0; i<NUMRSRCTYPES; ++i) { // compare the fields of each resource
    if (host->rsrcs[i] >= r->rsrcs[i]) {
      total = total + 1;
    }
  }
  if (total == NUMRSRCTYPES) {
  // this means that for each of the fields, the host's count was greater than
  // the required count, so we're able to allocate the resource
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
  // subtract the resource counts from the main resource monitor
    host->rsrcs[i] = host->rsrcs[i] - r->rsrcs[i];
  }
  r->allocated = TRUE;
  return r;
}

void rsrcfree(struct rsrcb *host, struct rsrcb *r) {
  if (!host || !r) {
    return;
  }
  if (!r->allocated) { // we don't need to free a resource that's not allocated
    return;
  }
  int i;
  for (i=0; i<NUMRSRCTYPES; ++i) {
  // add the resource counts back to the main resource monitor
    host->rsrcs[i] = host->rsrcs[i] + r->rsrcs[i];
  }
  // free the resource struct so we don't memory leak!
  free(r);
}
