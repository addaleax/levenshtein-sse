#include <algorithm>
#include <vector>
#include <cstdint>
#include <cassert>
#include <iostream>

#include "AlignmentAllocator.hpp"

namespace levenshteinSSE {
#ifdef __SSSE3__
constexpr std::size_t alignment = 16;
#else
#warning "No SIMD extensions enabled"
constexpr std::size_t alignment = 1;
#endif

template<typename Vec1, typename Vec2, typename Iterator1, typename Iterator2>
struct LevenshteinIterationBase {
static inline void perform(const Iterator1& a, const Iterator2& b,
  std::size_t& i, std::size_t j, std::size_t bLen, Vec1& diag, const Vec2& diag2)
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

template<typename Vec1, typename Vec2, typename Iterator1, typename Iterator2>
struct LevenshteinIteration : LevenshteinIterationBase<Vec1, Vec2, Iterator1, Iterator2> {
};

template<typename T>
struct LevenshteinIterationSIMD {
/* Decide which implementation is acceptable */
static inline void performSIMD(const T* a, const T* b,
  std::size_t& i, std::size_t j, std::size_t bLen,
  std::uint32_t* diag, const std::uint32_t* diag2)
{
#ifdef __SSSE3__
  if (i >= 16 && bLen - j >= 16) {
    performSSE(a, b, i, j, bLen, diag, diag2);
    return;
  }
#endif
  
  LevenshteinIterationBase<std::uint32_t*, const std::uint32_t*, const T*, const T*>
    ::perform(a, b, i, j, bLen, diag, diag2);
}

/* SSE */

#ifdef __SSSE3__
#ifndef __SSE4_1__
// we need a replacement for _mm_min_epi32
#define _mm_min_epi32 FLST__mm_min_epi32

static inline
__m128i _mm_min_epi32 (__m128i a, __m128i b) {
  __m128i compare = _mm_cmpgt_epi32(a, b);
  __m128i aIsSmaller = _mm_andnot_si128(a, compare);
  __m128i bIsSmaller = _mm_and_si128   (b, compare);
  return _mm_or_si128(aIsSmaller, bIsSmaller);
}
#endif // __SSE4_1__

static inline void performSSE(const T* a, const T* b,
  std::size_t& i, std::size_t j, std::size_t bLen,
  std::uint32_t* diag, const std::uint32_t* diag2)
{
  const __m128i one128_epi8 = _mm_set1_epi8(1);
  const __m128i one128_epi16 = _mm_set1_epi16(1);
  const __m128i one128_epi32 = _mm_set1_epi32(1);
  const __m128i reversedIdentity128_epi8 = _mm_setr_epi8(
    15, 14, 13, 12,
    11, 10,  9,  8,
     7,  6,  5,  4,
     3,  2,  1,  0
  );
  const __m128i reversedIdentity128_epi16 = _mm_setr_epi8(
    14, 15, 12, 13,
    10, 11,  8,  9,
     6,  7,  4,  5,
     2,  3,  0,  1
  );
  const __m128i blockwiseReversed128_epi8_4 = _mm_setr_epi8(
     3,  2,  1,  0,
     7,  6,  5,  4,
    11, 10,  9,  8,
    15, 14, 13, 12
  );
  const __m128i blockwiseReversed128_epi16_4 = _mm_setr_epi8(
     6,  7,  4,  5,
     2,  3,  0,  1,
    14, 15, 12, 13,
    10, 11,  8,  9
  );

  __m128i zero = _mm_setzero_si128();
  __m128i substitutionCost32[4];
  std::size_t k;
  
  if (sizeof(T) <= 2) {
    __m128i substitutionCost16LX, substitutionCost16HX;
    
    if (sizeof(T) == 1) {
      __m128i a_ = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&a[i-16]));
      __m128i b_ = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&b[j-1]));
      a_ = _mm_shuffle_epi8(a_, reversedIdentity128_epi8);
      
      // int substitutionCost = a[i-1] == b[j-1] ? 0 : 1;
      
      // diag/diag2 will contain the following entries from the diagonal:
      // index  [0]0 [0]1 [0]2 [0]3 [1]0 [1]1 [1]2 [1]3 ... [4]0 [4]1 [4]2 [4]3
      // diag+i   -3   -2   -1    0   -7   -6   -5   -4 ...  -19  -18  -17  -16
      
      // substitutionCost8 contains the comparisons for the following indices in a and b:
      // index  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
      // a+i   -1  -2  -3  -4  -5  -6  -7  -8  -9 -10 -11 -12 -13 -14 -15 -16
      // b+j   -1   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14
      // in substitutionCost8X we shuffle this so that it matches up with diag/diag2
      // (barring the offset induces by comparing i-1 against j-1)
      __m128i substitutionCost8 = _mm_add_epi8(_mm_cmpeq_epi8(a_, b_), one128_epi8);
      __m128i substitutionCost8X = _mm_shuffle_epi8(substitutionCost8, blockwiseReversed128_epi8_4);
      substitutionCost16LX = _mm_unpacklo_epi8(substitutionCost8X, zero);
      substitutionCost16HX = _mm_unpackhi_epi8(substitutionCost8X, zero);
    } else {
      __m128i a0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&a[i-8]));
      __m128i a1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&a[i-16]));
      __m128i b0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&b[j-1]));
      __m128i b1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&b[j+7]));
      a0 = _mm_shuffle_epi8(a0, reversedIdentity128_epi16);
      a1 = _mm_shuffle_epi8(a1, reversedIdentity128_epi16);
      __m128i substitutionCost16L = _mm_add_epi16(_mm_cmpeq_epi16(a0, b0), one128_epi16);
      __m128i substitutionCost16H = _mm_add_epi16(_mm_cmpeq_epi16(a1, b1), one128_epi16);
      substitutionCost16LX = _mm_shuffle_epi8(substitutionCost16L, blockwiseReversed128_epi16_4);
      substitutionCost16HX = _mm_shuffle_epi8(substitutionCost16H, blockwiseReversed128_epi16_4);
    }
    
    substitutionCost32[0] = _mm_unpacklo_epi16(substitutionCost16LX, zero);
    substitutionCost32[1] = _mm_unpackhi_epi16(substitutionCost16LX, zero);
    substitutionCost32[2] = _mm_unpacklo_epi16(substitutionCost16HX, zero);
    substitutionCost32[3] = _mm_unpackhi_epi16(substitutionCost16HX, zero);
  } else {
    for (k = 0; k < 4; ++k) {
      __m128i a_ = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&a[i-4-k*4]));
      __m128i b_ = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&b[j-1+k*4]));
      b_ = _mm_shuffle_epi32(b_, 0x1b); // simple reverse
      substitutionCost32[k] = _mm_add_epi32(_mm_cmpeq_epi32(a_, b_), one128_epi32);
    }
  }
  
  __m128i diag_[5], diag2_[5];
  for (k = 0; k < 5; ++k) {
    diag_ [k] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&diag [i-3-k*4]));
  }
  for (k = 0; k < 5; ++k) {
    diag2_[k] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&diag2[i-3-k*4]));
  }
  
#ifndef NDEBUG
  for (k = 0; k < 5; ++k) {
    std::uint32_t diag0 = _mm_extract_epi32(diag_[k], 0);
    std::uint32_t diag1 = _mm_extract_epi32(diag_[k], 1);
    std::uint32_t diag2 = _mm_extract_epi32(diag_[k], 2);
    std::uint32_t diag3 = _mm_extract_epi32(diag_[k], 3);
    assert(diag0 == diag[i-k*4-3]);
    assert(diag1 == diag[i-k*4-2]);
    assert(diag2 == diag[i-k*4-1]);
    assert(diag3 == diag[i-k*4-0]);
  }
  for (k = 0; k < 5; ++k) {
    std::uint32_t diag20 = _mm_extract_epi32(diag2_[k], 0);
    std::uint32_t diag21 = _mm_extract_epi32(diag2_[k], 1);
    std::uint32_t diag22 = _mm_extract_epi32(diag2_[k], 2);
    std::uint32_t diag23 = _mm_extract_epi32(diag2_[k], 3);
    assert(diag20 == diag2[i-k*4-3]);
    assert(diag21 == diag2[i-k*4-2]);
    assert(diag22 == diag2[i-k*4-1]);
    assert(diag23 == diag2[i-k*4-0]);
  }
  
  for (k = 0; k < 4; ++k) {
    int sc0 = _mm_extract_epi32(substitutionCost32[k], 0);
    int sc1 = _mm_extract_epi32(substitutionCost32[k], 1);
    int sc2 = _mm_extract_epi32(substitutionCost32[k], 2);
    int sc3 = _mm_extract_epi32(substitutionCost32[k], 3);
    assert(sc0 == ((a[i-1-k*4-3] == b[j-1+k*4+3]) ? 0 : 1));
    assert(sc1 == ((a[i-1-k*4-2] == b[j-1+k*4+2]) ? 0 : 1));
    assert(sc2 == ((a[i-1-k*4-1] == b[j-1+k*4+1]) ? 0 : 1));
    assert(sc3 == ((a[i-1-k*4-0] == b[j-1+k*4+0]) ? 0 : 1));
  }
#endif

  // reminders:
  // the arrays (diag, diag2, substitutionCost32) correspond to i:
  // index  [0]0 [0]1 [0]2 [0]3 [1]0 [1]1 [1]2 [1]3 ... [4]0 [4]1 [4]2 [4]3
  // i+       -3   -2   -1    0   -7   -6   -5   -4 ...  -19  -18  -17  -16
  // so when we need to access ...[i-1], we need to align the entries
  // in *reverse* order
  
  // diag[i] = min(
  //  diag2[i-1] + 1,
  //  diag2[i]   + 1,
  //  diag[i-1]  + substitutionCost
  // );
  for (k = 0; k < 4; ++k) {
    __m128i diag2_i_m1 = _mm_alignr_epi8(diag2_[k], diag2_[k+1], 12);
    __m128i diag_i_m1  = _mm_alignr_epi8(diag_ [k], diag_ [k+1], 12);
    
    __m128i result1 = _mm_add_epi32(diag2_i_m1, one128_epi32);
    __m128i result2 = _mm_add_epi32(diag2_[k],  one128_epi32);
    __m128i result3 = _mm_add_epi32(diag_i_m1,  substitutionCost32[k]);
    __m128i min = _mm_min_epi32(_mm_min_epi32(result1, result2), result3);
    
#ifndef NDEBUG
    std::uint32_t diag_i_m10 = _mm_extract_epi32(diag_i_m1, 0);
    std::uint32_t diag_i_m11 = _mm_extract_epi32(diag_i_m1, 1);
    std::uint32_t diag_i_m12 = _mm_extract_epi32(diag_i_m1, 2);
    std::uint32_t diag_i_m13 = _mm_extract_epi32(diag_i_m1, 3);
    assert(diag_i_m10 == diag[i-k*4-4]);
    assert(diag_i_m11 == diag[i-k*4-3]);
    assert(diag_i_m12 == diag[i-k*4-2]);
    assert(diag_i_m13 == diag[i-k*4-1]);
    
    std::uint32_t diag2_i_m10 = _mm_extract_epi32(diag2_i_m1, 0);
    std::uint32_t diag2_i_m11 = _mm_extract_epi32(diag2_i_m1, 1);
    std::uint32_t diag2_i_m12 = _mm_extract_epi32(diag2_i_m1, 2);
    std::uint32_t diag2_i_m13 = _mm_extract_epi32(diag2_i_m1, 3);
    assert(diag2_i_m10 == diag2[i-k*4-4]);
    assert(diag2_i_m11 == diag2[i-k*4-3]);
    assert(diag2_i_m12 == diag2[i-k*4-2]);
    assert(diag2_i_m13 == diag2[i-k*4-1]);
    
    std::uint32_t result10 = _mm_extract_epi32(result1, 0);
    std::uint32_t result11 = _mm_extract_epi32(result1, 1);
    std::uint32_t result12 = _mm_extract_epi32(result1, 2);
    std::uint32_t result13 = _mm_extract_epi32(result1, 3);
    std::uint32_t result20 = _mm_extract_epi32(result2, 0);
    std::uint32_t result21 = _mm_extract_epi32(result2, 1);
    std::uint32_t result22 = _mm_extract_epi32(result2, 2);
    std::uint32_t result23 = _mm_extract_epi32(result2, 3);
    std::uint32_t result30 = _mm_extract_epi32(result3, 0);
    std::uint32_t result31 = _mm_extract_epi32(result3, 1);
    std::uint32_t result32 = _mm_extract_epi32(result3, 2);
    std::uint32_t result33 = _mm_extract_epi32(result3, 3);
    
    assert(result10 == diag2[i-k*4-4] + 1);
    assert(result11 == diag2[i-k*4-3] + 1);
    assert(result12 == diag2[i-k*4-2] + 1);
    assert(result13 == diag2[i-k*4-1] + 1);
    assert(result20 == diag2[i-k*4-3] + 1);
    assert(result21 == diag2[i-k*4-2] + 1);
    assert(result22 == diag2[i-k*4-1] + 1);
    assert(result23 == diag2[i-k*4-0] + 1);
    assert(result30 == diag[i-1-k*4-3] + ((a[i-1-k*4-3] == b[j-1+k*4+3]) ? 0 : 1));
    assert(result31 == diag[i-1-k*4-2] + ((a[i-1-k*4-2] == b[j-1+k*4+2]) ? 0 : 1));
    assert(result32 == diag[i-1-k*4-1] + ((a[i-1-k*4-1] == b[j-1+k*4+1]) ? 0 : 1));
    assert(result33 == diag[i-1-k*4-0] + ((a[i-1-k*4-0] == b[j-1+k*4+0]) ? 0 : 1));
#endif

    _mm_storeu_si128(reinterpret_cast<__m128i*>(&diag[i-k*4-3]), min);
  }
  
  i -= 16;
}
#endif // __SSSE3__
};

template<typename Alloc1, typename Alloc2, typename T>
struct LevenshteinIterationSIMDWrap : private LevenshteinIterationSIMD<T> {
static inline void perform(const T* a, const T* b,
  std::size_t& i, std::size_t j, std::size_t bLen,
  std::vector<std::uint32_t, Alloc1>& diag,
  const std::vector<std::uint32_t, Alloc2>& diag2) {
  return LevenshteinIterationSIMD<T>::performSIMD(a, b, i, j, bLen, diag.data(), diag2.data());
}
};

template<typename Alloc1, typename Alloc2, typename T>
struct LevenshteinIteration<std::vector<std::uint32_t, Alloc1>, std::vector<std::uint32_t, Alloc2>, const T*, const T*>
  : std::conditional<std::is_scalar<T>::value && (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4),
    LevenshteinIterationSIMDWrap<Alloc1, Alloc2, T>,
    LevenshteinIterationBase<std::vector<std::uint32_t, Alloc1>, std::vector<std::uint32_t, Alloc2>, const T*, const T*>
  >::type
{ };

template<typename Vec1, typename Vec2>
struct LevenshteinIteration<Vec1, Vec2, std::string::iterator, std::string::iterator> {
static inline void perform(const std::string::iterator& a, const std::string::iterator& b,
  std::size_t& i, std::size_t j, size_t bLen, Vec1& diag, const Vec2& diag2)
{
  LevenshteinIteration<Vec1, Vec2, const char*, const char*>
    ::perform(&*a, &*b, i, j, bLen, diag, diag2);
}
};

template<typename Vec1, typename Vec2>
struct LevenshteinIteration<Vec1, Vec2, std::string::const_iterator, std::string::const_iterator> {
static inline void perform(const std::string::const_iterator& a, const std::string::const_iterator& b,
  std::size_t& i, std::size_t j, size_t bLen, Vec1& diag, const Vec2& diag2)
{
  LevenshteinIteration<Vec1, Vec2, const char*, const char*>
    ::perform(&*a, &*b, i, j, bLen, diag, diag2);
}
};

template<typename T, typename Iterator1, typename Iterator2>
T levenshteinDiagonal(Iterator1 a, Iterator1 aEnd, Iterator2 b, Iterator2 bEnd) {
  const std::size_t aLen = aEnd - a;
  const std::size_t bLen = bEnd - b;
  
  assert(0 < aLen);
  assert(aLen <= bLen);
  
  typedef AlignmentAllocator<T, alignment> Alloc;
  std::vector<T, Alloc> diag  (aLen + 1, T(0));
  std::vector<T, Alloc> diag2 (aLen + 1, T(0));
  
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
      
      LevenshteinIteration<std::vector<T, Alloc>, std::vector<T, Alloc>, Iterator1, Iterator2>
        ::perform(a, b, i, j, bLen, diag, diag2);
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

template<typename String>
std::size_t levenshteinString(const String& a, const String& b) {
  return levenshtein(std::cbegin(a), std::cend(a), std::cbegin(b), std::cend(b));
}
}

/* Testing */
#include "FileMappedString.hpp"
#include <chrono>

template<typename CharT>
void levenshteinStringExpect(const std::string& a, const std::string& b, uint32_t expected) {
  std::basic_string<CharT> a_(std::begin(a), std::end(a));
  std::basic_string<CharT> b_(std::begin(b), std::end(b));
  auto start = std::chrono::high_resolution_clock::now();
  auto distance = levenshteinSSE::levenshteinString(a_, b_);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end-start;

  std::cerr << "A = " << a << "\nB = " << b << "\nT = " << typeid(CharT).name()
            << "\ndistance = " << distance << ", expected = " << expected
            << "\nTime: " << diff.count() << " s\n";
  
  assert (distance == expected);
}

template<typename CharT>
void levenshteinFileExpect(const std::string& a, const std::string& b, uint32_t expected) {
  auto start = std::chrono::high_resolution_clock::now();
  auto distance = levenshteinSSE::levenshteinString(FileMappedString<CharT>(a), FileMappedString<CharT>(b));
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end-start;
  
  std::cerr << "A = " << a << "\nB = " << b << "\nT = " << typeid(CharT).name()
            << "\ndistance = " << distance << ", expected = " << expected
            << "\nTime: " << diff.count() << " s\n";
  
  assert (distance == expected);
}

int main() {
  levenshteinStringExpect<char>("Saturday", "Sunday", 3);
  levenshteinStringExpect<char>("Sitting", "Kittens", 3);
  levenshteinStringExpect<char>("A somewhat longer string", "Here is a maybe even longer string!", 17);
  levenshteinStringExpect<char16_t>("A somewhat longer string", "Here is a maybe even longer string!", 17);
  levenshteinStringExpect<char32_t>("A somewhat longer string", "Here is a maybe even longer string!", 17);
  levenshteinStringExpect<wchar_t>("A somewhat longer string", "Here is a maybe even longer string!", 17);
  levenshteinStringExpect<wchar_t>("Saturday", "Sunday", 3);
  levenshteinStringExpect<wchar_t>("Sitting", "Kittens", 3);
  levenshteinStringExpect<char>("Kittens", "Sitting", 3);
  levenshteinStringExpect<char>("Kitten", "Sitting", 3);
  levenshteinStringExpect<char>("Hallo, Welt!", "Hello, World!", 4);
  levenshteinStringExpect<char>("", "", 0);
  levenshteinStringExpect<char16_t>("", "", 0);
  levenshteinStringExpect<char32_t>("", "", 0);
  levenshteinStringExpect<wchar_t>("", "", 0);
  levenshteinStringExpect<char>("A", "", 1);
  levenshteinStringExpect<char>("A", "A", 0);
  levenshteinStringExpect<char>("A", "Sitting", 7);
  levenshteinStringExpect<char>("", "Sitting", 7);
  levenshteinStringExpect<char>("Sitting", "Sitting", 0);
  levenshteinFileExpect<char>("test/assets/loremipsum_1-16k.utf8", "test/assets/loremipsum_2-16k.utf8", 12453);
  levenshteinFileExpect<short>("test/assets/loremipsum_1-16k.utf16", "test/assets/loremipsum_2-16k.utf16", 12450);
  levenshteinFileExpect<char>("test/assets/loremipsum_1-64k.utf8", "test/assets/loremipsum_2-64k.utf8", 49618);
  levenshteinFileExpect<short>("test/assets/loremipsum_1-64k.utf16", "test/assets/loremipsum_2-64k.utf16", 49607);
  levenshteinFileExpect<char>("test/assets/random64_1",   "test/assets/random64_2",    64);
  levenshteinFileExpect<char>("test/assets/random128_1",  "test/assets/random128_2",  127);
  levenshteinFileExpect<char>("test/assets/random1024_1", "test/assets/random1024_2", 1011);
  levenshteinFileExpect<char>("test/assets/random8192_1", "test/assets/random8192_2", 8074);
  levenshteinFileExpect<std::uint32_t>("test/assets/random1024_1", "test/assets/random1024_2", 256);
  levenshteinFileExpect<std::uint32_t>("test/assets/random8192_1", "test/assets/random8192_2", 2048);
  // levenshteinFileExpect<char>("test/assets/loremipsum_1.utf8", "test/assets/loremipsum_2.utf8", 218919);
  return 0;
}
