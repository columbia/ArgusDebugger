#include "mkrunnable.hpp"

FakedWokenEvent::FakedWokenEvent(double timestamp, std::string op, uint64_t tid,
    MakeRunEvent *_mkrun_peer, uint32_t coreid, std::string procname)
:EventBase(timestamp, FAKED_WOKEN_EVENT, op, tid, coreid, procname)
{
    set_event_peer(_mkrun_peer);
}

void FakedWokenEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << std::endl;
}

void FakedWokenEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
	outfile << std::endl;
}
