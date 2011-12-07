INCLUDES=-I$(GD_INCLUDEDIR)
LIBS=-I$(GD_LIBDIR)
LIB=-lgd -lpng -lpthread
OPT=-ffast-math -Wall -Wno-implicit-function-declaration

buddhabrot: out build debug
	gcc -o  build/basic -O3 basic.c ${LIB} ${OPT}
	time ./build/basic 256 256 /tmp 128

out: 
	mkdir out

build:
	mkdir build

debug:
	mkdir debug
