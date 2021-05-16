#ifndef DISPATCH_PATTERN_HPP
#define DISPATCH_PATTERN_HPP
#include "dispatch.hpp"
class DispatchPattern {
	typedef std::list<EventBase *> event_list_t;
	event_list_t &enqueue_list;
	event_list_t &execute_list;
	//event_list_t &dequeue_list;
public:
    //DispatchPattern(event_list_t &_enq_list, event_list_t &_deq_list, event_list_t &_exe_list);
    DispatchPattern(event_list_t &_enq_list, event_list_t &_exe_list);
	
    void connect_dispatch_patterns();
	void connect_enq_and_inv();
    //void connect_enq_and_deq();
    //void connect_deq_and_exe();
	bool connect_dispatch_enqueue_for_invoke(std::list<BlockEnqueueEvent *> &tmp_enq_list, BlockInvokeEvent *inv_event);
    //bool connect_dispatch_enqueue_for_dequeue(std::list<BlockEnqueueEvent *> &tmp_enq_list, BlockDequeueEvent *);
    //bool connect_dispatch_dequeue_for_execute(std::list<BlockDequeueEvent *> &tmp_deq_list, BlockInvokeEvent *);
};
typedef DispatchPattern dispatch_pattern_t;
#endif
