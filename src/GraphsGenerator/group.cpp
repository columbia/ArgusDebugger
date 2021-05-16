#include "group.hpp"
GroupDivideFlag::GroupDivideFlag()
{
    divide_by_msg = divide_by_disp = divide_by_wait = 0;
    disp_divide_by_wait = 0;
    blockinvoke_level = 0;
}

GroupSemantics::GroupSemantics(EventBase *_root)
{
    root = _root;
    nsapp_event = nullptr;
    caset_event = nullptr;
    group_peer.clear();
    group_tags.clear();
}

GroupSemantics::~GroupSemantics()
{
    root = nsapp_event = nullptr;
}

void GroupSemantics::set_root(EventBase *r)
{
    root = r;
}

void GroupSemantics::add_group_peer(std::string proc)
{
	group_peer.insert(proc);
}

void GroupSemantics::add_group_peer(std::set<std::string> procs)
{
    if (procs.size())
        group_peer.insert(procs.begin(), procs.end());
}

void GroupSemantics::add_group_tags(std::string desc, uint32_t freq)
{
    group_tags[desc] = freq;
}

std::set<std::string> GroupSemantics::get_syms_freq_under(uint32_t freq)
{
    std::set<std::string> ret;
    ret.clear();
    for (auto elem: group_tags) {
        if (elem.second > 0  && elem.second <= freq)
            ret.insert(elem.first);
    }
    return ret;
}

std::map<std::string, int> Group::get_busy_syms(void)
{
	std::map<std::string, int> ret;
	for (auto event: container) {
		if (event->get_event_type() != INTR_EVENT)
			continue;
		IntrEvent *intr = dynamic_cast<IntrEvent *>(event);
		if (intr->get_context().size() > 0) {
				ret[intr->get_context()]++;
		}
	}
	return ret;
}


Group::Group(uint64_t _group_id, EventBase *_root)
:GroupDivideFlag(), GroupSemantics(_root)
{
    group_id = _group_id;
    time_span = 0.0;
    sorted = false;
    container.clear();
    cluster_id = -1;
	propagate_frames.first = -1;
}

Group::~Group(void)
{
    container.clear();
}

void Group::set_group_id(uint64_t _group_id)
{
    group_id = _group_id;
    EventBase *root = get_root();
    if (root)
        root->set_group_id(group_id);

    std::list<EventBase *>::iterator it;
    for (it = container.begin(); it != container.end(); it++)
        (*it)->set_group_id(group_id);
}

std::string Group::get_procname(void)
{
    if (get_last_event()) 
        return get_last_event()->get_procname();
    return "";
}

void Group::propagate_bt(int index, BacktraceEvent *bt_event)
{
	assert(bt_event != nullptr);

	std::list<uint64_t> ret;
	ret.clear();

	std::list<uint64_t> cur = bt_event->get_frames();

	if (propagate_frames.first >= 0) {
		std::list<uint64_t> prev = propagate_frames.second;

		if (prev.size() > 0) {
			for (auto it = cur.begin(); it != cur.end(); it++)
				if (std::find(prev.begin(), prev.end(), *it) != prev.end()) {
					ret.insert(ret.end(), it, cur.end());
					break;
				}
		}
	}
	if (cur.size() > 3) {
		auto it = cur.begin();
		std::advance(it, 2);
		ret.insert(ret.end(), it, cur.end());
	}


	if (propagate_frames.first >= 0)
		assert(propagate_frames.first < container.size());
	else
		assert(propagate_frames.first == -1);

	auto it = container.begin();
	std::advance(it, propagate_frames.first + 1);

	for (; it != container.end(); it++) {
		(*it)->set_propagated_frame(ret);
	}
	propagate_frames.first = index;
	propagate_frames.second = cur;

	bt_event->set_propagated_frame(cur);
}

void Group::add_to_container(EventBase *event)
{
    assert(event);
    if (!contains_nsappevent() && event->get_event_type() == NSAPPEVENT_EVENT)
        set_nsappevent(event);
    if (!contains_view_update() && event->get_event_type() == CA_SET_EVENT)
        set_view_update(event);
    event->set_group_id(group_id);
    container.push_back(event);
#if PROPAGATE_BT
	if (event->get_event_type() == BACKTRACE_EVENT)
		propagate_bt(container.size() - 1, dynamic_cast<BacktraceEvent *>(event));
#endif
}

void Group::add_to_container(Group *group)
{
    std::list<EventBase*>::iterator it;
    std::list<EventBase *> &peer_container = group->get_container();
    for (it = peer_container.begin(); it != peer_container.end(); it++) {
        add_to_container(*it);
    }
}

void Group::empty_container()
{
    std::list<EventBase*>::iterator it;
    for (it = container.begin(); it != container.end(); it++) {
        (*it)->set_group_id(-1);
    }
    container.clear();
}

void Group::sort_container(void)
{
    assert(container.size() > 0);
    if (sorted == false) {
        container.sort(Parse::EventComparator::compare_time);
        sorted = true;
    }
}

EventBase *Group::get_last_event(void) {
    sort_container();
    return container.size() > 0 ? container.back() : nullptr;
}

EventBase *Group::get_first_event(void) {
	if (container.size() == 0)
		return nullptr;
	
    sort_container();
    std::list<EventBase *>::iterator it = container.begin();
	EventBase *first_event = container.front();
	while (it != container.end() && (*it)->get_event_type() == BACKTRACE_EVENT)
		it++;
	if (it != container.begin() && it != container.end())
		first_event = *it;
    return first_event;
}

EventBase *Group::get_real_first_event(void) {
	if (container.size() == 0)
		return nullptr;
	
    sort_container();
    std::list<EventBase *>::iterator it = container.begin();
	EventBase *real_first_event = container.front();
	while (it != container.end() && ((*it)->get_event_type() == BACKTRACE_EVENT
		||(*it)->get_event_type() == FAKED_WOKEN_EVENT))
		it++;
	if (it != container.begin() && it != container.end())
		real_first_event = *it;
    return real_first_event;
}

EventBase *Group::event_at(int event_index)
{
    std::list<EventBase *> lists = get_container();
    if (lists.size() > event_index) {
        std::list<EventBase *>::iterator it = lists.begin();
        std::advance(it, event_index);
		return *it;
	}
	return nullptr;
}

double Group::event_before(int index) {
    index = get_container().size() > index ? index : get_container().size() - 1;
    if (index < 0)
        return 0.0;

    EventBase *event = event_at(index);
    if (event != nullptr)
        return event->get_abstime() + 0.5;
    return 0.0;          
}

int Group::event_to_index(EventBase *event)
{
    auto pos = std::find(container.begin(), container.end(), event);
    if (pos != container.end())
        return distance(container.begin(), pos);
    return -1;
}

bool Group::operator==(Group &peer)
{
    sort_container();
    peer.sort_container();
    std::list<EventBase *> &peer_container = peer.get_container();
    std::list<EventBase *>::iterator it;
    std::list<EventBase *>::iterator peer_it;
    for (it = container.begin(), peer_it = peer_container.begin();;) {
        while (it != container.end() && (*it)->get_event_type() == INTR_EVENT)
            it++;
        while(peer_it != peer_container.end() && (*peer_it)->get_event_type() == INTR_EVENT)
            peer_it++;

        if (it == container.end() && peer_it != peer_container.end())
            return false;
        if (it != container.end() && peer_it == peer_container.end())
            return false;
        if (it == container.end() && peer_it == peer_container.end())
            return true;
        if ((*peer_it)->get_event_type() == (*it)->get_event_type()) {
            it++;
            peer_it++;
        } else
            return false;
    }

    return true;
}

double Group::calculate_time_span()
{
    if (container.size()) {
        sort_container();
        time_span = get_last_event()->get_abstime() - get_first_event()->get_abstime();
    }
    return time_span;
}

int Group::compare_syscall_ret(Group *peer)
{
    std::list<EventBase *> &peer_container = peer->get_container();
    std::list<EventBase *>::iterator cur_it, peer_it;
    int ret = 0;
    for (cur_it = container.begin(), peer_it = peer_container.begin();
        cur_it != container.end() && peer_it != peer_container.end();) {
        while(cur_it != container.end() && (*cur_it)->get_event_type() != SYSCALL_EVENT)
            cur_it++;
        if (cur_it == container.end())
            return ret;

        while(peer_it != peer_container.end() && (*peer_it)->get_event_type() != SYSCALL_EVENT)
            peer_it++;

        if (peer_it == peer_container.end())
            return ret;

        assert(*cur_it != nullptr && *peer_it != nullptr);
        SyscallEvent *cur_syscall = dynamic_cast<SyscallEvent *>(*cur_it);
        SyscallEvent *peer_syscall = dynamic_cast<SyscallEvent *>(*peer_it);

        if (cur_syscall->get_ret() != peer_syscall->get_ret())
            ret = 1;

        if (ret == 1)
            return ret;
        cur_it++;
        peer_it++;
    }
    return ret;
}

int Group::compare_wait_ret(Group *peer)
{
    WaitEvent *peer_wait = dynamic_cast<WaitEvent *>((peer->get_container()).back());
    WaitEvent *cur_wait = dynamic_cast<WaitEvent *>(container.back());
    if (!peer_wait || !cur_wait) {
        return 0;
    }
	SyscallEvent *peer_sys = peer_wait->get_syscall();
	SyscallEvent *cur_sys = cur_wait->get_syscall();
	if (!peer_sys || !cur_sys) {
        return 0;
	}
	
	if (peer_sys->get_ret() != cur_sys->get_ret()) {
		if (cur_sys->get_ret() != 0) 
			return 1;
		if (peer_sys->get_ret() != 0)
			return -1;
	}
	return 0;
}

int Group::compare_wait_time(Group *peer)
{
    WaitEvent *peer_wait = dynamic_cast<WaitEvent *>((peer->get_container()).back());
    WaitEvent *cur_wait = dynamic_cast<WaitEvent *>(container.back());
    if (!peer_wait || !cur_wait) {
        LOG_S(INFO) << " Not waits captured for both Group 0x"\
			 << std::hex << group_id \
             << " and 0x"\
			 << std::hex << peer->get_group_id();
        return 0;
    }
    double cur_wait_time = cur_wait->get_time_cost();
    double peer_wait_time = peer_wait->get_time_cost();
    if ((cur_wait_time < 0.001 && peer_wait_time < 1000000) 
		|| cur_wait_time - peer_wait_time > 1000000) {
        return 1;
    }
    else if (cur_wait_time > 0.1 && cur_wait_time < 1500000
		 && peer_wait_time - cur_wait_time > 1000000) {
        return -1;
    }
    return 0;
}

int Group::compare_timespan(Group *peer)
{
    double peer_timespan = peer->calculate_time_span();
    this->calculate_time_span();
    if (time_span - peer_timespan > 1000000)
        return 1;
     if (peer_timespan - time_span > 1000000)
        return -1;
    return 0;
}

double Group::wait_time()
{
	sort_container();
	WaitEvent *wait_event = dynamic_cast<WaitEvent *>(container.back());
	if (wait_event)
		return wait_event->get_time_cost();
	return 0.0;
} 

bool Group::wait_over()
{
    sort_container();
    std::list<EventBase*>::iterator it;
    for (it = container.begin(); it != container.end(); it++) {
        WaitEvent *wait_event = dynamic_cast<WaitEvent *>(*it);
        if (wait_event != nullptr) {
            double wait_length = wait_event->get_time_cost();
            if (wait_length > 800000)
                return true;
            if (wait_length < 10e-8 && wait_length > -10e-8)
                return true;
        }
    }
    return false;
}

bool Group::wait_timeout()
{
	sort_container();
	WaitEvent *wait_event = dynamic_cast<WaitEvent *>(container.back());
	if (wait_event != nullptr) {
		SyscallEvent *syscall = dynamic_cast<SyscallEvent *>(wait_event->get_syscall());
		if (syscall != nullptr && syscall->get_ret() != 0)
			return true;
	}
	return false;
}

bool Group::execute_over()
{
	sort_container();
	this->calculate_time_span();
	if (time_span > 1500000)
		return true;
	return false;
}

void Group::decode_group(std::ofstream &output)
{    
    sort_container();
    for (auto cur_ev : container) {
        cur_ev->decode_event(0, output);
    }
}

void Group::streamout_group(std::ofstream &output)
{    
    sort_container();
    for (auto cur_ev : container) {
        cur_ev->streamout_event(output);
    }
}

void Group::streamout_group(std::ofstream &output, EventBase *in, EventBase *out)
{
	output << "Group " << std::hex << group_id << std::endl;
	sort_container();
	EventCategory event_checker;
	auto it = container.begin();
	while (*it != in && it != container.end()) it++;
	while (it != container.end()) {
		EventBase *cur_ev = *it;
		if (event_checker.is_semantics_event(cur_ev->get_event_type()))
			cur_ev->streamout_event(output);
		else if (event_checker.is_structure_event(cur_ev->get_event_type())) {
			cur_ev->EventBase::streamout_event(output);
			output << std::endl;
		}
		if (*it == out)
			break;
		it++;
	}
}

void Group::streamout_group(std::ostream &output)
{    
    sort_container();
    std::list<EventBase*>::iterator it;
    int index = 0;
    for (it = container.begin(); it != container.end(); it++) {
        EventBase *cur_ev = *it;
        output << std::hex << index++ << "\t";
        cur_ev->streamout_event(output);
        output << std::endl;
    }
}

void Group::pic_group(std::ostream &output)
{
    if (container.size() == 0) {
        LOG_S(INFO) << "Group " << std::hex << group_id << " has no events ";
        return;
    }
    
    output << std::hex << group_id << "\t" << get_procname() << "_" << get_first_event()->get_tid() << std::endl;
    output << "Time " << std::fixed << std::setprecision(1) << get_first_event()->get_abstime();
    output << " ~ " << std::fixed << std::setprecision(1) << get_last_event()->get_abstime() << std::endl;
}

bool cmp(std::pair<std::string, int>& a, 
         std::pair<std::string, int>& b) 
{ 
    return a.second > b.second; 
} 
  
std::vector<std::pair<std::string, int> > local_sort(std::unordered_map<std::string, int> M) 
{ 
    std::vector<std::pair<std::string, int> > A; 
    for (auto& it : M) { 
        A.push_back(it); 
    } 
    std::sort(A.begin(), A.end(), cmp); 
	return A;
} 

void Group::recent_backtrace()
{
	for (auto rit = container.rbegin(); rit != container.rend(); rit++) {
		if ((*rit)->get_event_type() != BACKTRACE_EVENT)
            continue;
        BacktraceEvent *bt = dynamic_cast<BacktraceEvent *>(*rit);
        assert(bt != nullptr);
        if (bt->get_frames().size() > 0 && bt->get_sym(0).size() > 0) {
            bt->log_event();
            break;
        }
	}
}

void Group::busy_backtrace()
{
	LOG_S(INFO) << "Busy Callstack : (top frame(API) may contains infinite loop)";
	auto stacks = deepest_common_apis();
	int i = 0;
	for (auto elem: stacks) {
        if (i == 0)
		    LOG_S(INFO) << "[busy API] " << std::hex << i++ << "\t" << elem;
	    LOG_S(INFO) << std::hex << i++ << "\t" << elem;
	}
	if (i == 0)
		LOG_S(INFO) << "No common busy APIS found";
}

std::list<std::string> Group::deepest_common_apis()
{
	std::unordered_map<std::string, int> sym_freq;
	std::unordered_map<std::string, int> local_freq;
	std::list<std::string> commons;
	BacktraceEvent *bt = NULL, *check = NULL;
	int bt_num = 0;
    int index = 0;

	sym_freq.clear();
	commons.clear();

    for (auto event: container) {
        if (++index >= container.size())
            break;
        auto next_event = event_at(index);
		if (next_event->get_event_type() != INTR_EVENT || event->get_event_type() != BACKTRACE_EVENT)
			continue;
		bt = dynamic_cast<BacktraceEvent *>(event);
		assert(bt != nullptr);
		bt_num++;
		local_freq.clear();
		for (auto frame: bt->get_frames_info()) {
			local_freq[frame.symbol]++;
		}
		for (auto elem: local_freq) {
			sym_freq[elem.first]++;
		}
		check = bt;
	}

	if (bt_num == 0) {
		LOG_S(INFO) << "No backtrace event in node";
		return commons;
	}
	
	if (check != nullptr) {
		for (auto frame: check->get_frames_info()) {
			if (sym_freq[frame.symbol] >= bt_num * 0.95)
				commons.push_back(frame.symbol);
		}
		if (commons.size() > 0) {
			LOG_S(INFO) << "DEBUG: symbols size = " << commons.size() << ", freqs = bt_num = " << bt_num;
			return commons;
		}
	}

	int max_freq = 0;
	for (auto elem: sym_freq) {
		int freq = elem.second;
		assert(freq <= bt_num);
		if (freq > max_freq) 
			max_freq = freq;
		LOG_S(INFO) << "DEBUG: " << elem.first << "\t" << elem.second << " VS " << bt_num;
	}
	LOG_S(INFO) << "DEBUG: max_freq in local group = " << max_freq << " while bt_num = " << bt_num; 
	LOG_S(INFO) << "DEBUG: symbols size = " << sym_freq.size();
	auto result = local_sort(sym_freq);
	for (auto elem: result) {
		commons.push_back(elem.first);
	}
	return commons;
}
