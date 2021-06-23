#include <stdio.h>
#include <string.h>
#include "value.h"
#include "talloc.h"
#include "linkedlist.h"


/* Attempts to look up the symbol associated with the given Value* in the given
 * frame and its parents.  If the symbol is not found, returns NULL, otherwise
 * returns the associated value (without calling eval on it). */
Value *lookup_symbol(Value *expr, Frame *frame) {
    Value curr_frame, *value, *pair;
    Frame *current = frame;
    if (expr->type != SYMBOL_TYPE) {
        fprintf(stderr, "Evaluation error: called lookup_symbol on value of type %d\n", expr->type);
        texit(4);
    }
    while (current != NULL) {
        value = current->bindings;
        while (value->type == CONS_TYPE) {
            pair = car(value);
            if (strcmp(car(pair)->s, expr->s) == 0) {
                return cdr(pair);
            }
            value = cdr(value);
        }
        current = current->parent;
    }
    // if while loops end, never found matching symbol in frames
    return NULL;
}

void error_display_tree(char *name, Value *args) {
    Value tmp_cons, tmp_symbol;
    tmp_symbol.type = SYMBOL_TYPE;
    tmp_symbol.s = name;
    tmp_cons.type = CONS_TYPE;
    tmp_cons.c.car = &tmp_symbol;
    tmp_cons.c.cdr = args;
    display_to_fd(&tmp_cons, stderr);
    return;
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
    Value *current, *current_pair, *result, *binding;
    Frame *new_frame;
    size_t parent_frame = !star;   // use parent frame for lookup of new bindings instead of current frame
    char *name = (char *)((parent_frame * (size_t)"let") + (!parent_frame * (size_t)"let*"));
    if (length(args) < 2) {
        goto LET_ERROR_BAD_FORM;
    }
    new_frame = talloc(sizeof(Frame));
    new_frame->bindings = makeNull();
    new_frame->parent = frame;
    current = car(args);    // list of (symbol value) pairs
    while (current->type == CONS_TYPE) {
        current_pair = car(current);
        if ((current_pair->type != CONS_TYPE)
                || (length(current_pair) != 2)
                || (car(current_pair)->type != SYMBOL_TYPE))
            goto LET_ERROR_BAD_FORM;
        binding = new_frame->bindings;
        while (binding->type == CONS_TYPE) {
            if (strcmp(car(car(binding))->s, car(current_pair)->s) == 0) {
                fprintf(stderr, "Evaluation error: built-in function `%s`: duplicate bound variable %s in form ", name, car(current_pair)->s);
                goto LET_ERROR_DISPLAY_TREE;
            }
            binding = cdr(binding);
        }
        new_frame->bindings = cons(
                cons(
                    car(current_pair),
                    eval(
                        car(cdr(current_pair)),
                        (Frame *)((parent_frame * (size_t)frame) + (!parent_frame * ((size_t)new_frame))))),
                        // branchlessly use either parent `frame` or current `new_frame` => fast
                new_frame->bindings);
        current = cdr(current);
    }
    if (current->type != NULL_TYPE) {
        goto LET_ERROR_BAD_FORM;
    }
    current = cdr(args);    // current is now reused at a different level in the list
    while (current->type == CONS_TYPE) {
        result = eval(car(current), new_frame);
        current = cdr(current);
    }
    return result;
LET_ERROR_BAD_FORM:
    fprintf(stderr, "Evaluation error: built-in function `%s`: bad form in arguments: ", name);
LET_ERROR_DISPLAY_TREE:
    error_display_tree(name, args);
    texit(4);
    return NULL;    // will never return
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
            break;
        case CLOSURE_TYPE:
            printf("#<procedure>");
        default:
            fprintf(stderr, "Evaluation error: built-in function `display`: cannot display value of type %d\n", val->type);
            texit(4);
    }
    result = makeVoid();
    return result;
}

Value *eval_lambda(Value *args, Frame *frame) {
    Value *closure, *current, *next;;
    if (length(args) < 2) {
        fprintf(stderr, "Evaluation error: built-in function `lambda`: bad form in arguments: ");
        error_display_tree("lambda", args);
        texit(4);
    }
    current = car(args);
    closure = talloc(sizeof(Value));
    closure->type = CLOSURE_TYPE;
    closure->cl.paramNames = current;
    closure->cl.functionCode = cdr(args);
    closure->cl.frame = frame;
    if (current->type != SYMBOL_TYPE) {
        while (current->type == CONS_TYPE) {
            if (car(current)->type != SYMBOL_TYPE)
                goto LAMBDA_BAD_PARAMETERS;
            next = cdr(current);
            while (next->type == CONS_TYPE) {
                if (strcmp(car(current)->s, car(next)->s) == 0)
                    goto LAMBDA_BAD_PARAMETERS;
                next = cdr(next);
            }
            current = cdr(current);
        }
        if (current->type != NULL_TYPE)
            goto LAMBDA_BAD_PARAMETERS;
    }
    return closure;
LAMBDA_BAD_PARAMETERS:
    fprintf(stderr, "Evaluation error: built-in function `lambda`: bad form in parameters list: ");
    error_display_tree("lambda", args);
    texit(4);
    return NULL;    // will never return
}

Value *eval_define(Value *args, Frame *frame) {
    Value *var, *expr;
    if (length(args) != 2) {
        fprintf(stderr, "Evaluation error: built-in function `define`: bad form in arguments: ");
        error_display_tree("define", args);
        texit(4);
    }
    var = car(args);
    expr = car(cdr(args));
    if (var->type == CONS_TYPE) {
        expr = eval_lambda(cons(cdr(var), expr), frame);
        var = car(var);
    } else if (var->type != SYMBOL_TYPE) {
        fprintf(stderr, "Evaluation error: built-in function `define`: bad form in arguments: ");
        error_display_tree("define", args);
        texit(4);
    }
    frame->bindings = cons(cons(var, eval(expr, frame)), frame->bindings);
    return makeVoid();
}

Value *apply(Value *function, Value *args) {
    Value *result, *curr_param, *curr_arg;
    Frame *new_frame;
    if (function->type != CLOSURE_TYPE) {
        fprintf(stderr, "Evaluation error: wrong type to apply: expected type %d (CLOSURE_TYPE), received type %d\n", CLOSURE_TYPE, function->type);
        texit(4);
    }
    new_frame = talloc(sizeof(Frame));
    new_frame->bindings = makeNull();
    new_frame->parent = function->cl.frame;
    curr_param = function->cl.paramNames;
    curr_arg = args;
    if (curr_param->type == SYMBOL_TYPE) {
        new_frame->bindings = cons(cons(curr_param, curr_arg), new_frame->bindings);
    } else {
        while (curr_param->type == CONS_TYPE) {
            if (curr_arg->type != CONS_TYPE) {
                goto APPLY_WRONG_NUMBER_ARGS;
            }
            // lambda assures that parameters list is well-formed
            new_frame->bindings = cons(
                    cons(
                        car(curr_param),
                        car(curr_arg)),
                    new_frame->bindings);
            curr_param = cdr(curr_param);
            curr_arg = cdr(curr_arg);
        }
        if (curr_arg->type != NULL_TYPE)
            goto APPLY_WRONG_NUMBER_ARGS;
    }
    curr_arg = function->cl.functionCode;  // reuse curr_arg, now for body code
    while (curr_arg->type == CONS_TYPE) {
        result = eval(car(curr_arg), new_frame);
        curr_arg = cdr(curr_arg);
    }
    // lambda assures that body code is a list with at least one element
    return result;
APPLY_WRONG_NUMBER_ARGS:
    fprintf(stderr, "Evaluation error: possibly wrong number of arguments to apply\n");
    fprintf(stderr, "Expected: ");
    display_to_fd(function->cl.paramNames, stderr);
    fprintf(stderr, "Received: ");
    display_to_fd(args, stderr);
    texit(4);
    return NULL;    // will never return
}

Value *eval_all(Value *exprs, Frame *frame) {
    Value *current, *tail, head;
    switch (exprs->type) {
        case CONS_TYPE:
            break;
        case NULL_TYPE:
            return exprs;
        default:
            fprintf(stderr, "Evaluation error: expected CONS_TYPE or NULL_TYPE in eval_all, received type %d\n", exprs->type);
            texit(4);
    }
    head.c.cdr = NULL;
    tail = &head;
    current = exprs;
    while (current->type != NULL_TYPE) {
        tail->c.cdr = cons(eval(car(current), frame), NULL);
        tail = tail->c.cdr;
        current = cdr(current);
    }
    tail->c.cdr = current;
    return head.c.cdr;
}

Value *eval(Value *expr, Frame *frame) {
    Value *first, *args, *result = NULL;
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
            switch (first->type) {
                case CONS_TYPE:
                    return apply(eval(first, frame), eval_all(args, frame));
                case SYMBOL_TYPE:
                    result = lookup_symbol(first, frame);
                    if (result != NULL)
                        return apply(eval(result, frame), eval_all(args, frame));
                    break;
                default:
                    // should be CLOSURE_TYPE; if not, apply will catch it
                    return apply(eval(first, frame), eval_all(args, frame));
            }
            // Here, first was SYMBOL_TYPE and was not found by lookup_symbol
            switch (first->s[0]) {
                case 'a':
                    break;
                case 'b':
                    if (strcmp(first->s, "begin") == 0)
                        return eval_begin(args, frame);
                    break;
                case 'c':
                    break;
                case 'd':
                    if (strcmp(first->s, "display") == 0)
                        return eval_display(args, frame);
                    else if (strcmp(first->s, "define") == 0)
                        return eval_define(args, frame);
                    break;
                case 'e':
                    break;
                case 'f':
                    break;
                case 'g':
                    break;
                case 'h':
                    break;
                case 'i':
                    if (strcmp(first->s, "if") == 0)
                        return eval_if(args, frame);
                    break;
                case 'j':
                    break;
                case 'k':
                    break;
                case 'l':
                    if (strcmp(first->s, "let") == 0)
                        return eval_let(args, frame);
                    else if (strcmp(first->s, "let*") == 0)
                        return eval_let_star(args, frame);
                    else if (strcmp(first->s, "lambda") == 0)
                        return eval_lambda(args, frame);
                    break;
                case 'm':
                    break;
                case 'n':
                    if (strcmp(first->s, "not") == 0)
                        return eval_not(args, frame);
                    break;
                case 'o':
                    break;
                case 'p':
                    break;
                case 'q':
                    if (strcmp(first->s, "quote") == 0)
                        return eval_quote(args, frame);
                    break;
                case 'r':
                    break;
                case 's':
                    break;
                case 't':
                    break;
                case 'u':
                    if (strcmp(first->s, "unless") == 0)
                        return eval_unless(args, frame);
                    break;
                case 'v':
                    break;
                case 'w':
                    if (strcmp(first->s, "when") == 0)
                        return eval_when(args, frame);
                    break;
                case 'x':
                    break;
                case 'y':
                    break;
                case 'z':
                    break;
                default:
                    break;
            }
            fprintf(stderr, "Evaluation error: unrecognized function: %s\n", first->s);
            texit(4);
            break;
        case SYMBOL_TYPE:
            result = lookup_symbol(expr, frame);
            if (result == NULL) {
                fprintf(stderr, "Evaluation error: unknown symbol: %s\n", expr->s);
                texit(4);
            }
            return result;
        case CLOSURE_TYPE:
            return expr;
        default:
            fprintf(stderr, "Evaluation error: unexpected value of type %d\n", expr->type);
            texit(4);
    }
    return result;  // should always return before this
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

