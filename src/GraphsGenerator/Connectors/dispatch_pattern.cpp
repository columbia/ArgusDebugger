#include "dispatch_pattern.hpp"
#include "eventlistop.hpp"

#define DISP_PATTERN_DEBUG 0

DispatchPattern::DispatchPattern(std::list<EventBase*> &_enq_list, std::list<EventBase*> &_exe_list)
:enqueue_list(_enq_list), execute_list(_exe_list)
{
}

void DispatchPattern::connect_dispatch_patterns(void)
{
	connect_enq_and_inv();
}

void DispatchPattern::connect_enq_and_inv(void)
{
    std::list<EventBase *> mix_list;
    mix_list.insert(mix_list.end(), enqueue_list.begin(), enqueue_list.end());
    mix_list.insert(mix_list.end(), execute_list.begin(), execute_list.end());
    EventLists::sort_event_list(mix_list);

	std::unordered_map<uint64_t, std::list<BlockEnqueueEvent *> > tmp_enq_list_dict;
    BlockEnqueueEvent *enq_event;
    BlockInvokeEvent *inv_event;
	for(auto event: mix_list) {
		switch(event->get_event_type()) {
			case DISP_ENQ_EVENT: {
        		enq_event = dynamic_cast<BlockEnqueueEvent*>(event);
				uint64_t key = enq_event->get_item() + event->get_pid();
				if (tmp_enq_list_dict.find(key) == tmp_enq_list_dict.end()) {
					std::list<BlockEnqueueEvent *> empty;
					tmp_enq_list_dict[key] = empty;
				}
            	tmp_enq_list_dict[key].push_back(enq_event);
				break;
			}
			case DISP_INV_EVENT: {
				inv_event = dynamic_cast<BlockInvokeEvent*>(event);
				uint64_t key = inv_event->get_item() + inv_event->get_pid();
				if (inv_event->is_begin() == true && tmp_enq_list_dict.find(key) != tmp_enq_list_dict.end()) {
					auto &list = tmp_enq_list_dict[key];
					int sz = list.size();
					bool ret = connect_dispatch_enqueue_for_invoke(list, inv_event);
					assert(!ret || tmp_enq_list_dict[key].size() < sz);
				}
				break;
			}
			default:
				break;
		}
	}
}

bool DispatchPattern::connect_dispatch_enqueue_for_invoke(std::list<BlockEnqueueEvent *> &tmp_enq_list, BlockInvokeEvent *inv_event)
{
    std::list<BlockEnqueueEvent *>::iterator enq_it;

    for (enq_it = tmp_enq_list.begin();
            enq_it != tmp_enq_list.end(); )  {
        BlockEnqueueEvent *enqueue_event = *enq_it;
        if (enqueue_event->get_item() == inv_event->get_item()
			&& enqueue_event->get_pid() == inv_event->get_pid()) {
            inv_event->set_root(enqueue_event);
            enqueue_event->set_consumer(inv_event);
			tmp_enq_list.erase(enq_it);
            return true;
        } else
			enq_it++;
    }
	return false;
}
