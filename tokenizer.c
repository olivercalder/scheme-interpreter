/* tokenizer.c
 *
 * Oliver Calder
 * May 2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "value.h"
#include "talloc.h"
#include "linkedlist.h"

#define BUFSIZE 512

/* Calls fgetc on stdin and returns the resulting char. */
char get() {
    return (char)fgetc(stdin);
}

/* Calls ungetc on stdin and returns the char which was passed, or EOF if error. */
char unget(char c) {
    return ungetc(c, stdin);
}

/* Returns 1 if the given char is a delimiter (whitespace, newline, semicolon,
 * open or close parentheses or brackets, double quote, number sign, or
 * carriage return).  Otherwise returns 0.  Throws a warning if carriage
 * return, since these are not treated as newline characters. */
int is_delimiter(char c) {
    switch (c) {
        case ' ':
        case '\t':
        case '\n':
        case ';':
        case '(':
        case ')':
        case '[':
        case ']':
        case '"':
        case '#':
            return 1;
        case '\r':
            fprintf(stderr, "WARNING: carriage return character detected. Treating as whitespace, not newline.\n");
            return 1;
    }
    return 0;
}

/* Returns 1 if the given char is a base 10 digit, otherwise returns 0. */
int is_digit(char c) {
    return (int)(c >= '0' && c <= '9');
}

/* Returns 1 if the string stored in the buffer is a valid integer, otherwise
 * returns 0.  That is, if it begins with an optional + or - and contains only
 * digits subsequently.  Does not check whether the corresponding integer
 * overflows what is storable as an integer. */
int is_integer(const char *buf) {
    // Allow decimal, not hex
    int i = 1;
    char c = buf[0];
    if (!(is_digit(c) || c == '+' || c == '-'))
        return 0;
    c = buf[i];
    while (c != '\0') {
        if (!is_digit(c))
            return 0;
        i++;
        c = buf[i];
    }
    return 1;
}

/* Returns 1 if the string stored in the buffer is a valid double, otherwise
 * returns 0.  That is, if it begins with an optional + or - and contains only
 * digits and at most one decimal subsequently.  Does not check whether the
 * corresponding double overflows what is storable as an double. */
int is_double(const char *buf) {
    int had_decimal = 0, i = 1;
    char c = buf[0];
    if (c == '.')
        had_decimal = 1;
    else if (!(is_digit(c) || c == '+' || c == '-'))
        return 0;
    c = buf[i];
    while (c != '\0') {
        if (c == '.') {
            if (had_decimal)
                return 0;
            had_decimal = 1;
        } else if (!is_digit(c))
            return 0;
        i++;
        c = buf[i];
    }
    return 1;
}

/* Checks whether the given char is valid in a symbol.  Some chars are only
 * valid if they are not the initial char in the symbol, so the variable
 * initial indicates whether the given char occurred in the initial position
 * of the symbol in question.  Returns 1 if valid, otherwise 0. */
int char_valid_in_symbol(char c, int initial) {
    // valid are letters, numbers, and symbols:
    // ?, !, ., +, -, *, /, <, =, >, :, $, %, ^, &, _, ~, and @
    // Cannot start with + or - or . unless the complete token, respectively,
    // is + or - or ...
    // Cannot start with @ ever (guile and other interpreters ignore this rule)
    //
    // <symbol>     --> <initial> <subsequent>*
    // <initial>    --> <letter> | ! | $ | % | & | * | / | : | < | = | > | ? | ~ | _ | ^
	//              |   <Unicode Lu, Ll, Lt, Lm, Lo, Mn, Nl, No, Pd, Pc, Po, Sc, Sm, Sk, So, or Co>
	//              |   \x <hex scalar value> ;
    // <subsequent>	--> <initial> | <digit 10> | . | + | - | @ | <Unicode Nd, Mc, or Me>
    // <letter>     --> a | b | ... | z | A | B | ... | Z
    char next1, next2, next3;
    int valid;
    if (c >= 'a' && c <= 'z')
        return 1;
    if (c >= 'A' && c <= 'Z')
        return 1;
    if (c >= '0' && c <= '9')
        return !initial;
    switch (c) {
        case '!':
        case '$':
        case '%':
        case '&':
        case '*':
        case '/':
        case ':':
        case '<':
        case '=':
        case '>':
        case '?':
        case '^':
        case '_':
        case '~':
            return 1;
        case '.':   // allow ... token
            // already handled . and ... elsewhere
            if (!initial)
                return 1;
            // should never call this function with initial .
            valid = 0;
            next1 = get();
            if (next1 == '.') {
                next2 = get();
                if (next2 == '.') {
                    next3 = get();
                    if (is_delimiter(next3)) {
                        valid = 1;
                    }
                    unget(next3);
                }
                unget(next2);
            }
            unget(next1);
            return valid;
        case '+':   // allow + token
        case '-':   // allow - token
            if (!initial)
                return 1;
            valid = 0;
            next1 = get();
            if (is_delimiter(next1))
                valid = 1;
            unget(next1);
            return valid;
        case '@':
            // guile ignores this rule
            return !initial;
    }
    return 0;
}

/* Returns 1 if the string stored in the buffer is a valid symbol, otherwise
 * returns 0. */
int is_symbol(const char *buf) {
    int i = 0;
    while (buf[i] != '\0') {
        if (!char_valid_in_symbol(buf[i], (i == 0) ? 1 : 0))
            return 0;
        i++;
    }
    return 1;
}

/* Returns a Value* of type INT_TYPE which holds the integer representation of
 * the string in the given buffer.  Assumes the string is a valid integer. */
Value *make_integer(const char *buf) {
    Value *val = talloc(sizeof(Value));
    val->type = INT_TYPE;
    val->i = strtol(buf, NULL, 0);  // base 0 in case we support other bases later
    return val;
}

/* Returns a Value* of type DOUBLE_TYPE which holds the integer representation
 * of the string in the given buffer.  Assumes the string is a valid double. */
Value *make_double(const char *buf) {
    Value *val = talloc(sizeof(Value));
    val->type = DOUBLE_TYPE;
    val->d = strtod(buf, NULL);
    return val;
}

/* Returns a Value* of type DOUBLE_TYPE which holds the given boolean value. */
Value *make_bool(int boolean) {
    Value *val = talloc(sizeof(Value));
    val->type = BOOL_TYPE;
    val->i = boolean;
    return val;
}

/* Returns a Value* of type STR_TYPE which holds a copy of the string in the
 * given buffer. */
Value *make_string(const char *buf) {
    Value *val = talloc(sizeof(Value));
    val->type = STR_TYPE;
    val->s = talloc(sizeof(char) * strlen(buf) + 1);
    strcpy(val->s, buf);
    return val;
}

/* Returns a Value* of type SYMBOL_TYPE which holds a copy of the string in the
 * given buffer.  Assumes that the string is a valid symbol. */
Value *make_symbol(const char *buf) {
    Value *val = talloc(sizeof(Value));
    val->type = SYMBOL_TYPE;
    val->s = talloc(sizeof(char) * strlen(buf) + 1);
    strcpy(val->s, buf);
    return val;
}

/* Returns a Value* of the given type. */
Value *make_special(valueType t) {
    Value *val = talloc(sizeof(Value));
    val->type = t;
    return val;
}

/* Reads a string from stdin into the buffer.  Whenever a newline occurs,
 * increments the value at line_num.  The first char read should be a double
 * quote, and continues to read chars until another double quote is seen, or
 * until EOF (which throws an error).  Stores the complete string, including
 * both the initial and final double quote, in the buffer.  Returns the length
 * of the string in the buffer, including the quotes but excluding the null
 * terminator. */
int read_string(char *buf, int *line_num) {
    // Returns the length of the string, excluding the trailing '\0'
    // Returns the number of newline characters in the string
    // Assumes unget() has been called on leading "
    int i = 1;
    char char_read = get();
    buf[0] = char_read;
    char_read = get();
    while (char_read != EOF && char_read != '"' && i < BUFSIZE - 1) {
        if (char_read == '\n')
            (*line_num)++;
        buf[i] = char_read;
        i++;
        char_read = get();
    }
    if (char_read == EOF) {
        fprintf(stderr, "Syntax error: line %d: unexpected EOF when reading string; expected \"\n", *line_num);
        texit(1);
    }
    buf[i] = char_read;
    i++;
    buf[i] = '\0';
    if (i == BUFSIZE - 1) {
        fprintf(stderr, "WARNING: line %d: string length greater than or equal to the length of the buffer:\n", *line_num);
    }
    return i;
}

/* Reads a token from stdin into the buffer.  Whenever a newline occurs,
 * increments the value at line_num.  Reads chars until a delimiter char is
 * observed (see is_delimiter() for which chars are delimiters).  The unget()
 * function is called on the delimiter when it is observed, so it can be
 * properly handled in the tokenize function.  Stores the complete token,
 * excluding the delimiter, in the buffer.  Returns the length of the token
 * in the buffer, excluding the delimiter and the null terminator. */
int read_token(char *buf, int *line_num) {
    // Returns the length of the token, excluding the trailing '\0'
    // Should not be called when delimiter or special character is first
    // Assumes unget() has been called on leading character
    int i = 0;
    char char_read = get();
    while (char_read != EOF && !is_delimiter(char_read) && i < BUFSIZE - 1) {
        buf[i] = char_read;
        i++;
        char_read = get();
    }
    buf[i] = '\0';
    unget(char_read);   // delimiter
    if (i == BUFSIZE - 1) {
        fprintf(stderr, "WARNING: line %d: token length greater than or equal to the length of the buffer:\n", *line_num);
    }
    return i;
}

/* Read all of the input from stdin, and return a linked list consisting of the
 * tokens. */
Value *tokenize() {
    char char_read;
    char buf[BUFSIZE];
    int token_len;
    int line_num = 1;
    Value *revList;
    Value *list = makeNull();
    char_read = get();

    while (char_read != EOF) {

        // Assumption: whenever this loop happens, we are looking for a new token
        // Assume non-numeric identifiers do not start with digit, +, -, or .
        //     However, +, -, and ... are valid identifiers
        //     Additionally, . is special as well, if surrounded by whitespace

        switch (char_read) {
            // delimiters
            case ' ':
            case '\t':
                break;
            case '\n':
                line_num++;
                break;
            case '\r':
                fprintf(stderr, "WARNING: carriage return character detected. Treating as whitespace, not newline.\n");
                break;
            case ';':
                while (char_read != EOF && char_read != '\n')
                    char_read = get();
                line_num++;
                break;
            case '(':
                list = cons(make_special(OPEN_TYPE), list);
                break;
            case ')':
                list = cons(make_special(CLOSE_TYPE), list);
                break;
            case '[':
                list = cons(make_special(OPENBRACKET_TYPE), list);
                break;
            case ']':
                list = cons(make_special(CLOSEBRACKET_TYPE), list);
                break;
            case '"':
                unget(char_read);
                token_len = read_string(buf, &line_num);
                list = cons(make_string(buf), list);
                break;
            case '\'':
                list = cons(make_special(SINGLEQUOTE_TYPE), list);
                break;
            case '#':
                // we do NOT unget(#), as that loops forever
                // instead, compare the remainder of the token after the leading #
                token_len = read_token(buf, &line_num);
                if (strcmp(buf, "t") == 0) {
                    list = cons(make_bool(1), list);
                } else if (strcmp(buf, "f") == 0) {
                    list = cons(make_bool(0), list);
                } else {
                    // there are valid options we have not handled yet
                    fprintf(stderr, "Syntax error: line %d: handling for special token %s not yet implemented\n", line_num, buf);
                    texit(1);
                }
                break;
            default:
                // non-delimiters
                unget(char_read);
                token_len = read_token(buf, &line_num);
                if (buf[0] == '+' || buf[0] == '-') {
                    if (token_len == 1) {
                        list = cons(make_symbol(buf), list);
                    } else if (is_integer(buf)) {
                        list = cons(make_integer(buf), list);
                    } else if (is_double(buf)) {
                        list = cons(make_double(buf), list);
                    } else {
                        fprintf(stderr, "Syntax error: line %d: invalid symbol %s: Symbols may not begin with + or - unless the complete symbol is + or -\n", line_num, buf);
                        texit(1);
                    }
                } else if (buf[0] == '.') {
                    if (token_len == 1) {
                        list = cons(make_special(DOT_TYPE), list);
                    } else if (strcmp(buf, "...") == 0) {
                        list = cons(make_symbol(buf), list);
                    } else if (is_double(buf)) {
                        list = cons(make_double(buf), list);
                    } else {
                        fprintf(stderr, "Syntax error: line %d: invalid symbol %s: Symbols may not begin with . unless the complete symbol is . or ...\n", line_num, buf);
                        texit(1);
                    }
                } else if (is_digit(buf[0])) {
                    if (is_integer(buf)) {
                        list = cons(make_integer(buf), list);
                    } else if (is_double(buf)) {
                        list = cons(make_double(buf), list);
                    } else {
                        fprintf(stderr, "Syntax error: line %d: invalid symbol %s: Symbols may not begin with a number\n", line_num, buf);
                        texit(1);
                    }
                } else if (is_symbol(buf)) {
                    list = cons(make_symbol(buf), list);
                } else {
                    fprintf(stderr, "Syntax error: line %d: invalid symbol %s: Symbol contains invalid character\n", line_num, buf);
                    texit(1);
                }
        }
        char_read = get();
    }
    revList = reverse(list);
    return revList;
}

/* Displays the contents of the linked list as tokens, with type information. */
void displayTokens(Value *list) {
    // Assume list is well-formed; ie. no null pointers
    while (list->type != NULL_TYPE) {
        switch (car(list)->type) {
            case INT_TYPE:
                printf("%d:integer\n", car(list)->i);
                break;
            case DOUBLE_TYPE:
                printf("%lf:double\n", car(list)->d);
                break;
            case STR_TYPE:
                printf("%s:string\n", car(list)->s);
                break;
            case OPEN_TYPE:
                printf("(:open\n");
                break;
            case CLOSE_TYPE:
                printf("):close\n");
                break;
            case BOOL_TYPE:
                if (car(list)->i == 0)
                    printf("#f:boolean\n");
                else
                    printf("#t:boolean\n");
                break;
            case SYMBOL_TYPE:
                printf("%s:symbol\n", car(list)->s);
                break;
            case OPENBRACKET_TYPE:
                printf("[:openbracket\n");
                break;
            case CLOSEBRACKET_TYPE:
                printf("]:closebracket\n");
                break;
            case DOT_TYPE:
                printf(".:dot\n");
                break;
            case SINGLEQUOTE_TYPE:
                printf("':singlequote\n");
                break;
            default:
                ;   // Never add any other types to the token list
        }
        list = cdr(list);
    }
}

