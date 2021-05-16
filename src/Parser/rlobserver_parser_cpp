#include "parser.hpp"
#include "rlobserver.hpp"

namespace Parse
{
    RLObserverParser::RLObserverParser(std::string filename)
    :Parser(filename)
    {}

    void RLObserverParser::process()
    {
        std::string line, deltatime, opname, procname;
        double abstime;
        uint64_t rl, mode, stage, unused, tid, coreid;

        while (getline(infile, line)) {
            std::istringstream iss(line);

            if (!(iss >> abstime >> deltatime >> opname) ||
                !(iss >> std::hex >> rl >> mode >> stage >> unused) ||
                !(iss >> tid >> coreid)) {
                outfile << line << std::endl;
                continue;
            }

            if (!getline(iss >> std::ws, procname) || !procname.size())
                procname = ""; 

            RunLoopObserverEvent *rl_observer_event = new RunLoopObserverEvent(abstime, opname, tid, rl, mode, stage, coreid, procname);
            if (!rl_observer_event) {
                std::cerr << "OOM " << __func__ << std::endl;
                exit(EXIT_FAILURE);
            }
            
            rl_observer_event->set_complete();
            local_event_list.push_back(rl_observer_event);
        }
    } 
}
