#ifndef _VALUE
#define _VALUE

typedef enum {INT_TYPE,DOUBLE_TYPE,STR_TYPE,CONS_TYPE,NULL_TYPE} valueType;

struct Value {
    valueType type;
    union {
        int i;
        double d;
        char *s;
        struct ConsCell {
            struct Value *car;
            struct Value *cdr;
        } c;
    };
};


typedef struct Value Value;

#endif
