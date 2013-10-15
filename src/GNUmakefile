CC     = gcc
CFLAGS = -Wl,--export-dynamic $(shell pkg-config --cflags gtk+-3.0 glib-2.0 gmodule-export-2.0) 
LIBS   = $(shell pkg-config --libs gtk+-3.0 glib-2.0 gmodule-export-2.0)
DEBUG  = -Wall -g

OBJECTS = main.o ui.o

.PHONY: clean

all: jahaziel

jahaziel: $(OBJECTS)
	$(CC) $(DEBUG) $(LIBS) $(OBJECTS) -o $@

main.o: main.c
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@

ui.o: ui.c
	$(CC) $(DEBUG) $(CFLAGS) -c $< -o $@


