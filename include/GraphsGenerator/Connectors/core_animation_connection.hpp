#ifndef CA_CONNECTION_HPP
#define CA_CONNECTION_HPP

#include "caset.hpp"
#include "cadisplay.hpp"

class CoreAnimationConnection {
	typedef std::list<EventBase *> event_list_t;
    event_list_t &caset_list;
    event_list_t &cadisplay_list;
public:
    CoreAnimationConnection(event_list_t &caset_list, event_list_t &cadisplay_list);
    void core_animation_connection(void);
};

#endif
