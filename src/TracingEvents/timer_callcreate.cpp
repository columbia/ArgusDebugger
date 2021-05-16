#include "timer_call.hpp"

TimerCreateEvent::TimerCreateEvent(double timestamp, std::string op, uint64_t _tid, uint32_t event_core, 
        void * funcptr, uint64_t p0, uint64_t p1, void * qptr, std::string procname)
:EventBase(timestamp, TMCALL_CREATE_EVENT, op, _tid, event_core, procname),
TimerItem(funcptr, p0, p1, qptr)
{
    is_event_processing = false;
    timercallout_event = nullptr;
    timercancel_event = nullptr;
}

void TimerCreateEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\n\tqueue " << std::hex << get_q_ptr() << std::endl;

    if (timercallout_event)
        outfile << "\n\t" << "called at " << std::fixed << std::setprecision(1) << timercallout_event->get_abstime();
    else
        outfile << "\n\t" << "not called";

    outfile << "\n\tfunc_" << std::hex << get_func_ptr();
    outfile << "(" << std::hex << get_param0() << ",";
    outfile << std::hex << get_param1() << ")" << std::endl;
}

void TimerCreateEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\ttimercallout_func_" << std::hex << get_func_ptr();
    outfile << "(" << std::hex << get_param0() << ",";
    outfile << std::hex << get_param1() << ")";
	outfile << std::hex << q_ptr << std::endl;
}
