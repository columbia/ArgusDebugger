#include "parser.hpp"
#include "caset.hpp"
#include "cadisplay.hpp"

namespace Parse
{
    CAParser::CAParser(std::string filename)
    :Parser(filename)
    {}

    bool CAParser::process_caset(std::string opname, double abstime, std::istringstream &iss)
    {
        uint64_t object, unused, tid, coreid;
        std::string procname;

        if (!(iss >> std::hex >> object >> unused >> unused >> unused) ||
            !(iss >> tid >> coreid)) {
            return false;
        }

        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = ""; 
        
        CASetEvent * ca_set_event = new CASetEvent(abstime, opname, tid, object, coreid, procname);
        if (!ca_set_event) {
            std::cerr << "OOM " << __func__ << std::endl;
            exit(EXIT_FAILURE);
        }
        ca_set_event->set_complete();
        local_event_list.push_back(ca_set_event);
        return true;
    }

    bool CAParser::process_cadisplay(std::string opname, double abstime, std::istringstream &iss)
    {
        uint64_t object, serial, unused, tid, coreid;
        std::string procname;
        if (!(iss >> std::hex >> object >> serial >> unused >> unused) ||
            !(iss >> tid >> coreid)) {
            return false;
        }
        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = ""; 

        CADisplayEvent *ca_display_event = new CADisplayEvent(abstime, opname, tid, object, coreid, procname);
        if (!ca_display_event) {
            std::cerr << "OOM " << __func__ << std::endl;
            exit(EXIT_FAILURE);
        }
        outfile << "serial " << std::hex << serial << std::endl;
        ca_display_event->set_serial(serial);
        outfile << "is_serial " << std::hex << ca_display_event->get_serial() << std::endl;
        ca_display_event->set_complete();
        local_event_list.push_back(ca_display_event);
        return true;
    }
    
    void CAParser::process()
    {
        std::string line, deltatime, opname;
        double abstime;
        bool ret;

        while (getline(infile, line)) {
            std::istringstream iss(line);
            
            if (!(iss >> abstime >> deltatime >> opname)) {
                outfile << line << std::endl;
                continue;
            }

            switch (LoadData::map_op_code(0, opname)) {
                case CA_SET:
                    ret = process_caset(opname, abstime, iss);
                    break;
                case CA_DISPLAY:
                    ret = process_cadisplay(opname, abstime, iss);
                    break;
                default:
                    ret = false;    
                    break;
            }
            if (ret == false)
                outfile << line << std::endl;    
        }
        // check events
    }
}
