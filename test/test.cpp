/* Testing */
#include "levenshtein-sse.hpp"
#include "FileMappedString.hpp"
#include <chrono>
#include <list>
#include <deque>
#include <array>
#include <iostream>
#include <typeinfo>

using levenshteinSSE::levenshtein;

template<typename CharT>
void levenshteinStringExpect(const std::string& a, const std::string& b, std::uint32_t expected) {
  std::basic_string<CharT> a_(std::begin(a), std::end(a));
  std::basic_string<CharT> b_(std::begin(b), std::end(b));
  auto start = std::chrono::high_resolution_clock::now();
  auto distance = levenshtein(a_, b_);
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);

  std::cerr << "A = " << a << "\nB = " << b << "\nT = " << typeid(CharT).name()
            << "\ndistance = " << distance << ", expected = " << expected
            << "\nTime: " << diff.count() << " ms\n";
  
  if (distance != expected)
    std::exit(1);
}

template<typename Container>
void levenshteinContainerExpect(const Container& a, const Container& b, std::uint32_t expected) {
  auto start = std::chrono::high_resolution_clock::now();
  auto distance = levenshtein(a, b);
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);

  std::cerr << "a.size() = " << a.size() << "\nb.size() = " << b.size()
            << "\nContainer = " << typeid(Container).name()
            << "\ndistance = " << distance << ", expected = " << expected
            << "\nTime: " << diff.count() << " ms\n";
  
  if (distance != expected)
    std::exit(1);
}

template<typename CharT>
void levenshteinFileExpect(const std::string& a, const std::string& b, std::uint32_t expected) {
  auto start = std::chrono::high_resolution_clock::now();
  auto distance = levenshtein(FileMappedString<CharT>(a), FileMappedString<CharT>(b));
  auto end = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(end-start);
  
  std::cerr << "A = " << a << "\nB = " << b << "\nT = " << typeid(CharT).name()
            << "\ndistance = " << distance << ", expected = " << expected
            << "\nTime: " << diff.count() << " ms\n";
  
  if (distance != expected)
    std::exit(1);
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
  
  levenshteinContainerExpect<std::vector<char>>(
    {'S', 'i', 't', 't', 'i', 'n', 'g'},
    {'K', 'i', 't', 't', 'e', 'n'},
    3);
  
  levenshteinContainerExpect<std::vector<long>>(
    {'S', 'i', 't', 't', 'i', 'n', 'g'},
    {'K', 'i', 't', 't', 'e', 'n'},
    3);
  
  levenshteinContainerExpect<std::vector<std::string>>(
    {"Bananas", "are", "yellow"},
    {"Bananas", "are", "always", "yellow"},
    1);
  
  levenshteinContainerExpect<std::list<std::string>>(
    {"Bananas", "are", "yellow"},
    {"Bananas", "are", "always", "yellow"},
    1);
  
  levenshteinContainerExpect<std::list<int>>(
    {2, 3, 5, 7, 11, 13, 17, 19},
    {1, 3, 5, 7, 9, 11, 13, 15, 17, 19},
    3);
  
  levenshteinContainerExpect<std::deque<int>>(
    {2, 3, 5, 7, 11, 13, 17, 19},
    {1, 3, 5, 7, 9, 11, 13, 15, 17, 19},
    3);
  
  levenshteinContainerExpect<std::array<int, 10>>(
    {2, 3, 5, 7, 11, 13, 17, 19, 23, 29},
    {1, 3, 5, 7, 9, 11, 13, 15, 17, 19},
    5);
  
  levenshteinContainerExpect<std::vector<float>>(
    {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
    {-0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f,
     -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f,
     -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f,
     -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f, -0.0f},
    0);
  
  levenshteinFileExpect<char>("test/assets/loremipsum_1-16k.utf8", "test/assets/loremipsum_2-16k.utf8", 12453);
  levenshteinFileExpect<short>("test/assets/loremipsum_1-16k.utf16", "test/assets/loremipsum_1-16k.utf16", 0);
  levenshteinFileExpect<short>("test/assets/loremipsum_1-16k.utf16", "test/assets/loremipsum_2-16k.utf16", 12450);
  levenshteinFileExpect<char>("test/assets/loremipsum_1-64k.utf8", "test/assets/loremipsum_2-64k.utf8", 49618);
  levenshteinFileExpect<short>("test/assets/loremipsum_1-64k.utf16", "test/assets/loremipsum_1-64k.utf16", 0);
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
