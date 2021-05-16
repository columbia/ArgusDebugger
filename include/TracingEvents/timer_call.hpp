#ifndef TIMER_CALLOUT_HPP
#define TIMER_CALLOUT_HPP

#include "base.hpp"

class TimerItem {
protected:
    void *func_ptr;
    uint64_t param0;
    uint64_t param1;
    void *q_ptr;

public:
    TimerItem(void *_func_ptr, uint64_t _param0, uint64_t _param1, void *_q_ptr)
        :func_ptr(_func_ptr), param0(_param0), param1(_param1), q_ptr(_q_ptr){}

    void *get_func_ptr(void) {return func_ptr;}
    void *get_q_ptr(void) {return q_ptr;}
    uint64_t get_param0(void) {return param0;}
    uint64_t get_param1(void) {return param1;}
};

class TimerCalloutEvent :public EventBase, public TimerItem {
    TimerCreateEvent *create_event;
public:
    TimerCalloutEvent(double abstime, std::string op, uint64_t _tid, uint32_t event_core,
		void *func, uint64_t param0, uint64_t param1, void *qptr, std::string proc= "");

    void set_timercreate(TimerCreateEvent *call_create)
	{
        create_event = call_create;
        set_event_peer((EventBase *)create_event);
    }
    void unset_timercreate(void) {create_event = nullptr;}
    TimerCreateEvent *get_timercreate(void) {return create_event;}
    bool check_root(TimerCreateEvent *);
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};

class TimerCancelEvent : public EventBase, public TimerItem {
    TimerCreateEvent *create_event;
public:
    TimerCancelEvent(double abstime, std::string op, uint64_t _tid, uint32_t event_core,
    	void *func, uint64_t param0, uint64_t param1, void *qptr, std::string proc= "");
    void set_timercreate(TimerCreateEvent *_create_event) {
        create_event = _create_event;
        set_event_peer((EventBase *)create_event);
    }
    void unset_timercreate(void) {create_event = nullptr;}
    TimerCreateEvent *get_timercreate(void) {return create_event;}
    bool check_root(TimerCreateEvent *);
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};

class TimerCreateEvent : public EventBase, public TimerItem {
    bool is_event_processing;
    std::string func_symbol;
    TimerCancelEvent *timercancel_event;
    TimerCalloutEvent *timercallout_event;
public:
    TimerCreateEvent(double abstime, std::string op, uint64_t _tid, uint32_t event_core,
        void *func, uint64_t param0, uint64_t param1, void *qptr, std::string proc= "");

    void set_func_symbol(std::string symbol) {func_symbol = symbol;}

    void set_called_peer(TimerCalloutEvent *timercallout) {
        timercallout_event = timercallout;
        set_event_peer((EventBase *)timercallout);
    }

    void unset_called_peer(void) {
        if (timercallout_event != nullptr)
            timercallout_event->unset_timercreate();
        timercallout_event = nullptr;
    }

    bool check_called(void) {return timercallout_event != nullptr;}
    TimerCalloutEvent *get_called_peer(void) {return timercallout_event;}

    void cancel_call(TimerCancelEvent *timercancel) {
        timercancel_event = timercancel;
        if (get_event_peer() == nullptr)
            set_event_peer((EventBase *)timercancel);
    }

    void unset_cancel_call(void) {
        if (timercancel_event != nullptr)
            timercancel_event->unset_timercreate();
        timercancel_event = nullptr;
    }

    TimerCancelEvent *get_cancel_peer(void) {return timercancel_event;}
    bool check_cancel(void) {return timercancel_event != nullptr;}

    void set_is_event_processing(void) {is_event_processing = true;}
    bool check_event_processing() {return is_event_processing;}
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};
#endif
