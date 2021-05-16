#include "core_animation_connection.hpp"
#include "eventlistop.hpp"

#define DEBUG_CA_CONN 0
CoreAnimationConnection::CoreAnimationConnection(std::list<EventBase *> &_caset_list, std::list<EventBase *> &_cadisplay_list)
:caset_list(_caset_list), cadisplay_list(_cadisplay_list)
{
}

void CoreAnimationConnection::core_animation_connection (void)
{
    std::list<EventBase *> mix_list;
    mix_list.insert(mix_list.end(), caset_list.begin(), caset_list.end());
    mix_list.insert(mix_list.end(), cadisplay_list.begin(), cadisplay_list.end());
    EventLists::sort_event_list(mix_list);

    std::list<EventBase *>::iterator it;
    std::list<EventBase *>::reverse_iterator rit;
    
    for (it = mix_list.begin(); it != mix_list.end(); it++) {
        CADisplayEvent * display_event = dynamic_cast<CADisplayEvent *>(*it);
        if (!display_event)
            continue;

        rit = find(mix_list.rbegin(), mix_list.rend(), display_event);
        uint64_t object_addr =  display_event->get_object_addr();

        for (; rit != mix_list.rend(); rit++) {
            CASetEvent *set_event = dynamic_cast<CASetEvent *>(*rit);
            if (!set_event)
                continue;
            assert(display_event->get_abstime() > set_event->get_abstime());
            if (set_event->get_object_addr() == object_addr) {
                if (set_event->get_display_object() != nullptr)
                    break;
                display_event->push_set(set_event);
                set_event->set_display(display_event);
            }
        }
            
        if (display_event->ca_set_event_size() == 0) {
        }

    }
    mix_list.clear();
}
