OMPFLAGS = -fopenmp
OMPLIBS = -lgomp

CC = gcc -std=c99
CPPFLAGS = -g -Wall $(OMPFLAGS)
LDFLAGS = -g -Wall
LDLIBS = $(OMPLIBS)

all: centralized_sense dissemination

centralized_sense: centralized_sense.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

dissemination: dissemination.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) -lm

clean:
	rm -f *.o centralized_sense dissemination
