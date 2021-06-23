#include <assert.h>
#include "talloc.h"

Value *ACTIVE_LIST = NULL;
size_t TALLOC_MEM_COUNT = 0;

/* Replacement for malloc that stores the pointers allocated. It should store
 * the pointers in some kind of list; a linked list would do fine, but insert
 * here whatever code you'll need to do so; don't call functions in the
 * pre-existing linkedlist.h. Otherwise you'll end up with circular
 * dependencies, since you're going to modify the linked list to use talloc. */
void *talloc(size_t size) {
    void *loc;
    Value *new_cons;
    loc = malloc(size);
    TALLOC_MEM_COUNT += size;
    assert(loc != NULL);
    new_cons = malloc(sizeof(Value));
    TALLOC_MEM_COUNT += sizeof(Value);
    assert(new_cons != NULL);
    new_cons->type = CONS_TYPE;
    new_cons->c.car = loc;
    new_cons->c.cdr = ACTIVE_LIST;
    ACTIVE_LIST = new_cons;
    return loc;
}

/* Free all pointers allocated by talloc, as well as whatever memory you
 * allocated in lists to hold those pointers. */
void tfree() {
    Value *curr;
    assert(ACTIVE_LIST != NULL);
    while (ACTIVE_LIST != NULL) {
        curr = ACTIVE_LIST;
        ACTIVE_LIST = ACTIVE_LIST->c.cdr;
        free(curr->c.car);
        free(curr);
    }
}

/* Replacement for the C function "exit", that consists of two lines: it calls
 * tfree before calling exit. It's useful to have later on; if an error happens,
 * you can exit your program, and all memory is automatically cleaned up. */
void texit(int status) {
    tfree();
    exit(status);
}

/* Returns the amount of memory currently in use by talloc, including the memory
 * used to store the talloc active list.  Does not compensate for alignment;
 * that is, calling malloc(1) will in reality allocate more than 1 byte on the
 * heap, but this function only returns the amount of memory requested through
 * talloc, in this case 1, in addition to the memory needed for the list itself.
 * Similarly, internal and external fragmentation are not accounted for with
 * respect to Value structs, which are 4+8+8 bytes, which will likely be allocated
 * as either 24 or 32 bytes total. */
size_t tallocMemoryCount() {
    return TALLOC_MEM_COUNT;
}

