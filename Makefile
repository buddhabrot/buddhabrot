LIB=-lgd -lpng -lpthread
OPT=-ffast-math -Wall

buddhabrot: out build debug
	gcc -o  build/buddhabrot -O3 main.c ${LIB} ${OPT}
	gcc -o  debug/buddhabrot main.c -g -pg ${LIB} ${OPT} 
	gcc -o  build/memoized -O3 memoized.c ${LIB} ${OPT}
	./build/memoized 256 256 /tmp

out: 
	mkdir out

build:
	mkdir build

debug:
	mkdir debug
