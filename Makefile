buddhabrot: out build debug
	gcc -o build/buddhabrot main.c -lgd -lpng -lpthread
	gcc -o debug/buddhabrot main.c -g -pg -lgd -lpng -lpthread

out: 
	mkdir out

build:
	mkdir build

debug:
	mkdir debug
