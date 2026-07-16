CC = gcc
CFLAGS = -O0 -Wall
LDFLAGS = -lm

DELAY = 750

all: work1 work2 work3 work4 work5 work6 work7 work5x2_io

work1: work.c
	$(CC) $(CFLAGS) -DLOAD=1 -DDELAY=$(DELAY) -o work1 work.c $(LDFLAGS)

work2: work.c
	$(CC) $(CFLAGS) -DLOAD=2 -DDELAY=$(DELAY) -o work2 work.c $(LDFLAGS)

work3: work.c
	$(CC) $(CFLAGS) -DLOAD=3 -DDELAY=$(DELAY) -o work3 work.c $(LDFLAGS)

work4: work.c
	$(CC) $(CFLAGS) -DLOAD=4 -DDELAY=$(DELAY) -o work4 work.c $(LDFLAGS)

work5: work.c
	$(CC) $(CFLAGS) -DLOAD=5 -DDELAY=$(DELAY) -o work5 work.c $(LDFLAGS)

work6: work.c
	$(CC) $(CFLAGS) -DLOAD=6 -DDELAY=$(DELAY) -o work6 work.c $(LDFLAGS)

work7: work.c
	$(CC) $(CFLAGS) -DLOAD=7 -DDELAY=$(DELAY) -o work7 work.c $(LDFLAGS)

work5x2_io: work_io.c
	$(CC) $(CFLAGS) -DLOAD=5 -DDELAY=$(DELAY) -o work5x2_io work_io.c $(LDFLAGS)

clean:
	rm -f work1 work2 work3 work4 work5 work6 work7 work5x2_io *.o