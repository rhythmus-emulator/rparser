#ifndef RPARSER_ERROR_H
#define RPARSER_ERROR_H

namespace rparser {

enum class ERROR
{
#define ERR(name,msg) name
#include "Error.list"
#undef ERR
};

static const char* get_error_msg(ERROR e)
{
	static const char *ERROR_MSG[] =
  {
#define ERR(name,msg) msg
#include "Error.list"
#undef ERR
  };
	return ERROR_MSG[(unsigned)e];
}

};

#endif