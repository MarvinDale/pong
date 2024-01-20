default: build run

build:
	g++ -Wall main.cpp -ld2d1 -ldwrite -o main.exe

debug:
	g++ -ggdb -Wall main.cpp -ld2d1 -ldwrite -o main.exe

run:
	./main.exe

clean:
	rm main.exe
