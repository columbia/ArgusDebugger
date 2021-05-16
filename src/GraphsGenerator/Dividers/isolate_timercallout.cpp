#include "thread_divider.hpp"

void ThreadDivider::add_timercallout_event_to_group(EventBase *event)
{
    /* in kernel thread, always has a new group
     * because one caller thread(kernel thread) might
     * invoke multiple timer timercallouts
     */
    if (cur_group != nullptr) {
		assert(cur_group->get_blockinvoke_level() >= 0);
        cur_group = nullptr;
    }
    add_general_event_to_group(event);
}
