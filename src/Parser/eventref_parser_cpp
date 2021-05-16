#include "parser.hpp"
#include "eventref.hpp"

namespace Parse
{
    EventRefParser::EventRefParser(std::string filename)
    :Parser(filename)
    {}

    void EventRefParser::process()
    {
        std::string line, deltatime, opname, procname;
        double abstime;
        uint64_t event_addr, event_class, event_kind, keycode, tid, coreid;

        while (getline(infile, line)) {
            std::istringstream iss(line);

            if (!(iss >> abstime >> deltatime >> opname) ||
                !(iss >> std::hex >> event_addr >> event_class >> event_kind >> keycode) ||
                !(iss >> tid >> coreid)) {
                outfile << line << std::endl;
                continue;
            }

            if (!getline(iss >> std::ws, procname) || !procname.size())
                procname = ""; 

            CoreGraphicsRefEvent *event_ref_event = new CoreGraphicsRefEvent(abstime, opname, tid, event_addr, event_class, event_kind, keycode, coreid, procname);
            if (!event_ref_event) {
                std::cerr << "OOM " << __func__ << std::endl;
                exit(EXIT_FAILURE);
            }
            
            event_ref_event->set_complete();
            local_event_list.push_back(event_ref_event);
        }
    } 
}
