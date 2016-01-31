levenshtein-sse
===============

[![Build Status](https://travis-ci.org/addaleax/levenshtein-sse.svg?style=flat&branch=master)](https://travis-ci.org/addaleax/levenshtein-sse?branch=master)

Really, *really* fast Levenshtein distance in C++, header-only implementation.

Usage
=====

```cpp
#include "levenshtein-sse.hpp"
using levenshteinSSE::levenshtein;
```

```cpp
template<typename Iterator1, typename Iterator2>
std::size_t levenshtein(Iterator1 a, Iterator1 aEnd, Iterator2 b, Iterator2 bEnd);
```

Compute the Levenshtein distances of `[a, aEnd)` and `[b, bEnd)`.
Both types of iterators need to be bidirectional, e.g.
fulfill the requirements of `BidirectionalIterator`, but may just be
pointers.

For pointers to types where SIMD instructions make sense, these are
used when available.

```cpp
template<typename Container1, typename Container2>
std::size_t levenshtein(const Container1& a, const Container2& b);
```

Compute the Levenshtein distances of a and b.
Both containers need to support bidirectional start and end iterators
(via `std::begin()` and `std::end()`).

If both containers provide `.data()` and `.size()`, these are used to
get pointers to the start and past-end elements. This holds true
for `std::string`, `std::vector`, `std::array` and possibly more, but
if you want to use this with custom container types, bear in mind that
this also means that the available memory area needs to be contiguous.

License
=======

MIT
