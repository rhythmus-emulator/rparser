#ifndef RPARSER_ERROR_H
#define RPARSER_ERROR_H

#define ERR(name,msg) name
namespace rparser {

enum class ERROR
{
#include "Error.list"
};
#undef ERR

#define ERR(name,msg) msg
static const char* get_error_msg(ERROR e)
{
	static const char *ERROR_MSG[] =
  {
#include "Error.list"
  };
	return ERROR_MSG[(unsigned)e];
}

};
#undef ERR

#endif