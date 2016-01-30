#ifndef FLST_ERRNO_EXCEPTION
#define FLST_ERRNO_EXCEPTION

#include <exception>
#include <cstring>
#include <cerrno>
#include <string>

class ErrnoException : public std::exception {
  public:
    ErrnoException(const std::string& call, int _errno = 0)
      : info(call)
    {
      info += ": " +
      info += std::strerror(_errno ? _errno : errno);
    }
    
    virtual const char* what() const noexcept {
      return info.c_str();
    }
  private:
    std::string info;
};

#endif
