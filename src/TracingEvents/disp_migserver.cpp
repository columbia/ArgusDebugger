#include "dispatch.hpp"

DispatchQueueMIGServiceEvent::DispatchQueueMIGServiceEvent(double abstime, std::string op, uint64_t tid, uint32_t core_id, std::string procname)
:EventBase(abstime, DISP_MIG_EVENT, op, tid, core_id, procname)
{
}
void DispatchQueueMIGServiceEvent::save_owner(void *_owner)
{
    owner = _owner;
}

void *DispatchQueueMIGServiceEvent::restore_owner(void)
{
    return owner;
}

void DispatchQueueMIGServiceEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << std::endl;
}

void DispatchQueueMIGServiceEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << std::endl;
}
