#ifndef MKRUN_WAIT_HPP
#define MKRUN_WAIT_HPP
#include "wait.hpp"
#include "mkrunnable.hpp"

class MakeRunnableWaitPair {
	typedef std::list<EventBase *> event_list_t;
    event_list_t &wait_list;
    event_list_t &mkrun_list;
public:
    MakeRunnableWaitPair(event_list_t &_wait_list, event_list_t &_mkrun_list);
    void pair_wait_mkrun(void);
};

typedef MakeRunnableWaitPair mkrun_wait_t;
#endif

