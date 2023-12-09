default: build run

build:
	g++ -Wall  main.cpp -ld2d1 -o main.exe

run:
	./main.exe
