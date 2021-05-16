#include "cadisplay.hpp"
CADisplayEvent::CADisplayEvent(double abstime, std::string _op, uint64_t _tid, uint64_t object, uint32_t _core_id, std::string _procname)
:EventBase(abstime, CA_DISPLAY_EVENT, _op, _tid, _core_id, _procname)
{
    object_addr = object;
    object_set_events.clear();
    is_serial = 0;
}

void CADisplayEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\n\t" << std::hex << object_addr;
}

void CADisplayEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\t" << std::hex << object_addr;
    outfile << "\t" << object_set_events.size() << std::endl;
}
