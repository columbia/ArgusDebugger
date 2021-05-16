#include "mkrun_wait_pair.hpp"
#include "eventlistop.hpp"

#define MKRUN_WAIT_DEBUG 0

MakeRunnableWaitPair::MakeRunnableWaitPair(std::list<EventBase *> &_wait_list, std::list<EventBase *> &_mkrun_list)
:wait_list(_wait_list), mkrun_list(_mkrun_list)
{
}

void MakeRunnableWaitPair::pair_wait_mkrun(void)
{
    std::list<EventBase*> mix_sorted_list;
    std::list<EventBase*>::iterator it;
    std::map<uint64_t, WaitEvent*> wait_map;
    WaitEvent * wait;
    MakeRunEvent * mr_event;

    mix_sorted_list.insert(mix_sorted_list.end(), wait_list.begin(), wait_list.end());
    mix_sorted_list.insert(mix_sorted_list.end(), mkrun_list.begin(), mkrun_list.end());
    EventLists::sort_event_list(mix_sorted_list);

#ifdef MKRUN_WAIT_DEBUG
    LOG_S(INFO) << "begin matching mkrun and wait ... ";
#endif
    for (it = mix_sorted_list.begin(); it != mix_sorted_list.end(); it++) {
        wait = dynamic_cast<WaitEvent*>(*it);
        if (wait) {
            wait_map[wait->get_tid()] = wait;
        } else {
            mr_event = dynamic_cast<MakeRunEvent*>(*it);
            assert(mr_event);
            if (wait_map.find(mr_event->get_peer_tid()) != wait_map.end()) {
                mr_event->pair_wait(wait_map[mr_event->get_peer_tid()]);
                wait_map[mr_event->get_peer_tid()]->pair_mkrun(mr_event);
            } 
        }    
    }
#ifdef MKRUN_WAIT_DEBUG
    LOG_S(INFO) << "finish matching mkrun and wait.";
#endif
}
