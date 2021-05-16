#include "base.hpp"
#include <math.h>

EventBase::EventBase(double _timestamp, int _event_type, std::string _op, uint64_t _tid, uint32_t _core_id, std::string _procname)
:ProcessInfo(_tid, _procname),
TimeInfo(_timestamp),
EventType(_event_type, _op)
{
	assert(event_type > 0 && event_type <= 28);
    core_id = _core_id;
    group_id = -1;
    complete = false;
    event_prev = nullptr;
    event_peer = nullptr;
	propagated_frames.clear();
}


EventBase::EventBase(EventBase *base)
:ProcessInfo(base->get_tid(), base->get_procname()),
TimeInfo(base->get_abstime()),
EventType(base->get_event_type(), base->get_op())
{
    *this = *base;
}

void EventBase::decode_event(bool is_verbose, std::ofstream &outfile) 
{
    outfile << "\n*****" << std::endl;
    outfile << "\n group_id = " << std::right << std::hex << get_group_id();
    outfile << "\n [" << std::dec << get_pid() <<"] " << get_procname() << "(" << std::hex << get_tid() << ")" << get_coreid();
    outfile << "\n\t" << get_op();
    outfile << "\n\t" << std::fixed << std::setprecision(1) << get_abstime();
    outfile << std::endl;
}

void EventBase::streamout_event(std::ofstream &outfile)
{

    outfile << "0x" << std::right << std::hex << get_group_id();
    outfile << "\t" << std::fixed << std::setprecision(1) << get_abstime();
    outfile << "\t0x" << std::hex << get_tid();
	outfile << "\t" << std::dec << get_pid();
    outfile << "\t" << get_procname();
    outfile << "\t" << get_op();

    if (event_peer != nullptr)
     	outfile << " <- / -> \ttid = 0x" << std::hex << event_peer->get_tid()\
        	<< "\t" << event_peer->get_procname()\
    		<< "\t at " << std::fixed << std::setprecision(1) << event_peer->get_abstime();
}

void EventBase::streamout_event(std::ostream &out)
{
    out << "0x" << std::right << std::hex << get_group_id();
    out << "\t" << std::fixed << std::setprecision(1) << get_abstime();
    out << "\t" << std::hex << get_tid();
	out << "\t" << std::dec << get_pid();
    out << "\t" << get_procname();
    out << "\t" << get_op();
    if (event_peer != nullptr)
		out << "\t 0x" << std::hex << event_peer->get_group_id()\
     		<< "\t 0x" << std::hex << event_peer->get_tid()\
			<< "\t" << std::dec << event_peer->get_pid()\
        	<< "\t" << event_peer->get_procname()\
    		<< "\t" << std::fixed << std::setprecision(1) << event_peer->get_abstime();
}

std::string EventBase::replace_blank(std::string s)
{
	std::string ret(s);
	assert(s.size() == ret.size());
	std::replace(ret.begin(), ret.end(), ' ', '#');
	return ret;
}
