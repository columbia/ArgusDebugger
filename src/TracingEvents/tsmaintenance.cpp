#include "base.hpp"
#include "tsmaintenance.hpp"

TimeshareMaintenanceEvent::TimeshareMaintenanceEvent(double abstime, std::string op, uint64_t tid, uint32_t event_core, std::string procname)
:EventBase(abstime, TSM_EVENT, op, tid, event_core, procname)
{
    preempted_group_ptr = nullptr;
}

void * TimeshareMaintenanceEvent:: load_gptr(void)
{
    void * ret = preempted_group_ptr;
    preempted_group_ptr = nullptr;
    return ret;
}

void TimeshareMaintenanceEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
}

void TimeshareMaintenanceEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\t" << "timeshared_maintenance" << std::endl;
}
