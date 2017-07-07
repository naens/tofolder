CFLAGS=-g

all: tofolder_cli tofolder_gui

tofolder_cli:
	gcc -g -static tofolder_cli.c  -L/home/andrei/lib -lpedit -I/home/andrei/include -o tofolder_cli

tofolder_gui:

clean:
	rm -f tofolder_cli tofolder_gui