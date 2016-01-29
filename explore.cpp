#include <algorithm>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cassert>

template<typename T>
std::ostream& operator <<(std::ostream& os, const std::vector<T>& vec) {
  os << '[';
  for (auto it = std::begin(vec); it != std::end(vec); ++it) {
    os << *it << ',';
  }
  os << ']';
  return os;
}

template<typename T, typename Iterator1, typename Iterator2>
T levenshteinDiagonal(Iterator1 a, Iterator1 aEnd, Iterator2 b, Iterator2 bEnd);

template<typename Iterator1, typename Iterator2>
std::size_t levenshtein(Iterator1 a, Iterator1 aEnd, Iterator2 b, Iterator2 bEnd) {
  if (aEnd - a > bEnd - b) {
    return levenshtein(b, bEnd, a, aEnd);
  }
  
  // skip common prefixes and suffixes
  while (a < aEnd && a[0] == b[0])
    ++a, ++b;
  
  while (a < aEnd && aEnd[-1] == bEnd[-1])
    --aEnd, --bEnd;
  
  std::size_t aLen = aEnd - a;
  std::size_t bLen = bEnd - b;
  
  if (aLen == 0) {
    return bLen;
  }
  
  if (aLen == 1) {
    return bLen - (std::find(b, bEnd, *a) == bEnd ? 0 : 1);
  }
  
  if (aLen + bLen <= std::numeric_limits<std::uint32_t>::max())
    return levenshteinDiagonal<std::uint32_t>(a, aEnd, b, bEnd);
  
  return levenshteinDiagonal<std::size_t>(a, aEnd, b, bEnd);
}

template<typename Vec1, typename Vec2, typename Iterator1, typename Iterator2>
struct LevenshteinIteration {
static inline void perform(const Iterator1& a, const Iterator2& b,
  std::size_t& i, std::size_t j, Vec1& diag, const Vec2& diag2)
{
  int substitutionCost = a[i-1] == b[j-1] ? 0 : 1;
  diag[i] = std::min({
    diag2[i-1]+1,
    diag2[i]+1,
    diag[i-1] + substitutionCost
  });
  --i;
}
};

template<typename Alloc1, typename Alloc2>
struct LevenshteinIteration<std::vector<std::uint32_t, Alloc1>, std::vector<uint32_t, Alloc2>, const char*, const char*> {
static inline void perform(const char* a, const char* b,
  std::size_t& i, std::size_t j, std::vector<std::uint32_t, Alloc1>& diag, const std::vector<std::uint32_t, Alloc2>& diag2)
{
  int substitutionCost = a[i-1] == b[j-1] ? 0 : 1;
  diag[i] = std::min({
    diag2[i-1]+1,
    diag2[i]+1,
    diag[i-1] + substitutionCost
  });
  --i;
}
};

template<typename Vec1, typename Vec2>
struct LevenshteinIteration<Vec1, Vec2, std::string::iterator, std::string::iterator> {
static inline void perform(const std::string::iterator& a, const std::string::iterator& b,
  std::size_t& i, std::size_t j, Vec1& diag, const Vec2& diag2)
{
  LevenshteinIteration<Vec1, Vec2, const char*, const char*>
    ::perform(&*a, &*b, i, j, diag, diag2);
}
};

template<typename Vec1, typename Vec2>
struct LevenshteinIteration<Vec1, Vec2, std::string::const_iterator, std::string::const_iterator> {
static inline void perform(const std::string::const_iterator& a, const std::string::const_iterator& b,
  std::size_t& i, std::size_t j, Vec1& diag, const Vec2& diag2)
{
  LevenshteinIteration<Vec1, Vec2, const char*, const char*>
    ::perform(&*a, &*b, i, j, diag, diag2);
}
};

template<typename T, typename Iterator1, typename Iterator2>
T levenshteinDiagonal(Iterator1 a, Iterator1 aEnd, Iterator2 b, Iterator2 bEnd) {
  const std::size_t aLen = aEnd - a;
  const std::size_t bLen = bEnd - b;
  
  assert(0 < aLen);
  assert(aLen <= bLen);
  
  std::vector<T> diag  (aLen + 1, T(0));
  std::vector<T> diag2 (aLen + 1, T(0));
  
  std::size_t i, j, k;
  
  k = 0;
  for (k = 1; ; ++k) {
    if (k % 1024 == 0) {
      std::cerr << "k = " << k << " (" << (k * 100.0 / double(aLen + bLen)) << " %)" << std::endl;
    }
    
    assert(k <= aLen + bLen);
    
    std::size_t startRow = k > bLen ? k - bLen : 1;
    std::size_t endRow = k > aLen ? aLen : k - 1;
    
    assert(endRow >= startRow || k == 1);
    
    for (i = endRow; i >= startRow; ) {
      assert(i < k);
      j = k - i;
      
      assert(bLen >= j);
      assert(aLen >= i);
      
      LevenshteinIteration<std::vector<T>, std::vector<T>, Iterator1, Iterator2>
        ::perform(a, b, i, j, diag, diag2);
    }
    
    diag[0] = k;
    
    if (k <= aLen) {
      diag[k] = k;
    }
    
    if (k == aLen + bLen) {
      assert(startRow == endRow);
      return diag[startRow];
    }
    
    // switch buffers
    std::swap(diag, diag2);
  }
  
  assert(0);
}

template<typename String>
std::size_t levenshteinString(const String& a, const String& b) {
  return levenshtein(std::cbegin(a), std::cend(a), std::cbegin(b), std::cend(b));
}

/* Testing */
#include "FileMappedString.hpp"

void levenshteinStringExpect(const std::string& a, const std::string& b, uint32_t expected) {
  auto distance = levenshteinString(a, b);
  
  std::cerr << "A = " << a << "\nB = " << b <<
      "\ndistance = " << distance << ", expected = " << expected << "\n";
  
  assert (distance == expected);
}

void levenshteinFileExpect(const std::string& a, const std::string& b, uint32_t expected) {
  auto distance = levenshteinString(FileMappedString(a), FileMappedString(b));
  
  std::cerr << "A = " << a << "\nB = " << b <<
      "\ndistance = " << distance << ", expected = " << expected << "\n";
  
  assert (distance == expected);
}

int main() {
  levenshteinStringExpect("Saturday", "Sunday", 3);
  levenshteinStringExpect("Sitting", "Kittens", 3);
  levenshteinStringExpect("Kittens", "Sitting", 3);
  levenshteinStringExpect("Kitten", "Sitting", 3);
  levenshteinStringExpect("Hallo, Welt!", "Hello, World!", 4);
  levenshteinStringExpect("", "", 0);
  levenshteinStringExpect("A", "", 1);
  levenshteinStringExpect("A", "A", 0);
  levenshteinStringExpect("A", "Sitting", 7);
  levenshteinStringExpect("", "Sitting", 7);
  levenshteinStringExpect("Sitting", "Sitting", 0);
  levenshteinFileExpect("/usr/bin/xz", "/usr/local/bin/xz", 62962);
  return 0;
}
