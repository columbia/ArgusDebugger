#include "canonization.hpp"

NormEvent::NormEvent(EventBase *_event)
{
    event = _event;
    peer = -1;
    proc_name = event->get_procname();
    event_type = event->get_event_type();

    if (event->get_event_peer())
        peer = event->get_event_peer()->get_pid();
}

bool NormEvent::operator==(NormEvent &other)
{
    if (proc_name != other.proc_name)
        return false;
    if (event_type != other.event_type)
        return false;
    if (event_type != WAIT_EVENT 
		&& event_type != FAKED_WOKEN_EVENT
		&& peer != -1
		&& other.peer != -1) {
        if (peer != other.peer)
            return false;
    }

    bool ret = true;
    EventBase *peer = other.get_real_event();

    switch(event_type) {
        case WAIT_EVENT: {
            WaitEvent *cur_event = dynamic_cast<WaitEvent *>(event);
            WaitEvent *peer_event = dynamic_cast<WaitEvent *>(peer);
			SyscallEvent *cur_syscall = cur_event->get_syscall();
			SyscallEvent *peer_syscall = peer_event->get_syscall();
			if ((cur_syscall != nullptr && peer_syscall == nullptr)
				|| (cur_syscall == nullptr && peer_syscall != nullptr))
				return false;
			if (cur_syscall != nullptr && peer_syscall != nullptr) {
				if (cur_syscall->get_op() != peer_syscall->get_op())
					return false;
			}
            break;
        }
        case MSG_EVENT: {
            MsgEvent *cur_event = dynamic_cast<MsgEvent *>(event);
            MsgEvent *peer_event = dynamic_cast<MsgEvent*>(peer);
            if (cur_event->get_header()->get_msgh_id() != 0)
                if (cur_event->get_header()->get_msgh_id() != peer_event->get_header()->get_msgh_id())
                    ret = false;
            break;
        }
        case SYSCALL_EVENT: {
            SyscallEvent *cur_event = dynamic_cast<SyscallEvent *>(event);
            SyscallEvent *peer_event = dynamic_cast<SyscallEvent *>(peer);
            if (cur_event->get_entry() != peer_event->get_entry()) {
                ret = false;    
			}
            break;
        }
        case RL_BOUNDARY_EVENT: {
            RunLoopBoundaryEvent *cur_event = dynamic_cast<RunLoopBoundaryEvent *>(event);
            RunLoopBoundaryEvent *peer_event = dynamic_cast<RunLoopBoundaryEvent *>(peer);
            if (cur_event->get_state() != peer_event->get_state())
                ret = false;
            if (cur_event->get_rls() != peer_event->get_rls())
                ret = false;
            if (cur_event->get_func_ptr() != peer_event->get_func_ptr())
                ret = false;
            break;
        } 

        case NSAPPEVENT_EVENT: {
            NSAppEventEvent *cur_event = dynamic_cast<NSAppEventEvent *>(event);
            NSAppEventEvent *peer_event = dynamic_cast<NSAppEventEvent *>(peer);
            if (cur_event->is_begin() != peer_event->is_begin())
                ret = false;
            if (cur_event->get_event_class() != peer_event->get_event_class())
                ret = false;
            break;
        }

		case BACKTRACE_EVENT: {
			BacktraceEvent *cur_event = dynamic_cast<BacktraceEvent *>(event);
			BacktraceEvent *peer_event = dynamic_cast<BacktraceEvent *>(peer);
			
			auto cur_vector = cur_event->get_frames();
			auto peer_vector = peer_event->get_frames();
			auto cur_it = cur_vector.begin();
			auto peer_it = peer_vector.begin();
			int count = 5;
			while(cur_it != cur_vector.end() && peer_it != peer_vector.end() && count-- > 0) {
				if (*cur_it != *peer_it)
					return false;
				cur_it++;
				peer_it++;
			}
		}
        default:
            break;
    }    
    return ret;
}

bool NormEvent::operator!=(NormEvent & other)
{
    return !(*this == other);
}

EventBase * NormEvent::get_real_event(void)
{
    return event;
}
