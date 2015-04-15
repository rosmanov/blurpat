#include <cstdarg>
#include <stdexcept>
#include <string>
#include <sstream>
#include "exceptions.hxx"

using std::ostringstream;
using std::string;


ErrorException::ErrorException(const char* format, ...)
  : std::runtime_error("")
{
  if (format) {
    va_list args;
    va_start(args, format);
    char message[1024];
    int message_len = vsnprintf(message, sizeof(message), format, args);
    mMsg = string(message, message_len);
    va_end(args);
  }
}

InvalidCliArgException::InvalidCliArgException(const char* format, ...)
{
  if (format) {
    va_list args;
    va_start(args, format);
    char message[1024];
    int message_len = vsnprintf(message, sizeof(message), format, args);
    mMsg = string(message, message_len);
    va_end(args);
  }
}

// vim: et ts=2 sts=2 sw=2
