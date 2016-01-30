#ifndef ALIGNMENT_ALLOCATOR_H
#define ALIGNMENT_ALLOCATOR_H

// based on https://stackoverflow.com/a/8545389/688904

#ifdef __SSE__
#include <xmmintrin.h>
#endif

#include <memory>

namespace levenshteinSSE {
template <typename T, std::size_t N = 16>
class AlignmentAllocator {
public:
  typedef T value_type;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  typedef T* pointer;
  typedef const T* const_pointer;

  typedef T& reference;
  typedef const T& const_reference;

  public:
  inline AlignmentAllocator () throw () { }

  template <typename T2>
  inline AlignmentAllocator (const AlignmentAllocator<T2, N> &) throw () { }

  inline ~AlignmentAllocator () throw () { }

  inline pointer address (reference r) {
    return &r;
  }

  inline const_pointer address (const_reference r) const {
    return &r;
  }

  inline pointer allocate (size_type n) {
    // this allocator is special in that it leaves 4*N bytes before and after
    // the allocated area for garbage reads/writes
    return reinterpret_cast<pointer>(reinterpret_cast<char*>(_mm_malloc(n * sizeof(value_type) + 8 * N, N)) + 4*N);
  }

  inline void deallocate (pointer p, size_type) {
    _mm_free(reinterpret_cast<char*>(p) - 4*N);
  }

  inline void construct (pointer p, const value_type& value) {
     new (p) value_type (value);
  }

  inline void destroy (pointer p) {
    p->~value_type ();
  }

  inline size_type max_size () const throw () {
    return size_type (-1) / sizeof (value_type);
  }

  template <typename T2>
  struct rebind {
    typedef AlignmentAllocator<T2, N> other;
  };

  bool operator!=(const AlignmentAllocator<T,N>& other) const  {
    return !(*this == other);
  }

  // Returns true if and only if storage allocated from *this
  // can be deallocated from other, and vice versa.
  // Always returns true for stateless allocators.
  bool operator==(const AlignmentAllocator<T,N>& other) const {
    return other.usesMMAlloc;
  }
  
  static constexpr bool usesMMAlloc = true;
};

template <typename T>
class AlignmentAllocator<T, 1> : public std::allocator<T> {
public:
  static constexpr bool usesMMAlloc = false;
};
}

#endif
