#include <stdbool.h>
#include "value.h"

#ifndef _LINKEDLIST
#define _LINKEDLIST

// Create a new NULL_TYPE value node.
Value *makeNull();

// Create a new CONS_TYPE value node.
Value *cons(Value *newCar, Value *newCdr);

// Display the contents of the linked list to the screen in some kind of
// readable format
void display(Value *list);

// Return a new list that is the reverse of the one that is passed in. All
// content within the list should be duplicated; there should be no shared
// memory whatsoever between the original list and the new one.
//
// FAQ: What if there are nested lists inside that list?
// ANS: There won't be for this assignment. There will be later, but that will
// be after we've got an easier way of managing memory.
Value *reverse(Value *list);

// Frees up all memory directly or indirectly referred to by list. This includes strings.
//
// FAQ: What if a string being pointed to is a string literal? That throws an
// error when freeing.
//
// ANS: Don't put a string literal into the list in the first place. All strings
// added to this list should be able to be freed by the cleanup function.
//
// FAQ: What if there are nested lists inside that list?
//
// ANS: There won't be for this assignment. There will be later, but that will
// be after we've got an easier way of managing memory.
void cleanup(Value *list);

// Utility to make it less typing to get car value. Use assertions to make sure
// that this is a legitimate operation.
Value *car(Value *list);

// Utility to make it less typing to get cdr value. Use assertions to make sure
// that this is a legitimate operation.
Value *cdr(Value *list);

// Utility to check if pointing to a NULL_TYPE value. Use assertions to make sure
// that this is a legitimate operation.
bool isNull(Value *value);

// Measure length of list. Use assertions to make sure that this is a legitimate
// operation.
int length(Value *value);

// Takes an integer indicating the total number of lists given as arguments.
// Creates a copy of the first list and appends a copy of each following list
// to the end of the previous list.
Value *append(int num, ...);

// Takes an integer indicating the total number of values given as arguments.
// Creates a linked list via cons cells of a copy of every given value.
Value *list(int num, ...);

#endif
