OMPLIBS = -lgomp

CC = mpicc
CFLAGS = -g -Wall -fopenmp
LIBS = -lm $(OMPLIBS)

all: mcs_sense

mcs_sense: mcs_sense.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f *.o mcs_sense
