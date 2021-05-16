#include  "thread_divider.hpp"

void ThreadDivider::add_disp_invoke_begin_event(BlockInvokeEvent *invoke_event)
{
    current_disp_invokers.push(invoke_event);
    invoke_event->set_nested_level(current_disp_invokers.size());
    /* a new execution segment should initiate only when it is dequeued from dispatch queue */
    if (invoke_event->get_root()) {
		if (cur_group != nullptr) {
        	cur_group->set_divide_by_disp(DivideOldGroup);
		}
        cur_group = nullptr;
    }
    /* add event to group */
    add_general_event_to_group(invoke_event);
    cur_group->blockinvoke_level_inc();
}

void ThreadDivider::add_disp_invoke_end_event(BlockInvokeEvent *invoke_event, BlockInvokeEvent *begin_invoke)
{
    /* check the stack of dispatch block stacks*/
    if (current_disp_invokers.empty()) {
       	LOG_S(INFO) << "[error] empty stack: no begin event for block "\
					<< std::fixed << std::setprecision(1)\
					<< invoke_event->get_abstime() << " tid = "\
					<< std::hex << invoke_event->get_tid() << std::endl;
        add_general_event_to_group(invoke_event);
        return;
    }
    if (!(begin_invoke == current_disp_invokers.top())) {
        LOG_S(INFO) << "[error] matching at matching\t"\
				 	<< "\t block end " << std::hex << std::fixed << std::setprecision(1)\
        			<< invoke_event->get_abstime() << std::endl\
        			<< "\tTop of stack is at" << std::hex << std::fixed << std::setprecision(1)\
        			<< current_disp_invokers.top()->get_abstime() << std::endl;
        assert(begin_invoke == current_disp_invokers.top());
    }
    current_disp_invokers.pop();
    /* dispatch_mig_server inside block */
    if (!dispatch_mig_servers.empty()) {
        DispatchQueueMIGServiceEvent *current_mig_server = dispatch_mig_servers.top();
        if (current_mig_server->get_mig_invoker() == begin_invoke) {
            dispatch_mig_servers.pop();
        }
    }
    /* check the case dispatch block get broken*/
    if (cur_group != nullptr && gid2group(begin_invoke->get_group_id()) == cur_group) {
		cur_group->blockinvoke_level_dec();
	}
    add_general_event_to_group(invoke_event);
    if (begin_invoke->get_root()) {
        cur_group->set_divide_by_disp(DivideOldGroup);
        cur_group = nullptr;
    }
}

void ThreadDivider::add_disp_invoke_event_to_group(EventBase *event)
{
    BlockInvokeEvent *invoke_event = dynamic_cast<BlockInvokeEvent *>(event);
    assert(invoke_event);
    if (invoke_event->is_begin()) {
        add_disp_invoke_begin_event(invoke_event);
    } else {
        /* sanity check for block invoke event
         * and restore group if dispatch_mig_server invoked
         */
        BlockInvokeEvent *begin_invoke = dynamic_cast<BlockInvokeEvent *>(invoke_event->get_root());
        assert(begin_invoke);
		invoke_event->set_nested_level(begin_invoke->get_nested_level());
        add_disp_invoke_end_event(invoke_event, begin_invoke);    
    }
}

void ThreadDivider::add_disp_mig_event_to_group(EventBase *event)
{
    DispatchQueueMIGServiceEvent *dispatch_mig_server = dynamic_cast<DispatchQueueMIGServiceEvent *>(event);
    add_general_event_to_group(event);

    dispatch_mig_servers.push(dispatch_mig_server);
    if (!current_disp_invokers.empty()) {
        BlockInvokeEvent *mig_server_invoker = current_disp_invokers.top();
        dispatch_mig_server->set_mig_invoker(mig_server_invoker);
        dispatch_mig_server->save_owner(cur_group);
        cur_group->set_divide_by_disp(DivideOldGroup);
        cur_group = nullptr;
    }
}
