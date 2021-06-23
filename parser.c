#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "value.h"
#include "talloc.h"
#include "linkedlist.h"


/* Handle singlequote, and in particular, dot types in singlequote blocks. */
Value *handle_singlequotes(Value *tree) {
    Value *current, *token, *prev, *next, *tmp;
    prev = NULL;
    current = tree;
    while (current->type == CONS_TYPE) {
        token = car(current);
        next = cdr(current);
        switch (token->type) {
            case CONS_TYPE:
                current->c.car = handle_singlequotes(token);
                break;
            case DOT_TYPE:
                if (car(next)->type == CONS_TYPE)
                    next->c.car = handle_singlequotes(car(next));
                if (cdr(next)->type != NULL_TYPE) {
                    fprintf(stderr, "Syntax error: failed to parse DOT_TYPE: missing close paren: ");
                    display_file_descriptor(tree, stderr);
                    texit(3);
                }
                prev->c.cdr = car(next);
                current = next;
                break;
            case SINGLEQUOTE_TYPE:
                tmp = talloc(sizeof(Value));
                tmp->type = SYMBOL_TYPE;
                tmp->s = malloc(sizeof("quote"));
                strcpy(tmp->s, "quote");
                if (car(next)->type == CONS_TYPE)
                    next->c.car = handle_singlequotes(car(next));
                current->c.car = cons(tmp, next);
                current->c.cdr = cdr(next);
                next->c.cdr = makeNull();
            default:
                break;
        }
        prev = current;
        current = cdr(current);
    }
    return tree;
}

/* Takes a list of tokens from a Scheme program, and returns a pointer to a
 * parse tree representing that program. */
Value *parse(Value *tokens) {
    Value *tree, *current, *token, *tmp, *tmp_val, *tmp_stack;
    int depth = 0;
    // Parse recursively, whenever an open parenthesis is seen
    // Whenever a ' is seen, parse the next item (tree, if the next is an open-
    // type), then add a parse tree consisting of a quote symbol followed by the
    // subsequent parsed entry.
    assert(tokens != NULL);
    tree = makeNull();
    tmp_stack = makeNull();
    current = tokens;
    while (current->type == CONS_TYPE) {
        token = car(current);
        switch (token->type) {
            case OPEN_TYPE:
            case OPENBRACKET_TYPE:
                depth++;
            case INT_TYPE:
            case DOUBLE_TYPE:
            case STR_TYPE:
            case BOOL_TYPE:
            case SYMBOL_TYPE:
            case SINGLEQUOTE_TYPE:
            case DOT_TYPE:
                tree = cons(token, tree);
                break;
            case CLOSE_TYPE:
            case CLOSEBRACKET_TYPE:
                depth--;
                tmp_stack = makeNull();
CLOSE_TYPE_LOOP:
                // Avoid while loop so breaks in switch statement exit the loop.
                // Could also use while (tmp->type != NULL_TYPE) and have a goto
                // to leave the while loop, but this seems clearer since most
                // programs should have matching parentheses and thus not hit
                // the NULL_TYPE tmp value.
                tmp = tree;
                if (tmp->type == NULL_TYPE) {
                    fprintf(stderr, "Syntax error: close parenthesis with no matching open parenthesis\n");
                    texit(3);
                }
                tmp_val = car(tmp);
                tree = cdr(tree);
                switch (tmp_val->type) {
                    case OPEN_TYPE:
                    case OPENBRACKET_TYPE:
                        if (token->type - 1 != tmp_val->type) { // CLOSE*_TYPE - 1 is OPEN*_TYPE
                            fprintf(stderr, "Syntax error: mismatched bracket or parenthesis\n");
                            texit(3);
                        }
                        tree = cons(tmp_stack, tree);
                        break;
                    default:
                        // tmp is already a cons cell, no need to allocate
                        // another to add to the stack
                        tmp->c.cdr = tmp_stack;
                        tmp_stack = cons(tmp_val, tmp_stack);
                        goto CLOSE_TYPE_LOOP;
                }
                break;
            default:
                // Should not be possible to get here
                fprintf(stderr, "Syntax error: invalid token of type %d in token list\n", token->type);
                texit(3);
        }
        current = cdr(current);
    }
    if (depth != 0) {
        fprintf(stderr, "Syntax error: open parenthesis with no matching close parenthesis\n");
        texit(3);
    }
    tree = reverse(tree);
    tree = handle_singlequotes(tree);
    return tree;
}


/* Prints the tree to the screen in a readable fashion. It should look just like
 * Scheme code; use parentheses to indicate subtrees. */
void printTree(Value *tree) {
    Value *current = tree;
    while (current->type == CONS_TYPE) {
        display(car(current));
        current = cdr(current);
    }
}

