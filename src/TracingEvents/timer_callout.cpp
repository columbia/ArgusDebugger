#include "timer_call.hpp"
TimerCalloutEvent::TimerCalloutEvent(double timestamp, std::string op, uint64_t _tid, uint32_t event_core, 
    void * funcptr, uint64_t p0, uint64_t p1, void * qptr, std::string procname)
:EventBase(timestamp, TMCALL_CALLOUT_EVENT, op, _tid, event_core, procname),
TimerItem(funcptr, p0, p1, qptr)
{
    create_event = nullptr;
}

bool TimerCalloutEvent::check_root(TimerCreateEvent * event)
{
    if (get_func_ptr() != event->get_func_ptr()
        || get_param0() != event->get_param0()
        || get_param1() != event->get_param1())
        return false;
    

    if (event->check_called() == true) {
        bool ret = false;
        if (event->get_called_peer()->get_abstime() < get_abstime()) {
            LOG_S(INFO) << "Error: timercallout at " << std::fixed << std::setprecision(1) << get_abstime();
            LOG_S(INFO) << "try to match called create " << std::fixed << std::setprecision(1) << event->get_abstime();
            LOG_S(INFO) << "calleded at " << std::fixed << std::setprecision(1) << event->get_called_peer()->get_abstime();
        } else {
            event->unset_called_peer();
            ret = true;
        }
        
        return ret;
    }
    
    if (event->check_cancel() == true) {
        bool ret = false;
        if (event->get_cancel_peer()->get_abstime() < get_abstime()) {
            LOG_S(INFO) << "Error: timercallout at " << std::fixed << std::setprecision(1) << get_abstime() ;
            LOG_S(INFO) << "try to match cancelled create " << std::fixed << std::setprecision(1) << event->get_abstime() ;
            LOG_S(INFO) << "canceled at " << std::fixed << std::setprecision(1) << event->get_cancel_peer()->get_abstime() ;
        } else {
            event->unset_cancel_call();
            ret = true;
        }
        return ret;
    }
    return true;
}

void TimerCalloutEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\n\tqueue" << std::hex << get_q_ptr() << std::endl;
    outfile << "\n\tfunc_" << std::hex << get_func_ptr() << "(" << std::hex << get_param0() << ",";
    outfile << std::hex << get_param1() << ")" << std::endl;
    if (create_event != nullptr) {
        outfile << "\n\tget creator: ";
        outfile << "\n\t" << std::fixed << std::setprecision(1) << create_event->get_abstime();
    }
}

void TimerCalloutEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\ttimercallout_func" << std::hex << get_func_ptr()\
		<< "(" << std::hex << get_param0() << ","\
    	<< std::hex << get_param1() << ")" << std::hex << q_ptr << std::endl;
}
