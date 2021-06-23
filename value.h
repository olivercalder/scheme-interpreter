#ifndef _VALUE
#define _VALUE

typedef enum {
    INT_TYPE, DOUBLE_TYPE, STR_TYPE, CONS_TYPE, NULL_TYPE, PTR_TYPE,
    OPEN_TYPE, CLOSE_TYPE, BOOL_TYPE, SYMBOL_TYPE,

    // Types below are only for bonus work
    OPENBRACKET_TYPE, CLOSEBRACKET_TYPE, DOT_TYPE, SINGLEQUOTE_TYPE,
    VOID_TYPE
} valueType;

struct Value {
    valueType type;
    union {
        int i;
        double d;
        char *s;
        void *p;
        struct ConsCell {
            struct Value *car;
            struct Value *cdr;
        } c;
    };
};

typedef struct Value Value;

// A frame is a linked list of bindings, and a pointer to another frame.  A
// binding is a variable name (represented as a string), and a pointer to the
// Value it is bound to. Specifically how you implement the list of bindings is
// up to you.
struct Frame {
    Value *bindings;
    struct Frame *parent;
};

typedef struct Frame Frame;


#endif
