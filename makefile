.DEFAULT_GOAL := compile

headers = spin-lock.hpp ffs-lock.hpp test.hpp
sources = main.cpp test.cpp

.PHONY : compile clean

compile: $(headers) $(sources)
	g++ $(sources) -o test.out -pthread -Wall -lm -g -std=c++17

clean:
	rm -f test.out
