#include "caset.hpp"
CASetEvent::CASetEvent(double abstime, std::string _op, uint64_t _tid, uint64_t object, uint32_t _core_id, std::string _procname)
:EventBase(abstime, CA_SET_EVENT, _op, _tid, _core_id, _procname)
{
    object_addr = object;
    display = nullptr;
}

void CASetEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\n\tobject_addr " << std::hex << object_addr << std::endl;
    if (display)
        outfile << "\n\tdisplay at " << std::fixed << std::setprecision(1) << display->get_abstime() << std::endl;
}

void CASetEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\t" << std::hex << object_addr;
    if (display)
        outfile << "\t" << std::fixed << std::setprecision(1) << display->get_abstime();
    outfile << std::endl;
}
