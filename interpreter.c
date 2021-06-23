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


////////////////////////////////////////
////////// BUILT-IN FUNCTIONS //////////
////////////////////////////////////////

Value *eval_begin(Value *args, Frame *frame) {
    Value *current = args, *result = NULL;
    while (current->type == CONS_TYPE) {
        result = eval(car(current), frame);
        current = cdr(current);
    }
    if (current->type != NULL_TYPE) {
        fprintf(stderr, "Evaluation error: built-in function `begin`: bad form in arguments: ");
        error_display_tree("begin", args);
        texit(4);
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

Value *eval_cond(Value *args, Frame *frame) {
    Value *current = args, *cur_clause, *test;
    if (length(args) == 0)
        goto COND_ERROR_BAD_FORM;
    while (current->type == CONS_TYPE) {
        cur_clause = car(current);
        if (cur_clause->type != CONS_TYPE)
            goto COND_ERROR_BAD_FORM;
        test = car(cur_clause);
        if (test->type == SYMBOL_TYPE && strcmp(test->s, "else") == 0)
            return eval_begin(cdr(cur_clause), frame);
        test = eval(test, frame);
        if (test->type != BOOL_TYPE)
            goto COND_ERROR_BAD_FORM;
        if (test->i == 1)
            return eval_begin(cdr(cur_clause), frame);
        current = cdr(current);
    }
    if (current->type != NULL_TYPE)
        goto COND_ERROR_BAD_FORM;
    return makeVoid();
COND_ERROR_BAD_FORM:
    fprintf(stderr, "Evaluation error: built-in function `cond`: bad form in arguments: ");
    error_display_tree("cond", args);
    texit(4);
    return NULL;    // will never return
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

/* Sets the evaluated binding in the given frame */
void letrec_eval_bindings_helper(Value *list, Frame *frame, int evaluate, int star) {
    Value *current, *cur_pair, *cur_bind, *binding;
    Frame *cur_frame;
    current = list;
    while (current->type == CONS_TYPE) {
        cur_pair = car(current);
        cur_frame = frame;
        while (cur_frame != NULL) {
            binding = frame->bindings;
            while (binding->type == CONS_TYPE) {
                cur_bind = car(binding);
                if (strcmp(car(cur_bind)->s, car(cur_pair)->s) == 0) {
                    cur_bind->c.cdr = evaluate ? eval(car(cdr(cur_pair)), frame) : cdr(cur_pair);
                    goto FOUND_BINDING;
                }
                binding = cdr(binding);
            }
            cur_frame = star ? cur_frame->parent : NULL;
        }
        // Should be imposible to get here, since matching binding should always be found
        fprintf(stderr, "Evaluation error: built-in function `%s`: temporary binding for evaluated variable no longer found in frame: %s", star ? "letrec*" : "letrec", car(cur_pair)->s);
        texit(4);
FOUND_BINDING:
        current = cdr(current);
    }
}

/* Evaluate the bindings which had previously been set to UNSPECIFIED_TYPE by
 * going through the original list of (var expr) pairs, evaluating expr, and
 * setting var to the result in the current frame.  If star, then the setting
 * happens immediately after evaluation; else, the setting happens after all
 * expressions have been evaulated. */
void letrec_eval_bindings(Value *pairs, Frame *frame, int star) {
    Value *current, *cur_pair, *evaluated, *eval_list;
    if (pairs->type == NULL_TYPE)
        return;
    if (star) {
        letrec_eval_bindings_helper(pairs, frame, 1, star);
        return;
    }
    eval_list = makeNull();
    current = pairs;
    while (current->type == CONS_TYPE) {
        cur_pair = car(current);
        evaluated = cons(car(cur_pair), eval(car(cdr(cur_pair)), frame));
        if (cdr(evaluated)->type == UNSPECIFIED_TYPE) {
            fprintf(stderr, "Evaluation error: built-in function `%s`: unbound variable ", star ? "letrec*" : "letrec");
            display_to_fd(car(evaluated), stderr);
            texit(4);
        }
        eval_list = cons(evaluated, eval_list);
        current = cdr(current);
    }
    letrec_eval_bindings_helper(eval_list, frame, 0, star);
}

Value *let_helper(Value *args, Frame *frame, int star, int rec) {
    Value *current, *current_pair, *result, *binding;
    Frame *new_frame, *eval_frame;
    char *name_possibilities[4] = {"let", "letrec", "let*", "letrec*"};
    char *name = name_possibilities[(!star << 1) | (!rec)];
    if (length(args) < 2)
        goto LET_ERROR_BAD_FORM;
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
                    rec ? makeUnspecified() : eval(car(cdr(current_pair)), frame)),
                new_frame->bindings);
        if (star) {
            frame = new_frame;
            new_frame = talloc(sizeof(Frame));
            new_frame->bindings = makeNull();
            new_frame->parent = frame;
        }
        current = cdr(current);
    }
    if (current->type != NULL_TYPE)
        goto LET_ERROR_BAD_FORM;
    if (rec)
        letrec_eval_bindings(car(args), new_frame, star);
    current = cdr(args);    // current is now reused to evaluate expressions
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
    return let_helper(args, frame, 0, 0);
}

Value *eval_let_star(Value *args, Frame *frame) {
    return let_helper(args, frame, 1, 0);
}

Value *eval_letrec(Value *args, Frame *frame) {
    return let_helper(args, frame, 0, 1);
}

Value *eval_letrec_star(Value *args, Frame *frame) {
    return let_helper(args, frame, 1, 1);
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
    Value *closure, *current, *next;
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

Value *eval_set(Value *args, Frame *frame) {
    Value *binding, *pair, *expr;
    Frame *current = frame;
    int argc = length(args);
    if (argc != 2) {
        fprintf(stderr, "Evaluation error: built-in function `set!`: expected 2 arguments, received %d\n", argc);
        texit(4);
    }
    expr = car(args);
    if (expr->type != SYMBOL_TYPE) {
        fprintf(stderr, "Evaluation error: built-in function `set!`: wrong type argument in position 1 (expected SYMBOL_TYPE): ");
        display_to_fd(expr, stderr);
        texit(4);
    }
    while (current != NULL) {
        binding = current->bindings;
        while (binding->type == CONS_TYPE) {
            pair = car(binding);
            if (strcmp(car(pair)->s, expr->s) == 0) {
                pair->c.cdr = eval(car(cdr(args)), frame);
                return makeVoid();
            }
            binding = cdr(binding);
        }
        current = current->parent;
    }
    fprintf(stderr, "Evaluation error: built-in function `set!`: unbound variable ");
    display_to_fd(expr, stderr);
    texit(4);
    return NULL;
}

Value *logic_helper(Value *args, Frame *frame, int end_val) {
    Value *cond, *current = args;
    int arg_num = 1;
    while (current->type == CONS_TYPE) {
        cond = eval(car(current), frame);
        if (cond->type != BOOL_TYPE) {
            fprintf(stderr, "Evaluation error: built-in function `and`: wrong type argument in position %d: ", arg_num);
            display_to_fd(cond, stderr);
            texit(4);
        }
        if (cond->i == end_val) {
            return cond;
        }
        arg_num++;
        current = cdr(current);
    }
    return makeBool(!end_val);
}

Value *eval_and(Value *args, Frame *frame) {
    return logic_helper(args, frame, 0);
}

Value *eval_or(Value *args, Frame *frame) {
    return logic_helper(args, frame, 1);
}


////////////////////////////////////////
///////// PRIMITIVE FUNCTIONS //////////
////////////////////////////////////////

enum operation {
    PLUS,
    MINUS,
    MULT,
};

Value *arith_helper(Value *result, Value *args, enum operation op) {
    Value *cur_val, *current = args;
    char names[3] = {'+', '-', '/'};
    char name = names[op];
    int arg_num = 1;
    while (current->type == CONS_TYPE) {
        cur_val = car(current);
        switch (cur_val->type) {
            case INT_TYPE:
                if (result->type == INT_TYPE) {
                    switch (op) {
                        case PLUS:
                            result->i += cur_val->i;
                            break;
                        case MINUS:
                            result->i -= cur_val->i;
                            break;
                        case MULT:
                            result->i *= cur_val->i;
                            break;
                    }
                } else if (result->type == DOUBLE_TYPE) {
                    switch (op) {
                        case PLUS:
                            result->d += (double)cur_val->i;
                            break;
                        case MINUS:
                            result->d -= (double)cur_val->i;
                            break;
                        case MULT:
                            result->d *= (double)cur_val->i;
                            break;
                    }
                }
                break;
            case DOUBLE_TYPE:
                if (result->type == INT_TYPE)
                    result->d = (double)result->i;
                result->type = DOUBLE_TYPE;
                switch (op) {
                    case PLUS:
                        result->d += cur_val->d;
                        break;
                    case MINUS:
                        result->d -= cur_val->d;
                        break;
                    case MULT:
                        result->d *= cur_val->d;
                        break;
                }
                break;
            default:
                fprintf(stderr, "Evaluation error: primitive function `%c`: wrong type argument in position %d: ", name, arg_num);
                display_to_fd(cur_val, stderr);
                texit(4);
        }
        current = cdr(current);
        arg_num++;
    }
    return result;
}

Value *prim_add(Value *args) {
    Value *result;
    result = talloc(sizeof(Value));
    result->type = INT_TYPE;
    result->i = 0;
    return arith_helper(result, args, PLUS);
}

Value *prim_sub(Value *args) {
    Value *result;
    int argc = length(args);
    result = talloc(sizeof(Value));
    result->type = INT_TYPE;
    result->i = 0;
    switch (argc) {
        case 0:
            fprintf(stderr, "Evaluation error: primitive function `-`: wrong number of arguments\n");
            texit(4);
        case 1:
            return arith_helper(result, args, MINUS);
        default:
            break;
    }
    *result = *car(args);
    return arith_helper(result, cdr(args), MINUS);
}

Value *prim_mul(Value *args) {
    Value *result = talloc(sizeof(Value));
    result->type = INT_TYPE;
    result->i = 1;
    return arith_helper(result, args, MULT);
}

Value *prim_div(Value *args) {
    Value *result, *divisor;
    int argc = length(args);
    if (argc != 2) {
        fprintf(stderr, "Evaluation error: primitive function `/`: wrong number of arguments\n");
        texit(4);
    }
    result = talloc(sizeof(Value));
    *result = *car(args);
    divisor = car(cdr(args));
    if (result->type != INT_TYPE && result->type != DOUBLE_TYPE) {
        fprintf(stderr, "Evaluation error: primitive function `/`: wrong type argument in position 1: ");
        display_to_fd(result, stderr);
        texit(4);
    }
    switch (divisor->type) {
        case INT_TYPE:
            if (result->type == INT_TYPE) {
                if (result->i % divisor->i == 0) {
                    result->i /= divisor->i;
                    return result;
                }
                result->type = DOUBLE_TYPE;
                result->d = (double)result->i;
            }
            divisor->d = (double)divisor->i;
        case DOUBLE_TYPE:
            if (result->type == INT_TYPE)
                result->d = (double)result->i;
            result->type = DOUBLE_TYPE;
            result->d /= divisor->d;
            break;
        default:
            fprintf(stderr, "Evaluation error: primitive function `/`: wrong type argument in position 2: ");
            display_to_fd(divisor, stderr);
            texit(4);
    }
    return result;
}

Value *prim_mod(Value *args) {
    Value *result, *first, *second;
    if (length(args) != 2) {
        fprintf(stderr, "Evaluation error: primitive function `modulo`: wrong number of arguments\n");
        texit(4);
    }
    first = car(args);
    second = car(cdr(args));
    if (first->type != INT_TYPE) {
        fprintf(stderr, "Evaluation error: primitive function `modulo`: wrong type argument in position 1: ");
        display_to_fd(first, stderr);
        texit(4);
    }
    if (second->type != INT_TYPE) {
        fprintf(stderr, "Evaluation error: primitive function `modulo`: wrong type argument in position 2: ");
        display_to_fd(second, stderr);
        texit(4);
    }
    result = talloc(sizeof(Value));
    result->type = INT_TYPE;
    result->i = first->i % second->i;
    return result;
}

enum comparison {
    EQ,
    GT,
    LT,
    GEQ,
    LEQ,
};

Value *compare_helper(Value *args, enum comparison comp) {
    Value *prev, *current, *cur_val;
    int arg_num = 2;
    if (length(args) <= 1)
        return makeBool(1);
    prev = car(args);   // should already be evaluated
    current = cdr(args);
    while (current->type == CONS_TYPE) {
        cur_val = car(current);
        switch (prev->type) {
            case INT_TYPE:
                switch (cur_val->type) {
                    case INT_TYPE:
                        switch (comp) {
                            case EQ:
                                if (prev->i != cur_val->i)
                                    return makeBool(0);
                                break;
                            case GT:
                                if (prev->i <= cur_val->i)
                                    return makeBool(0);
                                break;
                            case LT:
                                if (prev->i >= cur_val->i)
                                    return makeBool(0);
                                break;
                            case GEQ:
                                if (prev->i < cur_val->i)
                                    return makeBool(0);
                                break;
                            case LEQ:
                                if (prev->i > cur_val->i)
                                    return makeBool(0);
                                break;
                        }
                        break;
                    case DOUBLE_TYPE:
                        switch (comp) {
                            case EQ:
                                if (prev->d != cur_val->i)
                                    return makeBool(0);
                                break;
                            case GT:
                                if (prev->d <= cur_val->i)
                                    return makeBool(0);
                                break;
                            case LT:
                                if (prev->d >= cur_val->i)
                                    return makeBool(0);
                                break;
                            case GEQ:
                                if (prev->d < cur_val->i)
                                    return makeBool(0);
                                break;
                            case LEQ:
                                if (prev->d > cur_val->i)
                                    return makeBool(0);
                                break;
                        }
                        break;
                    default:
                        fprintf(stderr, "Evaluation error: primitive function `=`: wrong type argument in position %d: ", arg_num);
                        display_to_fd(prev, stderr);
                        texit(4);
                }
                break;
            case DOUBLE_TYPE:
                switch (cur_val->type) {
                    case INT_TYPE:
                        switch (comp) {
                            case EQ:
                                if (prev->i != cur_val->d)
                                    return makeBool(0);
                                break;
                            case GT:
                                if (prev->i <= cur_val->d)
                                    return makeBool(0);
                                break;
                            case LT:
                                if (prev->i >= cur_val->d)
                                    return makeBool(0);
                                break;
                            case GEQ:
                                if (prev->i < cur_val->d)
                                    return makeBool(0);
                                break;
                            case LEQ:
                                if (prev->i > cur_val->d)
                                    return makeBool(0);
                                break;
                        }
                        break;
                    case DOUBLE_TYPE:
                        switch (comp) {
                            case EQ:
                                if (prev->d != cur_val->d)
                                    return makeBool(0);
                                break;
                            case GT:
                                if (prev->d <= cur_val->d)
                                    return makeBool(0);
                                break;
                            case LT:
                                if (prev->d >= cur_val->d)
                                    return makeBool(0);
                                break;
                            case GEQ:
                                if (prev->d < cur_val->d)
                                    return makeBool(0);
                                break;
                            case LEQ:
                                if (prev->d > cur_val->d)
                                    return makeBool(0);
                                break;
                        }
                        break;
                    default:
                        fprintf(stderr, "Evaluation error: primitive function `=`: wrong type argument in position %d: ", arg_num);
                        display_to_fd(prev, stderr);
                        texit(4);
                }
                break;
            default:
                fprintf(stderr, "Evaluation error: primitive function `=`: wrong type argument in position 1: ");
                display_to_fd(prev, stderr);
                texit(4);
        }
        arg_num++;
        prev = current;
        current = cdr(current);
    }
    return makeBool(1);
}

Value *prim_eqnum(Value *args) {
    return compare_helper(args, EQ);
}

Value *prim_gt(Value *args) {
    return compare_helper(args, GT);
}

Value *prim_lt(Value *args) {
    return compare_helper(args, LT);
}

Value *prim_geq(Value *args) {
    return compare_helper(args, GEQ);
}

Value *prim_leq(Value *args) {
    return compare_helper(args, LEQ);
}

Value *prim_null(Value *args) {
    int argc = length(args);
    if (argc != 1) {
        fprintf(stderr, "Evaluation error: primitive function `null?`: expected 1 argument, received %d\n", argc);
        texit(4);
    }
    return makeBool(car(args)->type == NULL_TYPE);
}

Value *prim_car(Value *args) {
    Value *value;
    int argc = length(args);
    if (argc != 1) {
        fprintf(stderr, "Evaluation error: primitive function `car`: expected 1 argument, received %d\n", argc);
        texit(4);
    }
    value = car(args);
    if (value->type != CONS_TYPE) {
        fprintf(stderr, "Evaluation error: primitive function `car`: wrong type argument in position 1 (expected CONS_TYPE): ");
        display_to_fd(value, stderr);
        texit(4);
    }
    return car(value);
}

Value *prim_cdr(Value *args) {
    Value *value;
    int argc = length(args);
    if (argc != 1) {
        fprintf(stderr, "Evaluation error: primitive function `cdr`: expected 1 argument, received %d\n", argc);
        texit(4);
    }
    value = car(args);
    if (value->type != CONS_TYPE) {
        fprintf(stderr, "Evaluation error: primitive function `cdr`: wrong type argument in position 1 (expected CONS_TYPE): ");
        display_to_fd(value, stderr);
        texit(4);
    }
    return cdr(value);
}

Value *prim_cons(Value *args) {
    int argc = length(args);
    if (argc != 2) {
        fprintf(stderr, "Evaluation error: primitive function `cons`: expected 2 arguments, received %d\n", argc);
        texit(4);
    }
    return cons(car(args), car(cdr(args)));
}

Value *prim_list(Value *args) {
    return args;
}

Value *prim_append(Value *args) {
    Value head, *tail, *current, *current_list = NULL;
    int arg_num = 1;
    head.c.cdr = NULL;
    tail = &head;
    current = args;
    while (current->type == CONS_TYPE) {
        current_list = car(current);
        while (current_list->type == CONS_TYPE) {
            tail->c.cdr = cons(car(current_list), NULL);
            tail = tail->c.cdr;
            current_list = cdr(current_list);
        }
        if (current_list->type != NULL_TYPE && cdr(current)->type != NULL_TYPE) {
            fprintf(stderr, "Evaluation error: primitive function `append`: wrong type argument in position %d: ", arg_num);
            display_to_fd(current_list, stderr);
            texit(4);
        }
        current = cdr(current);
        arg_num++;
    }
    tail->c.cdr = current_list ? current_list : makeNull();
    return  head.c.cdr;
}

int equal_helper(Value *first, Value *second) {
    int equal = -1;
    if (first->type != second->type)
        return 0;
    switch (first->type) {
        case INT_TYPE:
        case BOOL_TYPE:
            return (first->i == second->i);
        case DOUBLE_TYPE:
            return (first->d == second->d);
        case STR_TYPE:
        case SYMBOL_TYPE:
            return !strcmp(first->s, second->s);
        case CONS_TYPE:
            equal = 1;
            while (first->type == CONS_TYPE && second->type == CONS_TYPE) {
                equal &= equal_helper(car(first), car(second));
                if (!equal)
                    return equal;
                first = cdr(first);
                second = cdr(second);
            }
            return equal_helper(first, second); // first/second should usually be NULL_TYPE
        case NULL_TYPE:
            return 1;
        case CLOSURE_TYPE:
            equal = 1;
            equal &= equal_helper(first->cl.paramNames, second->cl.paramNames);
            equal &= equal_helper(first->cl.functionCode, second->cl.functionCode);
            equal &= (first->cl.frame == second->cl.frame);
            return equal;
        case PRIMITIVE_TYPE:
            return (first->pf == second->pf);
        default:
            fprintf(stderr, "Evaluation error: primitive function `equal?`: unexpected value of type %d\n", first->type);
            texit(4);
    }
    return equal;
}

Value *prim_equal(Value *args) {
    int argc = length(args);
    if (argc != 2) {
        fprintf(stderr, "Evaluation error: built-in function `equal?`: expected 2 arguments, received %d\n", argc);
        texit(4);
    }
    return makeBool(equal_helper(car(args), car(cdr(args))));
}


////////////////////////////////////////
///////// EVALUATION FUNCTIONS /////////
////////////////////////////////////////

Value *apply(Value *function, Value *args) {
    Value *result, *curr_param, *curr_arg;
    Frame *new_frame;
    if (function->type == PRIMITIVE_TYPE) {
        return function->pf(args);
    } else if (function->type != CLOSURE_TYPE) {
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
            new_frame->bindings = cons(cons(car(curr_param), car(curr_arg)), new_frame->bindings);
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

void bind_primitive(char *name, Value *(*function)(Value *), Frame *frame){
    Value *name_val, *func_val;
    name_val = talloc(sizeof(Value));
    name_val->type = SYMBOL_TYPE;
    name_val->s = talloc(sizeof(name));
    strcpy(name_val->s, name);
    func_val = talloc(sizeof(Value));
    func_val->type = PRIMITIVE_TYPE;
    func_val->pf = function;
    frame->bindings = cons(cons(name_val, func_val), frame->bindings);
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
                    if (strcmp(first->s, "and") == 0)
                        return eval_and(args, frame);
                    break;
                case 'b':
                    if (strcmp(first->s, "begin") == 0)
                        return eval_begin(args, frame);
                    break;
                case 'c':
                    if (strcmp(first->s, "cond") == 0)
                        return eval_cond(args, frame);
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
                    else if (strcmp(first->s, "letrec") == 0)
                        return eval_letrec(args, frame);
                    else if (strcmp(first->s, "letrec*") == 0)
                        return eval_letrec_star(args, frame);
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
                    if (strcmp(first->s, "or") == 0)
                        return eval_or(args, frame);
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
                    if (strcmp(first->s, "set!") == 0)
                        return eval_set(args, frame);
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
        case PRIMITIVE_TYPE:
        case UNSPECIFIED_TYPE:
            return expr;
        default:
            fprintf(stderr, "Evaluation error: unexpected value of type %d\n", expr->type);
            texit(4);
    }
    return result;  // should always return before this
}

void interpret(Value *tree) {
    Value *result, *current = tree;
    Frame frame;
    Value null_value;
    null_value.type = NULL_TYPE;
    frame.bindings = &null_value;
    frame.parent = NULL;
    bind_primitive("car", prim_car, &frame);
    bind_primitive("cdr", prim_cdr, &frame);
    bind_primitive("cons", prim_cons, &frame);
    bind_primitive("+", prim_add, &frame);
    bind_primitive("-", prim_sub, &frame);
    bind_primitive("*", prim_mul, &frame);
    bind_primitive("/", prim_div, &frame);
    bind_primitive("modulo", prim_mod, &frame);
    bind_primitive("=", prim_eqnum, &frame);
    bind_primitive(">", prim_gt, &frame);
    bind_primitive("<", prim_lt, &frame);
    bind_primitive(">=", prim_geq, &frame);
    bind_primitive("<=", prim_leq, &frame);
    bind_primitive("null?", prim_null, &frame);
    bind_primitive("list", prim_list, &frame);
    bind_primitive("append", prim_append, &frame);
    bind_primitive("equal?", prim_equal, &frame);
    while (current->type == CONS_TYPE) {
        result = eval(car(current), &frame);
        if (result->type != VOID_TYPE)
            display(result);
        current = cdr(current);
    }
}

