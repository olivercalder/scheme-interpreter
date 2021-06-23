#include <stdio.h>
#include "tokenizer.h"
#include "value.h"
#include "linkedlist.h"
#include "talloc.h"

int main() {
    Value *list = tokenize();
    displayTokens(list);
    tfree();
}
