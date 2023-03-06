# This is a makefile to build the C++ project.

# The .DEFAULT_GOAL variable sets the default target to compile. 
.DEFAULT_GOAL := compile

# List of header files
headers = spin-lock.hpp ffs-lock.hpp test.hpp

# List of source files
sources = main.cpp test.cpp

# The .PHONY directive specifies that compile and clean are phony targets,
# i.e., they don't correspond to actual files.
.PHONY : compile clean

# Target to compile the source code using g++ and creates an executable named "test.out".
# The -pthread option is used to link with the POSIX thread library
compile: $(headers) $(sources)
	g++ $(sources) -o test.out -pthread -Wall -lm -g -std=c++17

# Target to clean up the generated files ("test.out")
clean:
	rm -f test.out
