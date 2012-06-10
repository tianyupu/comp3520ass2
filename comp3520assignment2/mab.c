/*
 * SID: 310182212
 * Name: Tianyu Pu
 */
#include <stdio.h>
#include <stdlib.h>
#include "mab.h"

struct mab *memchk(struct mab *m, int size) {
  // start from the the block m and traverse right until we find
  // a suitable block (first fit memory allocation)
  if (m == NULL) {
    return NULL;
  }
  struct mab *curr = m;
  while (curr) {
    if (size <= curr->size && curr->allocated == FALSE) {
      // if it's big enough and it's free
      return curr;
    }
    curr = curr->next;
  }
  return NULL;
}

struct mab *memalloc(struct mab *m, int size) {
  // find the first available block
  struct mab *availblock = memchk(m, size);
  if (availblock == NULL) { // no block was available
    return NULL;
  }
  return memsplit(availblock, size); // split the block with the required size
}

struct mab *memfree(struct mab *m) {
  if (m == NULL) {
    return NULL;
  }
  m->allocated = FALSE; // mark as unallocated
  return memmerge(m); // merge free memory on either side of this block
}

struct mab *memmerge(struct mab *m) {
  // get the block resulting from the left merge, then do a
  // right merge from there
  struct mab *ltmerge = mergeleft(m);
  return mergeright(ltmerge);
}

struct mab *mergeleft(struct mab *m) {
  // The idea is that we start from the given block and we traverse
  // the linked list to the left until we either hit the front of the list
  // or we've hit a block that's not free. Then we merge from that block
  // to our current block to make one contiguous free block, and return it.
  if (m == NULL) {
    return NULL;
  }
  struct mab *curr = m->prev;
  struct mab *prev;
  struct mab *rtblock = m->next;
  int totalsize = m->size;
  while (curr) {
    if (curr->allocated) {
      break;
    }
    totalsize = totalsize + curr->size;
    prev = curr;
    curr = curr->prev;
    if (prev->prev == NULL) { // if we've hit the head
      prev->size = totalsize;
      prev->allocated = FALSE;
      prev->next = rtblock;
      if (rtblock) {
        rtblock->prev = prev;
      }
      free(m);
      return prev;
    }
    free(prev);
  }
  m->size = totalsize;
  m->allocated = FALSE;
  m->prev = curr;
  if (curr) {
    curr->next = m;
  }
  return m;
}

struct mab *mergeright(struct mab *m) {
  if (m == NULL) {
    return NULL;
  }
  struct mab *curr = m->next;
  struct mab *prev;
  int totalsize = m->size;
  while (curr) {
    if (curr->allocated) {
      break;
    }
    totalsize = totalsize + curr->size;
    prev = curr;
    curr = curr->next;
    free(prev);
  }
  // by the end of the loop curr is either the null at the end or a nonfree
  // so we update the block and update the prev and next pointers
  m->size = totalsize;
  m->allocated = FALSE;
  m->next = curr;
  if (curr) {
    curr->prev = m;
  }
  return m;
}

struct mab *memsplit(struct mab *m, int size) {
  if (m == NULL) {
    return NULL;
  }
  int oldsize = m->size;
  m->size = size;
  m->allocated = TRUE;
  if (size < oldsize) {
    // if the desired size is less than the available space
    // then we split the block by creating a new block
    struct mab *newmab = (struct mab *)malloc(sizeof(struct mab));
    struct mab *oldnext = m->next;
    newmab->size = oldsize - size;
    newmab->allocated = FALSE;
    newmab->next = oldnext;
    newmab->offset = m->offset + size;
    if (oldnext) {
      oldnext->prev = newmab;
    }
    m->next = newmab;
    newmab->prev = m;
    return m;
  }
  // otherwise, the entire old block is filled so we mark it as
  // used and return it
  return m;
}

void printmem(struct mab *head) {
  if (head == NULL) {
    printf("memory does not exist\n");
    return;
  }
  struct mab *curr = head;
  while (curr) {
    printf("Offset: %d, Size: %d, Alloc: %d\n", curr->offset, curr->size, curr->allocated);
    curr = curr->next;
  }
}
