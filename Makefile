brot: main.o brot.o
	g++ main.o brot.o -o brot

brot.o: brot.cpp
	g++ brot.cpp -o brot.o

main.o: main.cpp
	g++ main.cpp -o main.o

