#ifndef RL_CONNECTION_HPP
#define RL_CONNECTION_HPP

#include "rlboundary.hpp"

class RunLoopTaskConnection {
	typedef std::list<EventBase *> event_list_t;
    typedef std::map<tid_t, event_list_t > tid_evlist_t;

    tid_evlist_t tid_lists;
    event_list_t rl_item_list;
public:
    RunLoopTaskConnection(event_list_t &rl_item_list, tid_evlist_t &tid_lists);
    void connect_for_source1(RunLoopBoundaryEvent *rl_boundary_event);
    void connect_for_source0(RunLoopBoundaryEvent *rl_boundary_event, event_list_t::reverse_iterator rit);
    void connect_for_blocks(RunLoopBoundaryEvent *rl_boundary_event, event_list_t::reverse_iterator rit);
    void runloop_connection(void);
};

#endif
