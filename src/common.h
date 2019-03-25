#ifndef RPARSER_HEADER_COMMON
#define RPARSER_HEADER_COMMON

#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#ifdef _DEBUG
# define ASSERT(x) assert(x)
#else
# define ASSERT(x)
#endif

#define RPARSER_LOG(x) {}

#endif
