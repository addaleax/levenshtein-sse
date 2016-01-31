CXXFLAGS = -O3 -DNDEBUG -march=native -std=c++11 -Wall -Wextra -Wno-unused-parameter

all: test/test

test/test: test/test.cpp levenshtein-sse.hpp test/FileMappedString.hpp test/ErrnoException.hpp
	$(CXX) $(CXXFLAGS) $(CLFAGS) -I. -o $@ test/test.cpp
	time test/test

clean:
	rm -f test/test
