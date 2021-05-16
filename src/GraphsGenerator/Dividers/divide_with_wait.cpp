#include "thread_divider.hpp"

//////////////////////////////////////////////
//hueristics of wait event 
/////////////////////////////////////////////
bool ThreadDivider::matching_wait_syscall(WaitEvent *wait)
{
    SyscallEvent *syscall_event = nullptr;
	auto ev_list = cur_group->get_container();
    int count = ev_list.size() < 5 ? ev_list.size() : 5;

	for (auto rit = ev_list.rbegin(); rit != ev_list.rend() && count > 0; rit++, count--) {
        if ((*rit)->get_event_type() != SYSCALL_EVENT)
            continue;
        syscall_event = dynamic_cast<SyscallEvent *>(*rit);
		break;
	}
	if (syscall_event == nullptr) return false;

    if (wait->get_abstime() < syscall_event->get_ret_time() 
		|| syscall_event->get_ret_time() < 0.5) {
         wait->pair_syscall(syscall_event);
         return true;
    }

    return false;
}

void ThreadDivider::add_wait_event_to_group(EventBase *event)
{
    add_general_event_to_group(event);
    WaitEvent *wait_event = dynamic_cast<WaitEvent *>(event);
    
    if (event->get_procname() != "kernel_task") {
        assert(wait_event);
        matching_wait_syscall(wait_event);
    }

#if !OVERRIDE_DISP
    /* if not in the middle of dispatch block execution
     * close cur_group
     * otherwise make a whole dispatch block in one group
     */
	if (cur_group->get_blockinvoke_level() == 0)
#endif
	{
        cur_group->set_divide_by_wait(DivideOldGroup);
        cur_group = nullptr;
	}
}

bool ThreadDivider::matching_mr_syscall(MakeRunEvent *mkrun)
{
    SyscallEvent *syscall_event = nullptr;
	auto ev_list = cur_group->get_container();
    int count = ev_list.size() < 5 ? ev_list.size() : 5;

	for (auto rit = ev_list.rbegin(); rit != ev_list.rend() && count > 0; rit++, count--) {
        if ((*rit)->get_event_type() != SYSCALL_EVENT)
            continue;
        syscall_event = dynamic_cast<SyscallEvent *>(*rit);
		break;
	}
	if (syscall_event == nullptr) return false;

    if (mkrun->get_abstime() < syscall_event->get_ret_time()
		|| syscall_event->get_ret_time() < 0.5) {
        mkrun->pair_syscall(syscall_event);
       return true;
    }

    return false;
}

void ThreadDivider::add_mr_event_to_group(EventBase *event)
{
    add_general_event_to_group(event);
    MakeRunEvent *mkrun = dynamic_cast<MakeRunEvent *>(event);
    assert(mkrun);
    matching_mr_syscall(mkrun);
}
