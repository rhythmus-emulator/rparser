/* supports bms, bme, bml formats. */

#include "NoteLoader.h"
#include "Chart.h"
#include "util.h"
#include <vector>
#include <string>
#include <cstdlib>

namespace rparser {

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
        void ELSE () {
            if (!actived) {
                active = true;
                actived = true;
            }
        }
	};
    std::vector<Condition> conds;

    public:
    bool IsCurrentValidStatement() {
        return (conds.size() == 0 || conds.back().active);
    }

	// process only conditional statement
    bool ProcessStatement(const std::string& line) {
        // set seed first
        srand(rparser::GetSeed());

		size_t space = line.find(' ');
		std::string name = line.substr(0, space); lower(name);
		std::string value = "";
        int isCond = 1;
        if (space != std::string::npos) value = line.substr(space+1);

        if (name == "#endif") {
            // pop out last statement (if exists)
            ASSERT(conds.size());
            conds.pop_back();
        } else if (name == "#if") {
            // starting of bms command
            conds.push_back(Condition(atoi(value.c_str())));
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
    void ExtractBMSCommand( const char* in, int iLen, std::vector<std::string> &out, std::vector<std::string> &cmd ) {
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
            if (isCommandExists) cmd.push_back(line_original);
            else out.push_back(line);
        }
    }
};


// main file
bool ChartLoaderBMS::Test( const char* p, int iLen )
{
    // As bms file has no file signature,
    // just search for "*--" string for first 100 string.
    // TODO: is there any exception?
    for (int i=0; i<iLen-4 && i<100; i++) {
        if (stricmp(p, "*-----", 6)==0)
            return true;
    }
    return false;
}

bool TestName( const char *fn )
{
    std::string fn_lower = fn;  lower(fn_lower);
    return EndsWith(fn_lower, ".bms") ||
        EndsWith(fn_lower, ".bme") ||
        EndsWith(fn_lower, ".bml") ||
        EndsWith(fn_lower, ".pms");
}

bool ChartLoaderBMS::Load( const char* p, int iLen )
{
    // parse metadata first

    // parse BGA channel second

    // parse notedata third

    // parse Expanded command fourth

    return true;
}

} /* rparser */