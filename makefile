CC = gcc
CFLAGS := -O3 -c
SOURCES=main.c classicrbn.c experiments.c rbnio.c rng.c fastrbn.c fastexp.c
HEADERS=$(wildcard *.h)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=netDynamics

# make OPENMP=1 to run the networks (attractors) and initial states (phase) in parallel.
ifeq ($(OPENMP),1)
CFLAGS  += -fopenmp
LDFLAGS += -fopenmp
endif
# make NATIVE=1 to optimize for this processor (uses pext with BMI2 in x86).
ifeq ($(NATIVE),1)
CFLAGS  += -march=native
endif

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ -lm

$(OBJECTS): $(HEADERS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@
