
bench: *.cpp *.h
	mkdir -p .bin
	g++ main.cpp -o .bin/bench -O3 -std=c++17

