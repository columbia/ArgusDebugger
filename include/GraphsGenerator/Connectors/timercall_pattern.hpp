#ifndef TIMER_CALL_PATTERN_HPP
#define TIMER_CALL_PATTERN_HPP
#include "timer_call.hpp"

class TimerCallPattern {
	struct key_t {
		void *ptr;
		uint64_t p0;
		uint64_t p1;
	
	bool operator == (const key_t &k) const
	{
		return (ptr == k.ptr && p0 == k.p0 && p1 == k.p1);
	}

	bool operator < (const key_t &k) const
	{
		if (*this == k) return false;
		if (ptr == k.ptr && p0 == k.p0)
			return p1 < k.p1;
		else if (ptr == k.ptr)
			return p0 < k.p0;
		return ptr < k.ptr;

	}
	
	bool operator > (const key_t &k) const
	{
		if (*this == k) return false;
		return !(*this < k);
	}

	};

	typedef std::list<EventBase *> event_list_t;
	typedef std::map<key_t, TimerCreateEvent *> create_map_t;

	event_list_t &timercreate_list;
    event_list_t &timercallout_list;
	event_list_t &timercancel_list;

public:
    TimerCallPattern(event_list_t &_timercreate_list, 
		event_list_t &_timercallout_list,
		event_list_t &_timercancel_list);
    void connect_timercall_patterns(void);
    void connect_create_and_cancel(void);
    void connect_create_and_callout(void);

    bool connect_timercreate_for_timercallout(create_map_t &recent_create, TimerCalloutEvent *timercallout_event);
    bool connect_timercreate_for_timercancel(create_map_t &recent_create, TimerCancelEvent *timercancel_event);
    bool connect_timercreate_for_timercallout(std::list<TimerCreateEvent*> &tmp_create_list,
												TimerCalloutEvent *timercallout_event);
    bool connect_timercreate_for_timercancel(std::list<TimerCreateEvent *> &tmp_create_list,
												TimerCancelEvent *timercancel_event);
};
typedef TimerCallPattern timercall_pattern_t;
#endif
