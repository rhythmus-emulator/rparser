/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_NOTELOADER_H
#define RPARSER_NOTELOADER_H

#include "Song.h"

namespace rparser {

class Chart;

class ChartLoader {
protected:
    Chart* c;
    int error;
public:
    ChartLoader(Chart* c): c(c), error(0) {};
    virtual bool Test( const char* p, int iLen ) = 0;
    virtual bool Load( const char* p, int iLen ) = 0;
};


class ChartLoaderBMS : public ChartLoader {
public:
    ChartLoaderBMS(Chart* c): ChartLoader(c) {};
    virtual bool Test( const char* p, int iLen );
    virtual bool Load( const char* p, int iLen );
};


class ChartLoaderVOS : public ChartLoader {
public:
    ChartLoaderVOS(Chart* c): ChartLoader(c) {};
    virtual bool Test( const char* p, int iLen );
    virtual bool Load( const char* p, int iLen );
};


int LoadChart( const std::string& fn, SONGTYPE songtype = SONGTYPE::UNKNOWN );
int LoadChart( const char* p, int iLen, SONGTYPE songtype = SONGTYPE::UNKNOWN );

}

#endif