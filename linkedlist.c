#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "linkedlist.h"
#include "talloc.h"

// Create a new NULL_TYPE value node.
Value *makeNull() {
    Value *new = talloc(sizeof(Value));
    new->type = NULL_TYPE;
    return new;
}

// Create a new CONS_TYPE value node.
Value *cons(Value *newCar, Value *newCdr) {
    Value *new = talloc(sizeof(Value));
    new->type = CONS_TYPE;
    new->c.car = newCar;
    new->c.cdr = newCdr;
    return new;
}

// Print values in a cons list
void displayHelper(Value *list, int first) {
    if (first) {
        switch (list->type) {
            case CONS_TYPE:
                printf("(");
                break;
            case NULL_TYPE:
                printf("( ");   // NULL_TYPE prints "\b)" in helper function
                break;
            default:
                ;
        }
    }
    switch(list->type) {
        case INT_TYPE:
            printf("%d ", list->i);
            break;
        case DOUBLE_TYPE:
            printf("%lf ", list->d);
            break;
        case STR_TYPE:
            printf("%s ", list->s);
            break;
        case CONS_TYPE:
            displayHelper(list->c.car, 1);  // if car is CONS_TYPE, restart
            displayHelper(list->c.cdr, 0);
            break;
        case NULL_TYPE:
            printf("\b) ");
            break;
        case PTR_TYPE:
            printf("%p ", list->p);
            break;
        case BOOL_TYPE:
            if (list->i == 0)
                printf("#f ");
            else
                printf("#t ");
            break;
        default:
            fprintf(stderr, "WARNING: Value type %d should not be printable\n", list->type);
    }
}

// Display the contents of the linked list to the screen in some kind of
// readable format
void display(Value *list) {
    displayHelper(list, 1);
    printf("\b\n");
}

// Return a new list that is the reverse of the one that is passed in. No stored
// data within the linked list should be duplicated; rather, a new linked list
// of CONS_TYPE nodes should be created, that point to items in the original
// list.
Value *reverse(Value *list) {
    Value *newh;
    Value *new = makeNull();
    assert(list != NULL);
    while (list->type == CONS_TYPE) {
        newh = talloc(sizeof(Value));
        newh->type = CONS_TYPE;
        newh->c.car = list->c.car;
        newh->c.cdr = new;
        list = list->c.cdr;
        assert(list != NULL);
        new = newh;
    }
    return new;
}

// Utility to make it less typing to get car value. Use assertions to make sure
// that this is a legitimate operation.
Value *car(Value *list) {
    assert(list != NULL);
    assert(list->type == CONS_TYPE);
    assert(list->c.car != NULL);
    return list->c.car;
}

// Utility to make it less typing to get cdr value. Use assertions to make sure
// that this is a legitimate operation.
Value *cdr(Value *list) {
    assert(list != NULL);
    assert(list->type == CONS_TYPE);
    assert(list->c.cdr != NULL);
    return list->c.cdr;
}

// Utility to check if pointing to a NULL_TYPE value. Use assertions to make sure
// that this is a legitimate operation.
bool isNull(Value *value) {
    assert(value != NULL);
    return (value->type == NULL_TYPE);
}

// Measure length of list. Use assertions to make sure that this is a legitimate
// operation.
int length(Value *value) {
    int len = 0;
    assert(value != NULL);
    while (value->type == CONS_TYPE) {
        len++;
        value = value->c.cdr;
        assert(value != NULL);
    }
    assert(value->type == NULL_TYPE);
    return len;
}

// Duplicates a list by creating new cons cells for each entry, but preserving
// the original car values.  Does not duplicate the final NULL_TYPE list.
// Returns a pointer to the head of the new list.  If the cdr of the head is
// NULL_TYPE, then updates the value at tail to be the address of head, which
// is the last non-NULL_TYPE cons cell in the list.
Value *duplicateList(Value *list, Value **tail) {
    Value *new;
    assert(list != NULL);
    new->type = CONS_TYPE;
    switch (list->type) {
        case CONS_TYPE:
            new = talloc(sizeof(Value));
            new->c.car = list->c.car;
            new->c.cdr = duplicateList(list->c.cdr, tail);
            if (new->c.cdr->type == NULL_TYPE)
                *tail = list;
            break;
        case NULL_TYPE:
            return list;
        default:
            assert(list->type == CONS_TYPE || list->type == NULL_TYPE);
    }
    return new;
}

// Takes an integer indicating the total number of lists given as arguments.
// Creates a copy of the first list and appends a copy of each following list
// to the end of the previous list.  Creates new cons cells, but not new values.
Value *append(int num, ...) {
    int i;
    va_list lists;
    Value *list = NULL, *tail = NULL, *new, *new_tail;
    va_start(lists, num);
    for (i = 0; i < num; i++) {
        new = va_arg(lists, Value*);
        assert(new != NULL);
        switch (new->type) {
            case CONS_TYPE:
                new = duplicateList(new, &new_tail);    // duplicateList cannot return NULL
                if (tail == NULL)
                    list = new;
                else
                    tail->c.cdr = new;
                tail = new_tail;
            case NULL_TYPE:
                break;
            default:
                assert(new->type == CONS_TYPE || new->type == NULL_TYPE);
        }
    }
    va_end(lists);
    if (list == NULL) {
        list = makeNull();
    }
    return list;
}

// Takes an integer indicating the total number of values given as arguments.
// Creates a linked list via cons cells pointing to each original value.
Value *list(int num, ...) {
    int i;
    va_list values;
    Value *list, *tail, *new;
    if (num == 0)
        return makeNull();
    va_start(values, num);
    new = va_arg(values, Value*);
    list = cons(new, NULL); // cons cannot return NULL
    tail = list;
    for (i = 1; i < num; i++) {
        new = va_arg(values, Value*);
        tail->c.cdr = cons(new, NULL);  // cannot be NULL
        tail = tail->c.cdr;
    }
    tail->c.cdr = makeNull();
    return list;
}

