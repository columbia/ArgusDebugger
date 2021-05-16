#include "canonization.hpp"
#define DEBUG_NORMGROUP 0

NormGroup::NormGroup(Group *g,std::map<EventType::event_type_t, bool> &_key_events)
:key_events(_key_events)
{
    group = g;
    normalized_events.clear();
    normalize_events();
}

NormGroup::~NormGroup(void)
{
    std::list<NormEvent *>::iterator it;
    for (it = normalized_events.begin(); it != normalized_events.end(); it++) {
        assert(*it != nullptr);
        delete(*it);
    }
    normalized_events.clear();
}

void NormGroup::normalize_events(void)
{
	for (auto event: group->get_container()) {
        if (event->get_op() == "MSC_thread_switch" 
			|| key_events[event->get_event_type()] == false)
            continue;

		NormEvent *NormGroupEvent = new NormEvent(event);
		if (!NormGroupEvent) {
			exit(EXIT_FAILURE);
		}
		normalized_events.push_back(NormGroupEvent);
    }
}

void NormGroup::print_events()
{
    LOG_S(INFO) << "Normalize Group #" << std::hex << group->get_group_id() ;
	for (auto event: normalized_events) {
		EventBase *real_event = event->get_real_event();
		LOG_S(INFO) << std::hex << get_group_id() \
			<< " Event " << real_event->get_op() \
			<< " at " << std::fixed << std::setprecision(1) \
            << real_event->get_abstime() \
			;
	}
}

//simplified calculation of edit distance for performance
bool NormGroup::is_subset_of(std::list<NormEvent *> peer_set)
{
    auto peer_rit = peer_set.rbegin();
    auto this_rit = normalized_events.rbegin();
    int cur_len = normalized_events.size();
    int peer_len = peer_set.size();
	int edit = std::abs(cur_len - peer_len);
	int len = cur_len > peer_len ? peer_len : cur_len;
	
	while (len > 0) {
		assert(this_rit != normalized_events.rend());
		bool equal = false;
		while (peer_len > 0) {
			assert(peer_rit != peer_set.rend());
			equal = (**this_rit) == (**peer_rit);
			peer_len--;
			peer_rit++;
			if (equal) {//check next event
				break;
			} else {
				edit++;
			}
		}
		if (equal == false)
			return false;
		this_rit++;
		len--;
	}

    if (len <= 0 && edit < 0.4 * cur_len) 
        return true;
	if (len <= 0 && edit >= 0.4 * cur_len)
		LOG_S(INFO) << "To much edits in comparison";
    return false;
}

NormEvent *NormGroup::last_event_of_type(int type)
{
	for (auto rit = normalized_events.rbegin(); rit != normalized_events.rend(); rit++) {
		if ((*rit)->event_type == type)
			return *rit;
	}
	return NULL;
}

bool NormGroup::operator<=(NormGroup &other)
{
    std::list<NormEvent *> other_normalized_events = other.get_normalized_events();
	return is_subset_of(other_normalized_events);
}

bool NormGroup::operator==(NormGroup &other)
{
    std::list<NormEvent *> other_normalized_events = other.get_normalized_events();
	int distance = other_normalized_events.size() - normalized_events.size();
	if (distance > 0.2 * normalized_events.size()) {
		return false;
	}
    if (!is_subset_of(other_normalized_events) || !other.is_subset_of(normalized_events)) {
        return false;
    }
    return true;
}

bool NormGroup::operator!=(NormGroup &other)
{
    return !(*this == other);
}

std::string NormGroup::signature()
{
	std::string sig;
	for (auto event: normalized_events) {
		sig += event->proc_name;
		sig += std::to_string(event->event_type);
	}

	for (auto event: normalized_events) {
		std::string desc;
		EventBase *real_event = event->get_real_event();

		if (real_event->get_event_peer() && event->event_type != WAIT_EVENT) {
			desc += "#";
			desc += real_event->get_event_peer()->get_procname();
		}

		switch(event->event_type) {
			case SYSCALL_EVENT: {
				SyscallEvent *cur_event = dynamic_cast<SyscallEvent *>(real_event);
				desc += "#";
				desc += cur_event->get_entry()->syscall_name;
				break;
			}
			case BACKTRACE_EVENT: {
				BacktraceEvent *cur_event = dynamic_cast<BacktraceEvent *>(real_event);
				std::list<uint64_t> frames = cur_event->get_frames();
				int count = 5;
				for (auto frame: frames) {
					if (count == 0)
						break;
					desc += "#";
					desc += std::to_string(frame);
					count--;
				}
				break;
			}		
		}
		sig += desc;
		desc.clear();
	}
	return sig;
}
