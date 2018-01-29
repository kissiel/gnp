.PHONY: debug all clean

all: gnp

gnp: main.c
	gcc -std=c99 -o gnp -O2 main.c -lgit2 $(CFLAGS)

clean:
	rm gnp

debug: CFLAGS := -DDEBUG_TO_STDOUT

debug: gnp
