CC = mpicc
CFLAGS = -g -Wall
LIBS = -lm

all: mcs dissemination

mcs: mcs.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

dissemination: dissemination.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o mcs dissemination
