CC = cc
CFLAGS = -g -O3

SRCS = linkedlist.c main.c
HDRS = linkedlist.h value.h
OBJS = $(SRCS:.c=.o)

.PHONY: linkedlist
linkedlist: $(OBJS)
	$(CC)  $(CFLAGS) $^  -o $@

.PHONY: phony_target
phony_target:

%.o : %.c $(HDRS) phony_target
	$(CC)  $(CFLAGS) -c $<  -o $@

clean:
	rm *.o
	rm linkedlist

valgrind: linkedlist
	valgrind --leak-check=yes --error-exitcode=1 ./linkedlist
