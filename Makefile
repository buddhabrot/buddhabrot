brot: main.o brot.o
	g++ main.o brot.o -o brot

brot.o: brot.cpp
	g++ -g -c brot.cpp -o brot.o

main.o: main.cpp
	g++ -g -c main.cpp -o main.o

