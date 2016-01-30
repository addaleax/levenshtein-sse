CXXFLAGS = -O3 -DNDEBUG -march=native -std=c++14 -Wall -Wextra -Wno-unused-parameter

all: test/test

test/test: test/test.cpp Levenshtein-SSE.hpp FileMappedString.hpp ErrnoException.hpp AlignmentAllocator.hpp
	$(CXX) $(CXXFLAGS) $(CLFAGS) -I. -o $@ test/test.cpp
	test/test
