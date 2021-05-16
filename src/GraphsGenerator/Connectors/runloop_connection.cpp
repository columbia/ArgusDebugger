#include "runloop_connection.hpp"
#include "mach_msg.hpp"
#include "eventlistop.hpp"
#include <iterator>

#define RL_BOUNDARY_DEBUG 0

RunLoopTaskConnection::RunLoopTaskConnection(std::list<EventBase *> &_rl_item_list, tid_evlist_t &_tid_lists)
:tid_lists(_tid_lists), rl_item_list(_rl_item_list)
{
}

void RunLoopTaskConnection::connect_for_source1(RunLoopBoundaryEvent *rl_boundary_event)
{
    event_list_t cur_tid_list = tid_lists[rl_boundary_event->get_tid()];
	event_list_t::iterator it = std::find(cur_tid_list.begin(), cur_tid_list.end(), rl_boundary_event);

	if (it == cur_tid_list.begin())
		return;

	event_list_t::reverse_iterator rit(it);
    for (; rit != cur_tid_list.rend(); rit++) {
        if ((*rit)->get_event_type() != MSG_EVENT)
            continue;
        MsgEvent *msg_event = dynamic_cast<MsgEvent *>(*rit);
        if (!msg_event->get_header()->is_mig() && msg_event->get_header()->check_recv()) {
            rl_boundary_event->set_owner(*rit);
            break;
        }
    }
}

void RunLoopTaskConnection::connect_for_source0(RunLoopBoundaryEvent *unset_sig_event,
	std::list<EventBase *>::reverse_iterator rit)
{
    RunLoopBoundaryEvent *rl_event = nullptr;
    for (;rit != rl_item_list.rend(); rit++) {
        rl_event = dynamic_cast<RunLoopBoundaryEvent *>(*rit);
        if (!rl_event)
            continue;
        if (rl_event->get_state() == SetSignalForSource0 
            && unset_sig_event->get_rls() == rl_event->get_rls()) {
            unset_sig_event->set_owner(rl_event);
            rl_event->set_consumer(unset_sig_event);
            break;
        }
    }
}

void RunLoopTaskConnection::connect_for_blocks(RunLoopBoundaryEvent *rl_boundary_event,
	std::list<EventBase *>::reverse_iterator rit)
{
    RunLoopBoundaryEvent *block_orig = nullptr;
    for (; rit != rl_item_list.rend(); rit++) {
        block_orig = dynamic_cast<RunLoopBoundaryEvent *>(*rit);
        if (!block_orig || block_orig->get_state() != BlockPerform)
            continue;
        if (block_orig->get_item() == rl_boundary_event->get_item()) {
            rl_boundary_event->set_owner(block_orig);
            block_orig->set_consumer(rl_boundary_event);
            break;
        }
    }
}

void RunLoopTaskConnection::runloop_connection(void)
{
	LOG_S(INFO) << "Begin runloop src connection...";
    std::list<EventBase *>::iterator it;
    EventLists::sort_event_list(rl_item_list);
    for (it = rl_item_list.begin(); it != rl_item_list.end(); it++) {
        RunLoopBoundaryEvent *rl_boundary_event = dynamic_cast<RunLoopBoundaryEvent *>(*it);
        assert(rl_boundary_event);
        if (rl_boundary_event->get_state() == ItemEnd)
            continue;

		if (rl_boundary_event->get_op() == "ARGUS_RL_DoSource0" 
            && rl_boundary_event->get_state() == UnsetSignalForSource0) {
            // RunLoopWakeup event that wake up current thread
            std::list<EventBase *>::reverse_iterator rit(it);
			if (rit == rl_item_list.rend())
				break;
            connect_for_source0(rl_boundary_event, rit);
        }

        if (rl_boundary_event->get_op() == "ARGUS_RL_DoBlocks") {
            if (rl_boundary_event->get_state() == ItemBegin) {
                std::list<EventBase *>::reverse_iterator rit(it);
				if (rit == rl_item_list.rend())
					break;
                connect_for_blocks(rl_boundary_event, rit);
            }
        } 
    }
	LOG_S(INFO) << "Finished runloop connection";
}
