#include "timercall_pattern.hpp"
#include "eventlistop.hpp"

#define TIMERCALL_DEBUG 0

TimerCallPattern::TimerCallPattern(std::list<EventBase*> &_timercreate_list, 
std::list<EventBase *> &_timercallout_list, std::list<EventBase*> &_timercancel_list)
:timercreate_list(_timercreate_list), 
timercallout_list(_timercallout_list), 
timercancel_list(_timercancel_list)
{
}

void TimerCallPattern::connect_timercall_patterns(void)
{
#ifdef TIMERCALL_DEBUG
    LOG_S(INFO) << "begin matching timercall pattern ... ";
#endif
    connect_create_and_cancel();
    connect_create_and_callout();
#ifdef TIMERCALL_DEBUG
    LOG_S(INFO) << "finish matching timercall pattern. ";
#endif
}

void TimerCallPattern::connect_create_and_cancel(void)
{

    std::list<EventBase *> mix_list;
    mix_list.insert(mix_list.end(), timercreate_list.begin(), timercreate_list.end());
    mix_list.insert(mix_list.end(), timercancel_list.begin(), timercancel_list.end());
    EventLists::sort_event_list(mix_list);
	
	create_map_t timer_create;
	for (auto it : mix_list) {
		switch (it->get_event_type()) {
			case TMCALL_CREATE_EVENT: {
    			TimerCreateEvent *timercreate_event = dynamic_cast<TimerCreateEvent *>(it);
				key_t key = {.ptr = timercreate_event->get_func_ptr(),
							.p0 = timercreate_event->get_param0(),
							.p1 = timercreate_event->get_param1()};
				timer_create[key] = timercreate_event;
				break;
			}
			case TMCALL_CANCEL_EVENT: {
    			TimerCancelEvent *timercancel_event = dynamic_cast<TimerCancelEvent *>(it);
				connect_timercreate_for_timercancel(timer_create, timercancel_event);
				break;
			}
		}
	}
	
}

bool TimerCallPattern::connect_timercreate_for_timercancel(create_map_t &recent_create,
		TimerCancelEvent *timercancel_event)
{
	try {
		key_t key = {.ptr = timercancel_event->get_func_ptr(),
			.p0 = timercancel_event->get_param0(),
			.p1 = timercancel_event->get_param1()};
		TimerCreateEvent *create = recent_create.at(key);
		if (timercancel_event->check_root(create) == true) {
            timercancel_event->set_timercreate(create);
            create->cancel_call(timercancel_event);
			recent_create.erase(key);
			return true;
		}
  	}
  	catch (const std::out_of_range& oor) {
		return false;
	}
	return false;
}

bool TimerCallPattern::connect_timercreate_for_timercancel(std::list<TimerCreateEvent *>&tmp_create_list,
		TimerCancelEvent *timercancel_event)
{
    std::list<TimerCreateEvent *>::reverse_iterator rit;
    for (rit = tmp_create_list.rbegin(); rit != tmp_create_list.rend(); rit++) {
        TimerCreateEvent * timercreate_event = dynamic_cast<TimerCreateEvent *>(*rit);
        if (timercancel_event->check_root(timercreate_event) == true) {
            timercancel_event->set_timercreate(timercreate_event);
            timercreate_event->cancel_call(timercancel_event);
            tmp_create_list.erase(next(rit).base());
            return true;
        }
    }
    return false;
}

void TimerCallPattern::connect_create_and_callout(void)
{
    std::list<EventBase *> mix_list;
    mix_list.insert(mix_list.end(), timercreate_list.begin(), timercreate_list.end());
    mix_list.insert(mix_list.end(), timercallout_list.begin(), timercallout_list.end());
    EventLists::sort_event_list(mix_list);

	create_map_t timer_create;
	for (auto it : mix_list) {
		switch (it->get_event_type()) {
			case TMCALL_CREATE_EVENT: {
        		TimerCreateEvent *create_event = dynamic_cast<TimerCreateEvent *>(it);
				key_t key = {.ptr = create_event->get_func_ptr(),
							.p0 = create_event->get_param0(),
							.p1 = create_event->get_param1()};
				timer_create[key] = create_event;
				//timer_create[create_event->get_func_ptr()] = create_event;
				break;
			}
			case TMCALL_CALLOUT_EVENT: {
            	TimerCalloutEvent *callout_event = dynamic_cast<TimerCalloutEvent *>(it);
				connect_timercreate_for_timercallout(timer_create, callout_event);
				break;
			}
		}
	}
}

bool TimerCallPattern::connect_timercreate_for_timercallout(create_map_t &recent_create,
		TimerCalloutEvent *timercallout_event)
{
	try {
		key_t key = {.ptr = timercallout_event->get_func_ptr(),
			.p0 = timercallout_event->get_param0(),
			.p1 = timercallout_event->get_param1()};
		TimerCreateEvent *create = recent_create.at(key);
		//TimerCreateEvent *create = recent_create.at(timercallout_event->get_func_ptr());
		if (timercallout_event->check_root(create) == true) {
            create->set_called_peer(timercallout_event);
            timercallout_event->set_timercreate(create);
			recent_create.erase(key);
			return true;
		}
  	}
  	catch (const std::out_of_range& oor) {
		return false;
	} 
	return false;
}

bool TimerCallPattern::connect_timercreate_for_timercallout(std::list<TimerCreateEvent*> &tmp_create_list,
		TimerCalloutEvent *timercallout_event)
{
    std::list<TimerCreateEvent *>::reverse_iterator rit;
    for (rit = tmp_create_list.rbegin(); rit != tmp_create_list.rend(); rit++) {
        TimerCreateEvent *timercreate_event = dynamic_cast<TimerCreateEvent *>(*rit);
        if (timercallout_event->check_root(timercreate_event) == true) {
            timercallout_event->set_timercreate(timercreate_event);
            timercreate_event->set_called_peer(timercallout_event);
            tmp_create_list.erase(next(rit).base());
            return true;
        }
    }
    return false;
}
