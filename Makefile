CXXFLAGS = -O3 -DNDEBUG -march=native -std=c++11 -Wall -Wextra -Wno-unused-parameter

all: test/test

test/test: test/test.cpp Levenshtein-SSE.hpp AlignmentAllocator.hpp test/FileMappedString.hpp test/ErrnoException.hpp
	$(CXX) $(CXXFLAGS) $(CLFAGS) -I. -o $@ test/test.cpp
	test/test
