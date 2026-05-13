EXEC = a.out
SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=%.o)
CC=gcc
CFLAGS=-std=gnu99 -I.
LDLIBS=-lm -lrt -pthread
LDFLAGS=-z execstack

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

parse_line.o: parse_line.c parse_line.h

list.o: list.c list.h

job_control.o: job_control.c job_control.h parse_line.h list.h

clean:
	@rm -f $(OBJS)

vclean: clean
	@rm -f $(EXEC)

