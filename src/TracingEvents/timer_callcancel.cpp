#include "timer_call.hpp"

TimerCancelEvent::TimerCancelEvent(double timestamp, std::string op, uint64_t _tid, uint32_t event_core,
                        void * funcptr, uint64_t p0, uint64_t p1, void * qptr, std::string procname)
:EventBase(timestamp, TMCALL_CANCEL_EVENT, op, _tid, event_core, procname),
TimerItem(funcptr, p0, p1, qptr)
{
    create_event = nullptr;
}

bool TimerCancelEvent::check_root(TimerCreateEvent * event)
{
    if (get_func_ptr() != event->get_func_ptr()
        || get_param0() != event->get_param0()
        || get_param1() != event->get_param1())
        //|| get_q_ptr() != event->get_q_ptr())
        return false;

    if (event->check_cancel() == true) {
        std::cerr << "Error:\n";
        std::cerr << "timercallout cancel at " << std::fixed << std::setprecision(1) << get_abstime() << std::endl;
        std::cerr << "try to cancel timer created at" << std::fixed << std::setprecision(1) << event->get_abstime() << std::endl;
        std::cerr << "\t cancelled at " << std::fixed << std::setprecision(1) << event->get_cancel_peer()->get_abstime() << std::endl;
    }
    return true;
}

void TimerCancelEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\n\tqueue " << std::hex << get_q_ptr() << std::endl;
    outfile << "\n\tfunc_" << std::hex << get_func_ptr();
    outfile << "(" << std::hex << get_param0() << ",";
    outfile << std::hex << get_param1() << ")" << std::endl;

    if (create_event) {
        outfile << "\n\tcreator: ";
        outfile << std::fixed << std::setprecision(1) << create_event->get_abstime();
    }
}

void TimerCancelEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\ttimercancel_func_" << std::hex << get_func_ptr()\
		<< "(" << std::hex << get_param0() << ","\
    	<< std::hex << get_param1() << ")" << std::hex << q_ptr << std::endl;
}
