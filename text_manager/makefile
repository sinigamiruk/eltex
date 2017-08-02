COMPILER=gcc
CFLAGS=-g -Wall -std=c99 -lncurses -lm -lpthread
SOURSES=main.c copy.c navigate.c print_files.c ui.c
OBJECTS=$(SOURSES:.c=.o)
EXECUTABLE=fman

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(COMPILER)  $(OBJECTS) -o $@ $(CFLAGS)

##$(OBJECTS):$(SOURSES)
#	$(COMPILER) $(CFLAGS) -c $(SOURSES)

.o:.c
	$(COMPILER) $(CFLAGS) -c $<

clean:
	rm $(OBJECTS) $(EXECUTABLE)

run: $(EXECUTABLE)
	./$(EXECUTABLE)
