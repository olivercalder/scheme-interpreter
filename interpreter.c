#include <stdio.h>
#include <string.h>
#include "value.h"
#include "talloc.h"
#include "linkedlist.h"

Value *lookup_symbol(Value *expr, Frame *frame) {
    Value *value, *pair;
    Frame *current = frame;
    if (expr->type != SYMBOL_TYPE) {
        fprintf(stderr, "Evaluation error: called lookup_symbol on value of type %d\n", expr->type);
        texit(4);
    }
    while (frame != NULL) {
        value = frame->bindings;
        while (value->type == CONS_TYPE) {
            pair = car(value);
            if (strcmp(car(pair)->s, expr->s) == 0) {
                return cdr(pair);
            }
            value = cdr(value);
        }
        frame = frame->parent;
    }
    // if while loops end, never found matching symbol in frames
    fprintf(stderr, "Evaluation error: unknown symbol: %s\n", expr->s);
    texit(4);
    return expr;    // to appease the compiler -- will never actually return this
}

Value *eval(Value *expr, Frame *frame);

Value *eval_begin(Value *args, Frame *frame) {
    Value *current = args, *result = NULL;
    while (current->type == CONS_TYPE) {
        result = eval(car(current), frame);
        current = cdr(current);
    }
    if (result == NULL) {
        result = makeVoid();    // sequence of zero expressions
    }
    return result;
}

Value *eval_not(Value *args, Frame *frame) {
    Value *cond, *result;
    int argc = length(args);
    if (argc != 1) {
        fprintf(stderr, "Evaluation error: built-in function `not`: expected 1 argument, received %d\n", argc);
        texit(4);
    }
    cond = eval(car(args), frame);
    if (cond->type != BOOL_TYPE) {
        fprintf(stderr, "Evaluation error: built-in function `not`: expected type %d (BOOL_TYPE) as first argument, but received %d\n", BOOL_TYPE, cond->type);
        texit(4);
    }
    result = makeBool(!(cond->i));
    return result;
}

Value *eval_if(Value *args, Frame *frame) {
    Value *cond, *result;
    int argc = length(args);
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Evaluation error: built-in function `if`: expected 2 or 3 arguments, received %d\n", argc);
        texit(4);
    }
    cond = eval(car(args), frame);
    if (cond->type != BOOL_TYPE) {
        fprintf(stderr, "Evaluation error: built-in function `if`: expected type %d (BOOL_TYPE) as first argument, but received %d\n", BOOL_TYPE, cond->type);
        texit(4);
    }
    if (cond->i) {
        result = eval(car(cdr(args)), frame);
    } else if (argc == 2) {
        result = makeVoid();
    } else {
        result = eval(car(cdr(cdr(args))), frame);
    }
    return result;
}

Value *eval_when(Value *args, Frame *frame) {
    Value *cond, *result = NULL;
    cond = eval(car(args), frame);
    if (cond->type != BOOL_TYPE) {
        fprintf(stderr, "Evaluation error: built-in function `when`: expected type %d (BOOL_TYPE) as first argument, but received %d\n", BOOL_TYPE, cond->type);
        texit(4);
    }
    if (cond->i) {
        result = eval_begin(cdr(args), frame);
    } else {
        result = makeVoid();
    }
    return result;
}

Value *eval_unless(Value *args, Frame *frame) {
    Value *cond, *current, *result = NULL;
    cond = eval(car(args), frame);
    if (cond->type != BOOL_TYPE) {
        fprintf(stderr, "Evaluation error: built-in function `unless`: expected type %d (BOOL_TYPE) as first argument, but received %d\n", BOOL_TYPE, cond->type);
        texit(4);
    }
    if (!(cond->i)) {
        result = eval_begin(cdr(args), frame);
    } else {
        result = makeVoid();
    }
    return result;
}

Value *let_helper(Value *args, Frame *frame, int star) {
    Value *current, *current_pair, *result, scratch_value, *binding;
    Frame new_frame;
    size_t parent_frame = !star;   // use parent frame for lookup of new bindings instead of current frame
    if (length(args) < 2) {
        goto LET_ERROR_BAD_FORM;
    }
    scratch_value.type = NULL_TYPE;
    new_frame.bindings = &scratch_value;    // use as NULL_TYPE to end list
    new_frame.parent = frame;
    current = car(args);    // list of (symbol value) pairs
    while (current->type == CONS_TYPE) {
        current_pair = car(current);
        if ((current_pair->type != CONS_TYPE)
                || (length(current_pair) != 2)
                || (car(current_pair)->type != SYMBOL_TYPE))
            goto LET_ERROR_BAD_FORM;
        binding = new_frame.bindings;
        while (binding->type == CONS_TYPE) {
            if (strcmp(car(car(binding))->s, car(current_pair)->s) == 0) {
                fprintf(stderr, "Evaluation error: let: duplicate bound variable %s in form ", car(current_pair)->s);
                goto LET_ERROR_DISPLAY_TREE;
            }
            binding = cdr(binding);
        }
        new_frame.bindings = cons(
                cons(
                    car(current_pair),
                    eval(
                        car(cdr(current_pair)),
                        (Frame *)((parent_frame * (size_t)frame) + (!parent_frame * ((size_t)&new_frame))))),
                        // branchlessly use either parent `frame` or current `&new_frame` => fast
                new_frame.bindings);
        current = cdr(current);
    }
    if (current->type != NULL_TYPE) {
        goto LET_ERROR_BAD_FORM;
    }
    current = cdr(args);    // current is now reused at a different level in the list
    while (current->type == CONS_TYPE) {
        result = eval(car(current), &new_frame);
        current = cdr(current);
    }
    return result;
LET_ERROR_BAD_FORM:
    fprintf(stderr, "Evaluation error: built-in function `let`: bad form in arguments: ");
LET_ERROR_DISPLAY_TREE:
    scratch_value.type = SYMBOL_TYPE;   // use as symbol for "let"
    scratch_value.s = "let";    // stack-allocated string is fine here
    display_to_fd(cons(&scratch_value, args), stderr);
    texit(4);
    return result;  // will never return
}

Value *eval_let(Value *args, Frame *frame) {
    return let_helper(args, frame, 0);
}

Value *eval_let_star(Value *args, Frame *frame) {
    return let_helper(args, frame, 1);
}

Value *eval_quote(Value *args, Frame *frame) {
    int argc = length(args);
    if (argc != 1) {
        fprintf(stderr, "Evaluation error: built-in function `quote`: expected 1 argument, received %d\n", argc);
        texit(4);
    }
    return car(args);
}

Value *eval_display(Value *args, Frame *frame) {
    Value *val, *result;
    int i, argc;
    char c;
    argc = length(args);
    if (argc != 1) {
        fprintf(stderr, "Evaluation error: built-in function `display`: expected 1 argument, received %d\n", argc);
        texit(4);
    }
    val = eval(car(args), frame);
    switch (val->type) {
        case INT_TYPE:
            printf("%d", val->i);
            break;
        case DOUBLE_TYPE:
            printf("%lf", val->d);
            break;
        case STR_TYPE:
            printf("%s", val->s);
            break;
        case BOOL_TYPE:
            if (val->i == 0)
                printf("#f");
            else
                printf("#t");
            break;
        case VOID_TYPE:
            printf("#<void>");
            break;
        default:
            fprintf(stderr, "Evaluation error: built-in function `display`: cannot display value of type %d\n", val->type);
            texit(4);
    }
    result = makeVoid();
    return result;
}

Value *eval(Value *expr, Frame *frame) {
    Value *first, *args, *result;
    switch (expr->type) {
        case INT_TYPE:
        case DOUBLE_TYPE:
        case STR_TYPE:
        case PTR_TYPE:
        case BOOL_TYPE:
            return expr;
        case CONS_TYPE:
            first = car(expr);
            args = cdr(expr);
            if (first->type != SYMBOL_TYPE) {
                fprintf(stderr, "Evaluation error: wrong type to apply: ");
                display_to_fd(first, stderr);
                texit(4);
            }
            if (strcmp(first->s, "begin") == 0) {
                result = eval_begin(args, frame);
            } else if (strcmp(first->s, "not") == 0) {
                result = eval_not(args, frame);
            } else if (strcmp(first->s, "if") == 0) {
                result = eval_if(args, frame);
            } else if (strcmp(first->s, "when") == 0) {
                result = eval_when(args, frame);
            } else if (strcmp(first->s, "unless") == 0) {
                result = eval_unless(args, frame);
            } else if (strcmp(first->s, "let") == 0) {
                result = eval_let(args, frame);
            } else if (strcmp(first->s, "let*") == 0) {
                result = eval_let_star(args, frame);
            } else if (strcmp(first->s, "quote") == 0) {
                result = eval_quote(args, frame);
            } else if (strcmp(first->s, "display") == 0) {
                result = eval_display(args, frame);
            } else {
                // Probably want a way of looking up symbols which are let or
                // defined to functions -- this likely comes later
                fprintf(stderr, "Evaluation error: unrecognized function: %s\n", first->s);
                texit(4);
            }
            break;
        case SYMBOL_TYPE:
            result = lookup_symbol(expr, frame);
            break;
        default:
            fprintf(stderr, "Evaluation error: unexpected value of type %d\n", expr->type);
            texit(4);
    }
    return result;
}

void interpret(Value *tree) {
    Value *current = tree;
    Frame frame;
    Value null_value;
    null_value.type = NULL_TYPE;
    frame.bindings = &null_value;
    frame.parent = NULL;
    while (current->type == CONS_TYPE) {
        display(eval(car(current), &frame));
        current = cdr(current);
    }
}

