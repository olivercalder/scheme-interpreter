// Tester for linked list.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "linkedlist.h"
#include "value.h"

void testForward(Value *head, int correctLength, bool exemplary) {
  Value *value = head;
  assert(CONS_TYPE == value->type);
  assert(DOUBLE_TYPE == car(value)->type);
  assert(1 == car(value)->d);

  if (exemplary) {
    value = cdr(value);
    assert(CONS_TYPE == value->type);
    assert(STR_TYPE == car(value)->type);
    assert(!strcmp("2.0s", car(value)->s));

    value = cdr(value);
    assert(CONS_TYPE == value->type);
    assert(STR_TYPE == car(value)->type);
    assert(!strcmp("3.0s", car(value)->s));
  }

  value = cdr(value);
  assert(CONS_TYPE == value->type);
  assert(DOUBLE_TYPE == car(value)->type);
  assert(4 == car(value)->d);

  if (exemplary) {
    value = cdr(value);
    assert(CONS_TYPE == value->type);
    assert(STR_TYPE == car(value)->type);
    assert(!strcmp("5.0s", car(value)->s));
  }

  value = cdr(value);
  assert(CONS_TYPE == value->type);
  assert(DOUBLE_TYPE == car(value)->type);
  assert(6 == car(value)->d);

  value = cdr(value);
  assert(CONS_TYPE == value->type);
  assert(INT_TYPE == car(value)->type);
  assert(7 == car(value)->i);

  value = cdr(value);
  assert(NULL_TYPE == value->type);

  assert(correctLength == length(head));
  assert(!isNull(head));
}

void testBackward(Value *head, int correctLength, bool exemplary) {
  Value *value = head;

  assert(CONS_TYPE == value->type);
  assert(INT_TYPE == car(value)->type);
  assert(7 == car(value)->i);

  value = cdr(value);
  assert(CONS_TYPE == value->type);
  assert(DOUBLE_TYPE == car(value)->type);
  assert(6 == car(value)->d);

  if (exemplary) {
    value = cdr(value);
    assert(CONS_TYPE == value->type);
    assert(STR_TYPE == car(value)->type);
    assert(!strcmp("5.0s", car(value)->s));
  }

  value = cdr(value);
  assert(CONS_TYPE == value->type);
  assert(DOUBLE_TYPE == car(value)->type);
  assert(4 == car(value)->d);

  if (exemplary) {
    value = cdr(value);
    assert(CONS_TYPE == value->type);
    assert(STR_TYPE == car(value)->type);
    assert(!strcmp("3.0s", car(value)->s));

    value = cdr(value);
    assert(CONS_TYPE == value->type);
    assert(STR_TYPE == car(value)->type);
    assert(!strcmp("2.0s", car(value)->s));
  }

  value = cdr(value);
  assert(CONS_TYPE == value->type);
  assert(DOUBLE_TYPE == car(value)->type);
  assert(1 == car(value)->d);

  value = cdr(value);
  assert(NULL_TYPE == value->type);

  assert(correctLength == length(head));
  assert(!isNull(head));
}


int main(int argc, char **argv) {

  bool exemplary = false;
  if (argc == 2 && !strcmp(argv[1], "E")) {
    exemplary = true;
  }

  Value *head = makeNull();
  int correctLength = 0;
  assert(length(head) == correctLength);

  Value *reverseLengthZero = reverse(head);
  assert(length(reverseLengthZero) == correctLength);
  cleanup(reverseLengthZero);

  Value *val1 = malloc(sizeof(Value));
  val1->type = INT_TYPE;
  val1->i = 7;
  head = cons(val1,head);
  correctLength++;
  assert(length(head) == correctLength);

  Value *reverseLengthOne = reverse(head);
  assert(length(reverseLengthOne) == correctLength);
  cleanup(reverseLengthOne);

  Value *val2 = malloc(sizeof(Value));
  val2->type = DOUBLE_TYPE;
  val2->d = 6.00;
  head = cons(val2,head);
  correctLength++;
  assert(length(head) == correctLength);

  if (exemplary) {
    Value *val3 = malloc(sizeof(Value));
    val3->type = STR_TYPE;
    char *text = "5.0s";
    val3->s = malloc(sizeof(char)*(strlen(text) + 1));
    strcpy(val3->s,text);
    head = cons(val3,head);
    correctLength++;
    assert(length(head) == correctLength);

  }

  Value *val4 = malloc(sizeof(Value));
  val4->type = DOUBLE_TYPE;
  val4->d = 4.00000;
  head = cons(val4,head);
  correctLength++;
  assert(length(head) == correctLength);

  if (exemplary) {
    Value *val5 = malloc(sizeof(Value));
    val5->type = STR_TYPE;
    char *text = "3.0s";
    val5->s = malloc(sizeof(char)*(strlen(text) + 1));
    strcpy(val5->s,text);
    head = cons(val5,head);
    correctLength++;
    assert(length(head) == correctLength);

    Value *val6 = malloc(sizeof(Value));
    val6->type = STR_TYPE;
    text = "2.0s";
    val6->s = malloc(sizeof(char)*(strlen(text) + 1));
    strcpy(val6->s,text);
    head = cons(val6,head);
    correctLength++;
    assert(length(head) == correctLength);

  }

  Value *val7 = malloc(sizeof(Value));
  val7->type = DOUBLE_TYPE;
  val7->d = 1.0;
  head = cons(val7,head);
  correctLength++;
  assert(length(head) == correctLength);

    
  display(head);
  testForward(head, correctLength, exemplary);

  Value *rev = reverse(head);
  display(rev);

  testBackward(rev, correctLength, exemplary);

  cleanup(head);
  cleanup(rev);

  head = NULL;
  rev = NULL;
}
