CC = cc
CFLAGS = -g -O3

SRCS = linkedlist.c main.c talloc.c tokenizer.c
HDRS = linkedlist.h value.h talloc.h tokenizer.h
OBJS = $(SRCS:.c=.o)

.PHONY: interpreter
interpreter: $(OBJS)
	$(CC)  $(CFLAGS) $^  -o $@
	rm -f *.o
	rm -f vgcore.*

.PHONY: phony_target
phony_target:

%.o : %.c $(HDRS) phony_target
	$(CC)  $(CFLAGS) -c $<  -o $@

clean:
	rm -f *.o
	rm -f interpreter

