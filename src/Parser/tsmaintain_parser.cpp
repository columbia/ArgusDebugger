#include "parser.hpp"
#include "tsmaintenance.hpp"

namespace Parse
{
    TsmaintainParser::TsmaintainParser(std::string filename)
    :Parser(filename)
    {}
    
    void TsmaintainParser::process()
    {
        std::string line, deltatime, opname, procname;
        double abstime;
        uint64_t unused, tid, coreid;

        while (getline(infile, line)) {
            std::istringstream iss(line);

            if (!(iss >> abstime >> deltatime >> opname) ||
                !(iss >> std::hex >> unused >> unused >> unused >> unused) ||
                !(iss >> tid >> coreid)) {
                outfile << line << std::endl;
                continue;
            }

            if (!getline(iss >> std::ws, procname) || !procname.size())
                procname = ""; 

            TimeshareMaintenanceEvent *tsm_event = new TimeshareMaintenanceEvent(abstime, opname,
                tid, coreid, procname);
            if (!tsm_event) {
                std::cerr << "OOM " << __func__ << std::endl;
                exit(EXIT_FAILURE);
            }

            tsm_event->set_complete();
            local_event_list.push_back(tsm_event);
        }
    }
}
