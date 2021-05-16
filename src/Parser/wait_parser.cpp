#include "parser.hpp"
#include "wait.hpp"

namespace Parse
{
    WaitParser::WaitParser(std::string filename)
    :Parser(filename)
    {
        wait_events.clear();
    }
    
    bool WaitParser::set_info(WaitEvent *wait_event, uint64_t pid,
        uint64_t deadline, uint64_t wait_result)
    {
        wait_event->set_pid(lo(pid));
        wait_event->set_wait_result(wait_result);
        return true;
    }

    bool WaitParser::finish_wait_event(std::string opname, double abstime,
        std::istringstream &iss)
    {
        uint64_t wait_event, pid, deadline, wait_result, tid, coreid;
        if (!(iss >> std::hex >> wait_event >> pid >> deadline >> wait_result \
            >> tid >> coreid))
            return false;
    
            std::string procname;

            if (!getline(iss >> std::ws, procname) || !procname.size())
                procname = "";

            WaitEvent *wait_evt = new WaitEvent(abstime, opname, tid,
                wait_event, (uint32_t)coreid, procname);

            if (!wait_evt) {
                std::cerr << "OOM " << __func__ << std::endl;
                exit(EXIT_FAILURE);
            }

            set_info(wait_evt, pid, deadline, wait_result);
            wait_evt->set_complete();
            local_event_list.push_back(wait_evt);
            return true;
    }


    void WaitParser::process()
    {
        std::string line, deltatime, opname;
        double abstime;
        bool ret = false;

        while(getline(infile, line)) {
            std::istringstream iss(line);

            if (!(iss >> abstime >> deltatime >> opname)) {
                outfile << line << std::endl;
                continue;
            }

            ret = false;

            if (opname == "ARGUS_WAIT") {
                ret = finish_wait_event(opname, abstime, iss);
            } else {
                outfile << "Error: unkown opname" << std::endl;
            }

            if (ret == false)
                outfile << line << std::endl;
        }
    }
}
