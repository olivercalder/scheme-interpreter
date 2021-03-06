#include <stdbool.h>
#include "value.h"

#ifndef _LINKEDLIST
#define _LINKEDLIST

/* Create a new NULL_TYPE value node. */
Value *makeNull();

/* Creates a new VOID_TYPE value node. */
Value *makeVoid();

/* Create a new BOOL_TYPE value node with the given boolean value. */
Value *makeBool(int boolean);

/* Create a new UNSPECIFIED_TYPE value node. */
Value *makeUnspecified();

/* Create a new CONS_TYPE value node. */
Value *cons(Value *newCar, Value *newCdr);

/* Display the contents of the linked list to the given file descriptor in some
 * kind of readable format. */
void display_to_fd(Value *list, FILE *fd);

/* Display the contents of the linked list to the screen in some kind of
 * readable format. */
void display(Value *list);

/* Return a new list that is the reverse of the one that is passed in. No stored
 * data within the linked list should be duplicated; rather, a new linked list
 * of CONS_TYPE nodes should be created, that point to items in the original
 * list. */
Value *reverse(Value *list);

/* Utility to make it less typing to get car value. Use assertions to make sure
 * that this is a legitimate operation. */
Value *car(Value *list);

/* Utility to make it less typing to get cdr value. Use assertions to make sure
 * that this is a legitimate operation. */
Value *cdr(Value *list);

/* Utility to check if pointing to a NULL_TYPE value. Use assertions to make sure
 * that this is a legitimate operation. */
bool isNull(Value *value);

/* Measure length of list. Use assertions to make sure that this is a legitimate
 * operation. */
int length(Value *value);

/* Takes an integer indicating the total number of lists given as arguments.
 * Creates a copy of the first list and appends a copy of each following list
 * to the end of the previous list.  Creates new cons cells, but not new values. */
Value *append(int num, ...);

/* Takes an integer indicating the total number of values given as arguments.
 * Creates a linked list via cons cells pointing to each original value. */
Value *list(int num, ...);


#endif
