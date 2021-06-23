#ifndef _INTERPRETER
#define _INTERPRETER

Value *eval(Value *expr, Frame *frame);

Value *eval_begin(Value *expr, Frame *frame);

Value *eval_not(Value *expr, Frame *frame);

Value *eval_if(Value *expr, Frame *frame);

Value *eval_when(Value *expr, Frame *frame);

Value *eval_unless(Value *expr, Frame *frame);

Value *eval_let(Value *expr, Frame *frame);

Value *eval_let_star(Value *expr, Frame *frame);

Value *eval_display(Value *expr, Frame *frame);

void interpret(Value *tree);

#endif

