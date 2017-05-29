/* supports bms, bme, bml formats. */

#include "NoteLoader.h"
#include "Chart.h"
#include "util.h"
#include <vector>
#include <string>
#include <sstream>
#include <cstdlib>

namespace rparser {

/* processes bms statement command. */
class BMSExpandProc {
private:
    Chart *c;
    std::vector<int> active_value_stack;    // used in case of #RANDOM~#ENDRANDOM
    int active_value;
    std::string m_sExpand;      // only extracted expand commands
    std::string m_sProcCmd;     // expand-processed commands

    enum COND_RESULT {
        COND_UNKNOWN = 0,
        COND_NEW = 1,
        COND_ACCEPT = 2,
        COND_END = 3
    };
    class Condition {
        virtual bool Valid() = 0;
        virtual std::string GetType() = 0;
        virtual void Parse(const std::string& cmd_lower, const std::string& value) = 0;
    };
    class IFCondition : public Condition {
        int target;      // active value
        int activecnt;  // condition is activated?
        int activeidx;  // current condition is active?
        int condidx;    // current condition index
        IFCondition() : 
            condidx(0), activecnt(0), activeidx(-1), target(active_value) {}

        static int Test(const std::string& name) {
            if (name == "#endif" || name == "#end") {
                return COND_END;
            } else if (name == "#if") {
                return COND_NEW;
            } else if (name == "#else") {
                return COND_ACCEPT;
            } else if (name == "#elseif") {
                return COND_ACCEPT;
            }
            return COND_UNKNOWN;
        }
        std::string GetType() { return "if"; }
        void Parse(const std::string& name, const std::string& value) {
            if (name == "#endif" || name == "#end") {
                // #ENDIF cannot be shown first
                ASSERT(condidx > 0);
            } else if (name == "#if") {
                // #IF clause cannot be shown twice or later
                ASSERT(condidx <= 0);
                condidx++;
                if (target == atoi(value.c_str())) {
                    activeidx = condidx;
                    activecnt++;
                }
            } else if (name == "#else") {
                condidx++;
                if (activecnt <= 0) {
                    activeidx = condidx;
                    activecnt++;
                }
            } else if (name == "#elseif") {
                condidx++;
                if (activecnt <= 0 && target == atoi(value.c_str())) {
                    activeidx = condidx;
                    activecnt++;
                }
            }
        }
        bool Valid() {
            return activeidx==condidx;
        }
	};
    class SWITCHCondition : public Condition {
        int target;      // active value
        int stat;       // -1: skipped, 0: not activated, 1~: activated
        SWITCHCondition() : 
            stat(0), value(active_value) {}

        static int Test(const std::string& name) {
            if (name == "#endsw") {
                return COND_END;
            } else if (name == "#switch" || name == "#setswitch") {
                return COND_NEW;
            } else if (name == "#case" || name == "#skip" || name == "#def") {
                return COND_ACCEPT;
            }
            return COND_UNKNOWN;
        }
        std::string GetType() { return "switch"; }
        void Parse(const std::string& name, const std::string& value) {
            if (name == "#endsw") {
                // #ENDIF cannot be shown first
                ASSERT(condidx > 0);
            } else if (name == "#switch" || name == "#setswitch") {
                // initialize random value
                value = atoi(value.c_str()) * rand();
            } else if (name == "#case") {
                if (atoi(value.c_str()) == target && stat >= 0)
                    stat++;
            } else if (name == "#skip") {
                stat = -1;
            } else if (name == "#def") {
                if (stat >= 0)
                    stat++;
            }
        }
        bool Valid() {
            return stat > 0;
        }
	};

    std::vector<Condition*> conds;
public:
    int m_iSeed;
    bool m_bAllowExpand;
    BMSExpandProc(Chart *c, int iSeed=-1, bAllowExpand=true):
        c(c), m_iSeed(iSeed), m_bAllowExpand(bAllowExpand), active_value(0) {}

    bool IsCurrentValidStatement() {
        return (conds.size() == 0 || (m_bAllowExpand && conds.back().Valid()));
    }

	// process only conditional statement
    bool ProcessStatement(const std::string& line) {
        // get command
		size_t space = line.find(' ');
		std::string name = line.substr(0, space); lower(name);
		std::string value = "";
        if (space != std::string::npos) value = line.substr(space+1);

        // match command
        int isCond = 1;
        int cond_test;
        Condition* curr_cond;
        if (name == "#random" || name == "#rondom") {
            active_value_stack.push_back(active_value);
            active_value = atoi(value.c_str()) * rand();
        } else if (name == "#endrandom") {
            ASSERT(active_value_stack.size() > 0);
            active_value = active_value_stack.back();
            active_value_stack.pop_back();
        } else if ((cond_test = IFCondition::Test(name)) != COND_UNKNOWN) {
            switch (cond_test) {
            case COND_NEW:
                curr_cond = new IFCondition()
                curr_cond->Parse(name, value);
                cond.push_back(curr_cond);
                break;
            case COND_ACCEPT:
                ASSERT(cond.size() > 0);
                curr_cond = cond.back();
                ASSERT(curr_cond->GetType() == "if");
                curr_cond->Parse(name, value);
                break;
            case COND_END:
                ASSERT(cond.size() > 0);
                curr_cond = cond.back();
                ASSERT(curr_cond->GetType() == "if");
                curr_cond->Parse(name, value);
                cond.pop_back();
                delete curr_cond;
                break;
            }
        } else if ((cond_test = SWITCHCondition::Test(name)) != COND_UNKNOWN) {
            switch (cond_test) {
            case COND_NEW:
                curr_cond = new SWITCHCondition()
                curr_cond->Parse(name, value);
                cond.push_back(curr_cond);
                break;
            case COND_ACCEPT:
                ASSERT(cond.size() > 0);
                curr_cond = cond.back();
                ASSERT(curr_cond->GetType() == "switch");
                curr_cond->Parse(name, value);
                break;
            case COND_END:
                ASSERT(cond.size() > 0);
                curr_cond = cond.back();
                ASSERT(curr_cond->GetType() == "switch");
                curr_cond->Parse(name, value);
                cond.pop_back();
                delete curr_cond;
                break;
            }
        } else {
            isCond = 0;
        }
        // if condition size is over zero before and after,
        // then it's expand command.
        int is_curr_expand = isCond || cond.size();
        if (is_curr_expand) {
            m_sExpand += line;
        }
        // check processed statement
        if (isCond == 0 && IsCurrentValidStatement()) {
            m_sProcCmd += line;
        }
    }

    void proc( const char* in, int iLen ) {
        // clear tree
        conds.clear();

        // set seed first
        if (m_iSeed >= 0)
            srand(m_iSeed);

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
            ProcessStatement(line);
        }
    }

    std::string& GetProcCmd() { return m_sProcCmd; }
    std::string& GetExpandCmd() { return m_sExpandCmd; }

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
    // parse expand, and process it first
    // (expand parse effects to Metadata / Channels)
    BMSExpandProc bExProc(c, m_iSeed, procExpansion);
    bExProc.proc(p, iLen);
    std::string& proc_res = bExProc.GetProcCmd();
    c->GetMetaData().m_sExpand = bExProc.GetExpandCmd();

    // parse metadata(include expanded command) first
    ReadHeader(proc_res.c_str(), proc_res.size());

    // parse BGA & notedata channel second
    ReadChannels(proc_res.c_str(), proc_res.size());

    return true;
}

void ChartLoaderBMS::ReadHeader(const char* p, int iLen)
{
    dd
}

void ChartLoaderBMS::ReadChannels(const char* p, int iLen)
{

}

} /* rparser */