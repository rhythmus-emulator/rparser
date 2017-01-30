/* supports bms, bme, bml formats. */

#include "NoteLoaderBMS.h"
#include "Chart.h"
#include "util.h"
#include <vector>
#include <string>
#include <cstdlib>

namespace rparser {

namespace NoteLoaderBMS {

/* processes bms statement command. */
class BMSTree {
    private:
    struct Condition {
        int offset; // active threshold
        bool actived; // even once condition has activated?
        bool active;  // so, current condition is active?
        
        Condition(int val) {
            offset = rand();
            if (val < offset) {
                active = true;
                actived = true;
            } else {
                active = false;
                actived = false;
            }
        }

        void ELSEIF(int val) {
            active = false;
            if (!actived && val < offset) {
                active = true;
                actived = false;
            }
        }
        void ELSE (int val) {
            if (!actived) {
                active = true;
                actived = true;
            }
        }
    }
    std::vector<Condition> conds;

    public:
    bool IsCurrentValidStatement() {
        return (conds.size() == 0 || conds.back().active);
    }

    bool ProcessStatement(const std::string& line) {
        // set seed first
        srand(rparser::GetSeed());

		size_t space = line.find(' ');
		std::string name = line.substr(0, space).lower();
        std::string value = ""
        int isCond = 1;
        if (space != std::string::npos) value = line.substr(space+1);

        if (name == "#endif") {
            // pop out last statement (if exists)
            ASSERT(conds.size());
            conds.pop();
        } else if (name == "#if") {
            // starting of bms command
            conds.push(Condition(atoi(value.c_str())));
        } else if (name == "#else") {
            ASSERT(conds.size());
            conds.back().ELSE();
        } else if (name == "#elseif") {
            ASSERT(conds.size());
            conds.back().ELSEIF(atoi(value.c_str()));
        } else {
            isCond = 0;
        }

        return (isCond == 0 && IsCurrentValidStatement());
    }

    void ProcessBMSCommand( const char* in, int iLen, std::vector<std::string> &out) {
        // clear tree
        conds.clear();

        // read each line
        const char* p_end = in+iLen;
        while (in < p_end) {
            // split current line
            const char* p = in;
            while (p < p_end && *p == '\n') p++;
            // make trimmed string
            const char *s=in, *e=p;
            while (*s == ' ' || *s == '\t') s++;
            while (e > s && (*e == ' ' || *e == '\t')) e--;
            std::string line(s, e-s);
            // prepare for next line
            in = p+1;
            // process line 
            if (ProcessStatement(line)) out.push_back(line);
        }
    }

    // just extracts BMS command, not process it.
    void ExtractBMSCommand( const char* in, int iLen, std::vector<std::string> &out, std::vector &cmd ) {
        // clear tree
        conds.clear();

        // read each line
        const char* p_end = in+iLen;
        while (in < p_end) {
            // split current line
            const char* p = in;
            while (p < p_end && *p == '\n') p++;
            // make trimmed string
            const char *s=in, *e=p;
            while (*s == ' ' || *s == '\t') s++;
            while (e > s && (*e == ' ' || *e == '\t')) e--;
            std::string line(s, e-s);
            std::string line_original(in, in-p+1);
            // prepare for next line
            in = p+1;
            // process line
            int isCommandExists = conds.size();
            ProcessStatement(line);
            isCommandExists |= conds.size();
            if (isCommandExists) cmd += line_original;
            else out.append(line);
        }
    }
}

bool LoadChart( const std::string& fpath, Chart& chart, bool processCmd=true ) {

}

bool ParseChart( const std::string& fpath, Chart& chart, bool processCmd=true ) {

}

bool ParseNoteData( const char* p, int iLen, NoteData& nd, bool processCmd=true ) {

}

bool LoadMetaData( const std::string& fpath, MetaData& md, bool processCmd=true ) {

}

bool ParseMetaData( const char* p, int iLen, MetaData& md, bool processCmd=true ) {

}

bool ParseTimingData( const char* p, int iLen, TimingData& td, bool processCmd=true ) {

}

} /* rparser */

} /* NoteLoaderBMS */