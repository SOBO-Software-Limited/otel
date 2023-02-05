#ifndef BOLLOX
#define BOLLOX

#include <string>
#include <boost/lexical_cast.hpp>



#define LOG_GENERAL(level, msg) \
  { std::cout << "level:" << level << ":" << msg; }

#endif