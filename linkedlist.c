#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "linkedlist.h"


// Duplicate the given Value in its entirety.  If the value is a list, create
// a complete duplicate of the list
Value *duplicate(Value *val) {
    Value *new = malloc(sizeof(Value));
    assert(new != NULL);
    *new = *val;
    switch (val->type) {
        case STR_TYPE:
            new->s = malloc(sizeof(val->s));
            strcpy(new->s, val->s);
            break;
        case CONS_TYPE:
            new->c.car = duplicate(val->c.car);
            new->c.cdr = duplicate(val->c.cdr);
            break;
        default:
            ;
    }
    return new;
}

// Create a new NULL_TYPE value node.
Value *makeNull() {
    Value *new = malloc(sizeof(Value));
    assert(new != NULL);
    new->type = NULL_TYPE;
    return new;
}

// Create a new CONS_TYPE value node.
Value *cons(Value *newCar, Value *newCdr) {
    Value *new = malloc(sizeof(Value));
    assert(new != NULL);
    new->type = CONS_TYPE;
    new->c.car = newCar;
    new->c.cdr = newCdr;
    return new;
}

// Print values in a cons list, assuming the leading ( is already printed.
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
    }
}

// Display the contents of the linked list to the screen in some kind of
// readable format
void display(Value *list) {
    displayHelper(list, 1);
    printf("\b\n");
}

// Return a new list that is the reverse of the one that is passed in. All
// content within the list should be duplicated; there should be no shared
// memory whatsoever between the original list and the new one.
//
// FAQ: What if there are nested lists inside that list?
// ANS: There won't be for this assignment. There will be later, but that will
// be after we've got an easier way of managing memory.
Value *reverse(Value *list) {
    Value *newh;
    Value *new = makeNull();
    assert(list != NULL);
    assert(new != NULL);
    while (list->type == CONS_TYPE) {
        newh = malloc(sizeof(Value));
        assert(newh != NULL);
        newh->type = CONS_TYPE;
        newh->c.car = duplicate(list->c.car);
        newh->c.cdr = new;
        list = list->c.cdr;
        assert(list != NULL);
        new = newh;
    }
    return new;
}

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
void cleanup(Value *list) {
    Value *tmp;
    assert(list != NULL);
    switch(list->type) {
        case CONS_TYPE:
            while (list->type != NULL_TYPE) {
                cleanup(car(list));
                tmp = cdr(list);
                free(list);
                list = tmp; // cdr() cannot return NULL, so list is not NULL
            }
            free(list); // should be NULL_TYPE
            break;
        case STR_TYPE:
            assert(list->s != NULL);
            free(list->s);
        default:
            free(list);
    }
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

// Takes an integer indicating the total number of lists given as arguments.
// Creates a copy of the first list and appends a copy of each following list
// to the end of the previous list.
Value *append(int num, ...) {
    int i;
    va_list lists;
    Value *list = NULL, *tail, *new;
    va_start(lists, num);
    for (i = 0; i < num; i++) {
        new = va_arg(lists, Value*);
        assert(new != NULL);
        switch (new->type) {
            case CONS_TYPE:
                new = duplicate(new);   // duplicate cannot return NULL
                while (cdr(tail)->type == CONS_TYPE)    // cdr(tail) cannot be NULL
                    tail = tail->c.cdr; // no need for cdr() assertions here
                assert(cdr(tail)->type == NULL_TYPE);
                cleanup(tail->c.cdr);   // no need for cdr() assertions here
                tail->c.cdr = new;
            case NULL_TYPE:
                break;
            default:
                assert(new->type == CONS_TYPE || new->type == NULL_TYPE);
        }
    }
    va_end(lists);
    if (list == NULL) {
        return makeNull();
    }
    return list;
}
// Takes an integer indicating the total number of values given as arguments.
// Creates a linked list via cons cells of a copy of every given value.
Value *list(int num, ...) {
    int i;
    va_list values;
    Value *list, *tail, *new;
    if (num == 0)
        return makeNull();
    va_start(values, num);
    new = duplicate(va_arg(values, Value*));    // duplicate cannot return NULL
    list = cons(new, NULL);     // cons cannot return NULL
    tail = list;
    for (i = 1; i < num; i++) {
        new = duplicate(va_arg(values, Value*));    // cannot be NULL
        tail->c.cdr = cons(new, NULL);
        tail = tail->c.cdr;
    }
    tail->c.cdr = makeNull();
    return list;
}

