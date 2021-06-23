#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "linkedlist.h"
#include "talloc.h"

/* Create a new NULL_TYPE value node. */
Value *makeNull() {
    Value *new = talloc(sizeof(Value));
    new->type = NULL_TYPE;
    return new;
}

/* Create a new CONS_TYPE value node. */
Value *cons(Value *newCar, Value *newCdr) {
    Value *new = talloc(sizeof(Value));
    new->type = CONS_TYPE;
    new->c.car = newCar;
    new->c.cdr = newCdr;
    return new;
}

struct format_info {
    int first_in_list, leading_space, is_list;
};

/* Print values in a cons list. */
int displayHelper(Value *list, struct format_info *info, FILE *fd) {
    struct format_info car_info, cdr_info;
    if (info->leading_space && list->type != NULL_TYPE)
        fprintf(fd, " ");
    if (info->first_in_list) {
        switch (list->type) {
            case CONS_TYPE:
            case NULL_TYPE:
                fprintf(fd, "(");
            default:
                ;
        }
    }
    if (info->is_list) {
        // Should be CONS_TYPE or NULL_TYPE, or print a .
        switch (list->type) {
            case CONS_TYPE:
            case NULL_TYPE:
                break;
            default:
                fprintf(fd, ". ");
        }
    }
    switch(list->type) {
        case INT_TYPE:
            fprintf(fd, "%d", list->i);
            return 1;
        case DOUBLE_TYPE:
            fprintf(fd, "%lf", list->d);
            return 1;
        case STR_TYPE:
            fprintf(fd, "%s", list->s);
            return 1;
        case CONS_TYPE:
            car_info.first_in_list = 1;
            car_info.leading_space = 0;
            car_info.is_list = 0;
            cdr_info.first_in_list = 0;
            cdr_info.leading_space = displayHelper(list->c.car, &car_info, fd);
            cdr_info.is_list = 1;
            return displayHelper(list->c.cdr, &cdr_info, fd);
        case NULL_TYPE:
            fprintf(fd, ")");
            return 1;
        case PTR_TYPE:
            fprintf(fd, "%p", list->p);
            return 1;
        case BOOL_TYPE:
            if (list->i == 0)
                fprintf(fd, "#f");
            else
                fprintf(fd, "#t");
            return 1;
        case SYMBOL_TYPE:
            fprintf(fd, "%s", list->s);
            return 1;
        case DOT_TYPE:
            fprintf(fd, ".");
            return 1;
        case SINGLEQUOTE_TYPE:
            fprintf(fd, "'");
            return 0;
        default:
            fprintf(stderr, "WARNING: Value type %d should not be printable\n", list->type);
    }
    return 0;
}

/* Display the contents of the linked list to the given file descriptor in some
 * kind of readable format. */
void display_file_descriptor(Value *list, FILE *fd) {
    struct format_info info = {1, 0, 0};
    displayHelper(list, &info, fd);
    fprintf(fd, "\n");
}

/* Display the contents of the linked list to the screen in some kind of
 * readable format. */
void display(Value *list) {
    display_file_descriptor(list, stdout);
}

/* Return a new list that is the reverse of the one that is passed in. No stored
 * data within the linked list should be duplicated; rather, a new linked list
 * of CONS_TYPE nodes should be created, that point to items in the original
 * list. */
Value *reverse(Value *list) {
    Value *newh, *current;
    Value *new = makeNull();
    assert(list != NULL);
    current = list;
    while (current->type == CONS_TYPE) {
        newh = talloc(sizeof(Value));
        newh->type = CONS_TYPE;
        newh->c.car = current->c.car;
        newh->c.cdr = new;
        current = current->c.cdr;
        assert(current != NULL);
        new = newh;
    }
    if (current->type != NULL_TYPE) {
        fprintf(stderr, "ERROR: In procedure reverse: Wrong type argument: ");
        display_file_descriptor(list, stderr);
        texit(1);
    }
    return new;
}

/* Utility to make it less typing to get car value. Use assertions to make sure
 * that this is a legitimate operation. */
Value *car(Value *list) {
    assert(list != NULL);
    assert(list->type == CONS_TYPE);
    assert(list->c.car != NULL);
    return list->c.car;
}

/* Utility to make it less typing to get cdr value. Use assertions to make sure
 * that this is a legitimate operation. */
Value *cdr(Value *list) {
    assert(list != NULL);
    assert(list->type == CONS_TYPE);
    assert(list->c.cdr != NULL);
    return list->c.cdr;
}

/* Utility to check if pointing to a NULL_TYPE value. Use assertions to make sure
 * that this is a legitimate operation. */
bool isNull(Value *value) {
    assert(value != NULL);
    return (value->type == NULL_TYPE);
}

/* Measure length of list. Use assertions to make sure that this is a legitimate
 * operation. */
int length(Value *value) {
    Value *current = value;
    int len = 0;
    assert(current != NULL);
    while (current->type == CONS_TYPE) {
        len++;
        current = current->c.cdr;
        assert(current != NULL);
    }
    if (current->type != NULL_TYPE) {
        fprintf(stderr, "ERROR: In procedure length: Wrong type argument: ");
        display_file_descriptor(value, stderr);
        texit(1);
    }
    return len;
}

/* Duplicates a list by creating new cons cells for each entry, but preserving
 * the original car values.  Does not duplicate the final NULL_TYPE list.
 * Returns a pointer to the head of the new list.  If the cdr of the head is
 * NULL_TYPE, then updates the value at tail to be the address of head, which
 * is the last non-NULL_TYPE cons cell in the list. */
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

/* Takes an integer indicating the total number of lists given as arguments.
 * Creates a copy of the first list and appends a copy of each following list
 * to the end of the previous list.  Creates new cons cells, but not new values. */
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

/* Takes an integer indicating the total number of values given as arguments.
 * Creates a linked list via cons cells pointing to each original value. */
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

