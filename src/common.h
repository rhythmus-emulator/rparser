#ifndef RPARSER_HEADER_COMMON
#define RPARSER_HEADER_COMMON

/**
 * @warn this file must included at source file, not header. 
 */

#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef _DEBUG
# define ASSERT(x) assert(x)
#else
# define ASSERT(x)
#endif

#include <exception>

namespace rparser
{
  class RparserException : public std::exception
  {
    using std::exception::exception;
  };
}

#define RPARSER_LOG(x) std::cerr << x << std::endl;
#define RPARSER_ASSERT(x, m) throw rparser::RparserException(m)

#endif
