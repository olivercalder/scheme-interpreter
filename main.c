#include <stdio.h>
#include "tokenizer.h"
#include "value.h"
#include "linkedlist.h"
#include "parser.h"
#include "talloc.h"

/* Main for parser, by Dave Musicant
   Fairly self-explanatory. Tokenizes, parses, and prints out a parsed Scheme program. */

int main() {

    Value *list = tokenize();
    Value *tree = parse(list);

    printTree(tree);
    printf("\n");

    tfree();
    return 0;
}
