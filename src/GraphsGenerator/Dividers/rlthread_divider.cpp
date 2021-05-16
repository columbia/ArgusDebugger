#include "thread_divider.hpp"

RunLoopThreadDivider::RunLoopThreadDivider(int _index,
	divides_t &sub_results,
	event_list_t &tid_list) 
:ThreadDivider(_index, sub_results, tid_list)
{
    save_cur_rl_group_for_invoke = nullptr;
    invoke_in_rl = nullptr;
    rl_event_process = nullptr;
}

RunLoopThreadDivider::~RunLoopThreadDivider(void)
{
}

void RunLoopThreadDivider::add_nsappevent_event_to_group(EventBase *event)
{
    cur_group = nullptr;
    add_general_event_to_group(event);
}

void RunLoopThreadDivider::add_rlboundary_event_to_group(EventBase *event)
{
    RunLoopBoundaryEvent *boundary_event = dynamic_cast<RunLoopBoundaryEvent *>(event);
    assert(boundary_event);

    if (boundary_event->get_state() == EventsBegin) {
		if (cur_group != nullptr && cur_group != rl_event_process)
			cur_group = nullptr;
        add_general_event_to_group(event);
        rl_event_process = cur_group;
        return;
    }
    
    if (boundary_event->get_state() == EventsEnd) {
        cur_group = rl_event_process;
        add_general_event_to_group(event);
        rl_event_process = nullptr;
		cur_group = nullptr;
        return;
    }

    if (boundary_event->get_op() == "ARGUS_RL_DoBlocks"
        || boundary_event->get_op() == "ARGUS_RL_DoObserver"
        || boundary_event->get_op() == "ARGUS_RL_DoSource0") {

        switch(boundary_event->get_state()) {
            case ItemBegin:
                cur_group = nullptr;
                add_general_event_to_group(event);
                break;
            case ItemEnd:
                add_general_event_to_group(event);
                cur_group = rl_event_process;
                break;
            default:
                add_general_event_to_group(event);
                break;
        } 
    }

    if (boundary_event->get_op() == "ARGUS_RL_DoSource1") {
       add_general_event_to_group(event); 
    }
}

void RunLoopThreadDivider::divide()
{
	for (auto event : tid_list) {
        switch (event->get_event_type()) {
			// store events to handler for hooking
            case VOUCHER_EVENT:
            case FAKED_WOKEN_EVENT:
                store_event_to_group_handler(event);
                break;
            case BACKTRACE_EVENT:
				add_backtrace_event_to_group(event);
				break;
            //traditional heristic to divide batching
            case DISP_INV_EVENT:
                add_disp_invoke_event_to_group(event);
                break;

            case TMCALL_CALLOUT_EVENT:
                add_timercallout_event_to_group(event);
                break;

            case NSAPPEVENT_EVENT:
                add_nsappevent_event_to_group(event);
                break;

            case RL_BOUNDARY_EVENT:
                add_rlboundary_event_to_group(event);
                break;

#if HEURISTICS
			// add heuristics for dividing unkown batching
#if HEURISTICS_MSG
            case MSG_EVENT:
                add_msg_event_into_group(event);
                break;
#endif
#if HEURISTICS_WAIT
            case WAIT_EVENT:
                add_wait_event_to_group(event);
                break;
#endif
#endif
            case MR_EVENT:
                add_mr_event_to_group(event);
                break;

            default:
                add_general_event_to_group(event);
                break;
        }
    }
    submit_result[index] = local_result;
}
