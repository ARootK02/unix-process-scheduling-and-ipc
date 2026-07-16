CC = gcc
CFLAGS = -Wall
LDFLAGS = -lm

all: scheduler scheduler_io

scheduler: scheduler.c
	$(CC) $(CFLAGS) -o scheduler scheduler.c $(LDFLAGS)

scheduler_io: scheduler_io.c
	$(CC) $(CFLAGS) -o scheduler_io scheduler_io.c $(LDFLAGS)

clean:
	rm -f scheduler scheduler_io