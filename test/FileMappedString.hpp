#ifndef FLST_FILE_MAPPED_STRING_HPP
#define FLST_FILE_MAPPED_STRING_HPP

#include "ErrnoException.hpp"

#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>

template<typename T>
class FileMappedString {
  public:
    FileMappedString(const std::string& file)
      : base(nullptr)
    {
      int fd = ::open(file.c_str(), O_RDONLY);
      if (fd == -1) {
        throw ErrnoException("open()");
      }
      
      struct ::stat buf;
      if (fstat(fd, &buf) == -1) {
      close_and_throw:
        int err = errno;
        close(fd);
        throw ErrnoException("fstat()", err);
      }
      
      sz = buf.st_size;
      
      void* mem = ::mmap(nullptr, sz, PROT_READ, MAP_PRIVATE, fd, 0);
      if (mem == MAP_FAILED) {
        goto close_and_throw;
      }
      
      base = static_cast<T*>(mem);
    }
    
    ~FileMappedString() {
      if (base) {
        ::munmap(base, sz);
      }
    }
    
    FileMappedString& operator=(const FileMappedString& other) = delete;
    FileMappedString(const FileMappedString& other) = delete;
    FileMappedString& operator=(FileMappedString&& other) {
      base = other.base;
      sz = other.sz;
      other.base = nullptr;
      return *this;
    }
    
    FileMappedString(FileMappedString&& other)
      : base(other.base), sz(other.sz)
    {
      other.base = nullptr;
    }
    
    const T* begin() const { return base; }
    const T* end() const { return base + size(); }
    size_t size() const { return sz/sizeof(T); }
  private:
    T* base;
    size_t sz;
};

#endif
