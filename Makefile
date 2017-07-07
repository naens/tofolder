CFLAGS=-g

all: clean tofolder_cli tofolder_gui

tofolder_cli:
	gcc -g -static tofolder_cli.c  -L/home/andrei/lib -lpedit -I/home/andrei/include -o tofolder_cli

tofolder_gui:

install: tofolder_cli tofolder_gui
	mkdir -p ~/bin
	cp tofolder_cli ~/bin

clean:
	rm -f tofolder_cli tofolder_gui