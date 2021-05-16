#include "wait.hpp"

WaitEvent::WaitEvent(double timestamp, std::string op, uint64_t tid, uint64_t _wait_resource, uint32_t coreid, std::string procname)
:EventBase(timestamp, WAIT_EVENT, op, tid, coreid, procname)
{
    wait_resource = _wait_resource;
    syscall_event = nullptr;
	wait_resource_desc = "";
}

double WaitEvent::get_time_cost(void)
{
    EventBase *mkrun_event = get_event_peer();
    if (mkrun_event != nullptr)
        return mkrun_event->get_abstime() - get_abstime();
    if (syscall_event != nullptr && syscall_event->get_ret_time() > 0.0)
        return syscall_event->get_ret_time() - get_abstime();
	if (LoadData::meta_data.last_event_timestamp > 0)
		return LoadData::meta_data.last_event_timestamp - get_abstime();
    return 0.0;
}

const char * WaitEvent::decode_wait_result(void) 
{
    switch(wait_result) {
        case THREAD_WAITING:
            return "waiting";
        case THREAD_AWAKENED:
            return "awakened";
        case THREAD_TIMED_OUT:
            return "timed_out";
        case THREAD_INTERRUPTED:
            return "interrupted";
        case THREAD_RESTART:
            return "restart";
        case THREAD_NOT_WAITING:
            return "not_waiting";
        default:
            return "unknown";
    }
}

void WaitEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    const char *wait_result_name = decode_wait_result();
    outfile << "\n\t" << std::hex << wait_resource;
    outfile << "\n\t" << wait_result_name;

    if (wait_resource_desc.size())
        outfile << "\n\t" << wait_resource_desc;
    outfile << std::endl;
}

void WaitEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    const char *wait_result_name = decode_wait_result();
    outfile << "\twait_" << wait_resource_desc;
    outfile << "\t" << wait_result_name;
    if (syscall_event) {
        outfile << "\twait from " << std::fixed << std::setprecision(1)\
			 << syscall_event->get_abstime();
    }
    outfile << "\ttime cost " << std::fixed << std::setprecision(1) \
			<< get_time_cost() << std::endl;
    outfile << std::endl;
}
