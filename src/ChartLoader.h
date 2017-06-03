/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_NOTELOADER_H
#define RPARSER_NOTELOADER_H

#include "Song.h"
#include "NoteData.h"
#include <vector>

namespace rparser {

class Chart;

class ChartLoader {
protected:
    Chart* c;
    int error;
    int m_iSeed;
public:
    ChartLoader(Chart* c): c(c), error(0), m_iSeed(-1) {};
    void SetSeed(int seed);
    virtual bool Test( const char* p, int iLen ) = 0;
    virtual bool TestName( const char *fn ) = 0;
    virtual bool Load( const char* p, int iLen ) = 0;
};


class ChartLoaderBMS : public ChartLoader {
private:
    bool procExpansion;         // should process expand command?
    std::vector<Note> notes;    // parsed note objects

    void ReadHeader(const char* p, int iLen);
    void ReadObjects(const char* p, int iLen);
public:
    ChartLoaderBMS(Chart* c, bool procExpansion=true): ChartLoader(c) {};
    bool Test( const char* p, int iLen );
    bool TestName( const char *fn );
    bool Load( const char* p, int iLen );
};


class ChartLoaderVOS : public ChartLoader {
public:
    ChartLoaderVOS(Chart* c): ChartLoader(c) {};
    bool Test( const char* p, int iLen );
    bool TestName( const char *fn );
    bool Load( const char* p, int iLen );
};


int LoadChart( const std::string& fn, SONGTYPE songtype = SONGTYPE::UNKNOWN );
int LoadChart( const char* p, int iLen, SONGTYPE songtype = SONGTYPE::UNKNOWN );

}

#endif