#include "group.hpp"
#include "thread_divider.hpp"
#include <time.h>

#define DEBUG_GROUP    0

typedef std::unique_ptr<boost::asio::io_service::work> asio_worker;

pid_t ListsCategory::tid2pid(uint64_t tid)
{	
	pid_t ret = -1;
	assert(LoadData::tpc_maps.find(tid) != LoadData::tpc_maps.end());
	if (LoadData::tpc_maps[tid] != nullptr)
        ret = LoadData::tpc_maps[tid]->get_pid();
	return ret;
}

void ListsCategory::prepare_list_for_tid(tid_t tid, tid_evlist_t &tid_list_map)
{
	if (LoadData::tpc_maps.find(tid) == LoadData::tpc_maps.end())
		LoadData::tpc_maps[tid] = nullptr;

    if (tid_list_map.find(tid) != tid_list_map.end())
        return;

    event_list_t l;
    l.clear();
    tid_list_map[tid] = l;
}

// A sequential iteration of all events to prepare maps 
// for lock free multi-thread processing 
void ListsCategory::threads_wait_graph(event_list_t &ev_list)
{
    tid_evlist_t tid_list_map;
    std::map<uint64_t, MakeRunEvent *> who_make_me_run_map;
    MakeRunEvent *mr_event;
    uint64_t tid, peer_tid;

    event_list_t::iterator it;
    for (it = ev_list.begin(); it != ev_list.end(); it++) {
		assert((*it)->get_event_type() > 0);
        tid = (*it)->get_tid();
        prepare_list_for_tid(tid, tid_list_map);
		assert(tid_list_map.find(tid) != tid_list_map.end());
        if (who_make_me_run_map.find(tid) != who_make_me_run_map.end()) {
            FakedWokenEvent *i_am_running_now = new FakedWokenEvent((*it)->get_abstime() - 0.01,
                "faked_woken", (*it)->get_tid(), who_make_me_run_map[tid],
                (*it)->get_coreid(), (*it)->get_procname());
            who_make_me_run_map[tid]->set_event_peer(i_am_running_now);
            tid_list_map[tid].push_back(i_am_running_now);
            ev_list.insert(it, i_am_running_now);
            who_make_me_run_map.erase(tid);
        }

        tid_list_map[tid].push_back(*it);
		assert((*it)->get_event_type() > 0);

		if ((*it)->get_event_type() == MR_EVENT) {
        	mr_event = dynamic_cast<MakeRunEvent *>(*it);
			assert(mr_event);
            peer_tid = mr_event->get_peer_tid();
            who_make_me_run_map[peer_tid] = mr_event;
        }
    }
    tid_lists = tid_list_map;
}

bool ListsCategory::interrupt_mkrun_pair(EventBase *cur, event_list_t::reverse_iterator rit)
{
    EventBase *pre = *(++rit);

    if (cur->get_event_type() != MR_EVENT)
        return false;
    
    switch (pre->get_event_type()) {
        case INTR_EVENT: {
            MakeRunEvent *mkrun = dynamic_cast<MakeRunEvent *>(cur);
            IntrEvent *interrupt = dynamic_cast<IntrEvent *>(pre);
            return mkrun->check_interrupt(interrupt);
        }
        case TSM_EVENT:
            return true;
        default:
            return false;
    }
}

int ListsCategory::remove_sigprocessing_events(event_list_t &event_list, event_list_t::reverse_iterator rit)
{
    event_list_t::iterator remove_pos;
    int sigprocessing_seq_reverse[] = {MSG_EVENT,
                    MSG_EVENT,
                    SYSCALL_EVENT,
                    BACKTRACE_EVENT, /*get_thread_state*/
                    WAIT_EVENT,
                    MR_EVENT,
                    MSG_EVENT}; /*wake up by cpu signal from kernel thread*/

    int i = 0, remove_count = 0, n = sizeof(sigprocessing_seq_reverse) / sizeof(int);

    while (rit != event_list.rend() && (*rit)->get_event_type() != MSG_EVENT)
        rit++;

    for (; rit != event_list.rend() && i < n; i++) {
    retry:
        while (rit != event_list.rend()) {
            if ((*rit)->get_event_type() == FAKED_WOKEN_EVENT) {
                remove_pos = next(rit).base();
                assert(*remove_pos == *rit);
                event_list.erase(remove_pos);
                remove_count++;
            } else if ((*rit)->get_event_type() == INTR_EVENT) {
                rit++;
            } else if (interrupt_mkrun_pair((*rit), rit)) {
                rit++;
                if (rit == event_list.rend()) 
                    return remove_count;
                rit++;
            } else {
                break;
            }
        }
        
        if (rit == event_list.rend()) 
            return remove_count;

        if (sigprocessing_seq_reverse[i] == WAIT_EVENT) {
            if (((*rit)->get_event_type() == MR_EVENT && !interrupt_mkrun_pair((*rit), rit)) 
                || (*rit)->get_event_type() == MSG_EVENT) {
                continue;
            } else if ((*rit)->get_op() == "MSC_mach_reply_port") {
                remove_pos = next(rit).base();
                assert(*remove_pos == *rit);
                event_list.erase(remove_pos);
                remove_count++;
                if (rit == event_list.rend()) 
                    return remove_count;
                goto retry;    
            } 
        }

        if ((*rit)->get_event_type() == sigprocessing_seq_reverse[i]) {
            remove_pos = next(rit).base();
            assert(*remove_pos == *rit);
            event_list.erase(remove_pos);
            remove_count++;
        } else {
            if (sigprocessing_seq_reverse[i] == MR_EVENT && (*rit)->get_event_type() == MSG_EVENT) {
                MsgEvent *msg_event = dynamic_cast<MsgEvent *>(*rit);
                if (msg_event->get_header()->check_recv() == false) { 
                    remove_pos = next(rit).base();
                    assert(*remove_pos == *rit);
                    event_list.erase(remove_pos);
                    remove_count++;
                } 
            }
            return remove_count;
        }
    }
    return remove_count;
}

ProcessInfo *ListsCategory::get_process_info(tid_t tid, EventBase *back)
{
    assert(LoadData::tpc_maps.find(tid) != LoadData::tpc_maps.end());
	if (LoadData::tpc_maps[tid] == nullptr) {
		ProcessInfo *ret = new ProcessInfo(tid, tid2pid(tid), back->get_procname());
		LoadData::tpc_maps[tid] = ret;
    } 

 	if (LoadData::tpc_maps[tid]->get_procname() == "" && tid2pid(tid) != -1) {
		std::string procname = LoadData::pid2comm(tid2pid(tid));
		if (procname != "") {
			LoadData::tpc_maps[tid]->override_procname(procname);
		}
	}
	return LoadData::tpc_maps[tid];
}

void ListsCategory::update_events_in_thread(uint64_t tid)
{
    event_list_t &tid_list = tid_lists[tid]; 
	if (tid_list.size() == 0) {
		LOG_S(INFO) << "thread list with size 0";
		return;
	}
    ProcessInfo *p = get_process_info(tid, tid_list.back());
    event_list_t::iterator it;
    EventBase *event, *prev = nullptr;

    for (it = tid_list.begin(); it != tid_list.end(); it++) {
        event = *it;
        event->update_process_info(*p);
        event->set_event_prev(prev);
        prev = event;
        /* remove events related to hwbr trap signal processing */
        if (event->get_event_type() == BREAKPOINT_TRAP_EVENT) {
            int dist = distance(tid_list.begin(), it);
            event_list_t::reverse_iterator rit(next(it));
            assert(*rit == *it);
            int removed_count = remove_sigprocessing_events(tid_list, rit);
            dist -= removed_count;
            assert(dist <= tid_list.size());
            it = tid_list.begin();
            advance(it, dist);
            assert((*it) == event);
        }
        /*remove Interrupt/timeshare_maintainance event waken*/
        if (event->get_event_type() == TSM_EVENT
            && next(it) != tid_list.end()
            && (*(next(it)))->get_event_type() == MR_EVENT) {
            it = tid_list.erase(it); // TSM
            assert((*it)->get_event_type() == MR_EVENT);
            it = tid_list.erase(it); // MR
			erased_tsm++;
            if (it != tid_list.begin())
                it = std::prev(it);
            event = *it;
        }

retry:
        if (event->get_event_type() == INTR_EVENT 
            && next(it) != tid_list.end()
            && (*(next(it)))->get_event_type() == MR_EVENT) {
            MakeRunEvent *mkrun = dynamic_cast<MakeRunEvent *>(*(next(it)));
            IntrEvent *interrupt = dynamic_cast<IntrEvent *>(event);
            if (mkrun->check_interrupt(interrupt) == false)
               continue;
            it++;// INTR
            assert((*it)->get_event_type() == MR_EVENT);
            it = tid_list.erase(it); // MR
			erased_intr++;
            if (it != tid_list.begin())
                it = std::prev(it);
            event = *it;
            goto retry;
        } 
    }
}
//////////////////////////////////////////

void ThreadType::check_rlthreads(event_list_t &rlboundary_list)
{
    std::set<uint64_t> rl_entries;
    for (auto event: rlboundary_list) {
        auto rl_boundary_event = dynamic_cast<RunLoopBoundaryEvent *>(event);
        tid_t tid = rl_boundary_event->get_tid();
        if (rl_entries.find(tid) != rl_entries.end())
            continue;
        if (rl_boundary_event->get_state() == ItemBegin)
            rl_entries.insert(tid);
    }
    categories[RunLoopThreadType] = rl_entries;

}

bool ThreadType::is_runloop_thread(tid_t tid)
{
	if (categories.find(RunLoopThreadType) != categories.end()
		&& categories[RunLoopThreadType].find(tid) != categories[RunLoopThreadType].end())
		return true;
	return false;
}

EventBase *ThreadType::ui_event_thread(event_list_t &backtrace_list)
{
    std::string nsthread("NSEventThread");
	for (auto event: backtrace_list) {
		if (event->get_procname() != LoadData::meta_data.host)
			continue;
		BacktraceEvent *backtrace_event = dynamic_cast<BacktraceEvent *>(event);
		if (backtrace_event->spinning())
			return backtrace_event;
	} 

	for (auto event: backtrace_list) {
		if (event->get_procname() != LoadData::meta_data.host)
			continue;
		BacktraceEvent *backtrace_event = dynamic_cast<BacktraceEvent *>(event);
		if (backtrace_event->contains_symbol(nsthread))
			return backtrace_event;
	}
	
	return nullptr;
}

void ThreadType::check_host_uithreads(event_list_t &backtrace_list)
{
	if (LoadData::meta_data.host == "Undefined") {
		LOG_S(INFO) << "Host is not set.";
		return;
	}
	EventBase *nsevent = ui_event_thread(backtrace_list);

	if (nsevent == nullptr) {
		LOG_S(INFO) << "UI event thread is not detected.";
	} else {
		nsevent_thread = nsevent->get_tid();
		LOG_S(INFO) << "UI event thread " << std::hex << nsevent_thread;
		LOG_S(INFO) << "Corresponding process " << std::dec << nsevent->get_pid();
	}

    std::string mainthread("[NSApplication run]");
    std::string mainthreadobserver("MainLoopObserver");
    std::string mainthreadsendevent("SendEventToEventTargetWithOptions");

	for (auto event : backtrace_list) {
		BacktraceEvent *backtrace_event = dynamic_cast<BacktraceEvent *>(event);
		if (backtrace_event->contains_symbol(mainthread)
				|| backtrace_event->contains_symbol(mainthreadobserver)
				|| backtrace_event->contains_symbol(mainthreadsendevent)) {
			if (event->get_procname() != LoadData::meta_data.host
					|| (nsevent && event->get_pid() != nsevent->get_pid())
					|| (nsevent && event->get_tid() == nsevent_thread)) {
				continue;
			}
			main_thread = event->get_tid();
			break;
		}
	}
	if (nsevent != nullptr)
		LOG_S(INFO) << "Process " << std::dec << nsevent->get_pid();
    LOG_S(INFO) << "Main thread 0x" << std::hex << main_thread;
    LOG_S(INFO) << "Event thread 0x" << std::hex << nsevent_thread;
}


/////////////////////////////////////////
Groups::Groups(EventLists *eventlists_ptr)
:ListsCategory(eventlists_ptr->get_event_lists()), ThreadType()
{
    groups.clear();
    main_groups.clear();
    host_groups.clear();
    sub_results.clear();
	empty_groups.clear();
	
	for (auto element : tid_lists) {
        gid_group_map_t temp;
        temp.clear();
        sub_results[element.first] = temp;
    }
    init_groups();
}

Groups::Groups(op_events_t &_op_lists)
:ListsCategory(_op_lists), ThreadType()
{
    groups.clear();
    main_groups.clear();
    host_groups.clear();
    sub_results.clear();
	empty_groups.clear();

	for (auto element : tid_lists) {
        gid_group_map_t temp;
        temp.clear();
        sub_results[element.first] = temp;
    }

    init_groups();
}

Groups::Groups(Groups &copy_groups)
:ListsCategory(copy_groups.get_op_lists()), ThreadType()
{
    sub_results.clear();
    groups = copy_groups.get_groups();
    main_groups = copy_groups.get_main_groups();
    host_groups = copy_groups.get_host_groups();
	empty_groups.clear();

    Group *this_group, *copy_group;
	for (auto element: groups) {
        copy_group = element.second;
        this_group = new Group(*copy_group);
        groups[element.first] = this_group;
        if (main_groups.find(element.first) != main_groups.end())
            main_groups[element.first] = this_group;
        if (host_groups.find(element.first) != host_groups.end())
            host_groups[element.first] = this_group;
    } 
}

Groups::~Groups(void)
{
    Group *cur_group;
	for (auto element : groups) {
        cur_group = element.second;
        assert(cur_group);
        delete(cur_group);
    }
    groups.clear();
    main_groups.clear();
    host_groups.clear();
    sub_results.clear();
}

void Groups::init_groups()
{

    para_preprocessing_tidlists();
    para_identify_thread_types();
    para_connector_generate();
	update_voucher_list(get_list_of_op(MACH_IPC_VOUCHER_INFO));
	para_thread_divide();
}

void Groups::update_voucher_list(event_list_t voucher_list)
{
    std::list<EventBase *>::iterator it;
    VoucherEvent *voucher_info;

    for (it = voucher_list.begin(); it != voucher_list.end(); it++) {
        voucher_info = dynamic_cast<VoucherEvent *>(*it);

        pid_t bank_holder = voucher_info->get_bank_holder();
        if (bank_holder != -1)
            voucher_info->set_bank_holder_name(LoadData::pid2comm(bank_holder));

        pid_t bank_merchant = voucher_info->get_bank_merchant();
        if (bank_merchant != -1) 
            voucher_info->set_bank_merchant_name(LoadData::pid2comm(bank_merchant));

        pid_t bank_orig = voucher_info->get_bank_orig();
        if (bank_orig != -1)
            voucher_info->set_bank_orig_name(LoadData::pid2comm(bank_orig));

        pid_t bank_prox = voucher_info->get_bank_prox();
        if (bank_prox != -1)
            voucher_info->set_bank_prox_name(LoadData::pid2comm(bank_prox));
    }
}

void Groups::para_preprocessing_tidlists(void)
{
    boost::asio::io_service ioService;
    boost::thread_group threadpool;
    asio_worker work(new asio_worker::element_type(ioService));
    
    for (int i = 0 ; i < LoadData::meta_data.nthreads; i++)
        threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));

    tid_evlist_t::iterator it;
    for (it = tid_lists.begin(); it != tid_lists.end(); it++)
        ioService.post(boost::bind(&ListsCategory::update_events_in_thread, this, it->first));
    work.reset();
    threadpool.join_all();
}

void Groups::para_identify_thread_types(void)
{
    boost::asio::io_service ioService;
    boost::thread_group threadpool;
    asio_worker work(new asio_worker::element_type(ioService));
    
    for (int i = 0 ; i < LoadData::meta_data.nthreads; i++)
        threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));

    ioService.post(boost::bind(&ThreadType::check_host_uithreads, this,
				get_list_of_op(BACKTRACE)));

    ioService.post(boost::bind(&ThreadType::check_rlthreads, this,
				get_list_of_op(RL_BOUNDARY)));
    
    work.reset();
    threadpool.join_all();
}

void Groups::para_connector_generate(void)
{
	boost::asio::io_service ioService;
	boost::thread_group threadpool;
	asio_worker work(new asio_worker::element_type(ioService));

	for (int i = 0 ; i < LoadData::meta_data.nthreads; i++)
		threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));

	/*pair mach msg*/
	MsgPattern msg_pattern(get_list_of_op(MACH_IPC_MSG));
	ioService.post(boost::bind(&MsgPattern::collect_patterned_ipcs, msg_pattern));

	/*pair dispatch queue*/
	DispatchPattern dispatch_pattern(get_list_of_op(DISP_ENQ),
			get_list_of_op(DISP_EXE));
	ioService.post(boost::bind(&DispatchPattern::connect_dispatch_patterns,
				dispatch_pattern));

	/*pair timer callout*/
	TimerCallPattern timercall_pattern(get_list_of_op(MACH_CALLCREATE),
			get_list_of_op(MACH_CALLOUT),
			get_list_of_op(MACH_CALLCANCEL));
	ioService.post(boost::bind(&TimerCallPattern::connect_timercall_patterns,
				timercall_pattern));

	/*match core animation*/
	CoreAnimationConnection core_animation_connection (get_list_of_op(CA_SET),
			get_list_of_op(CA_DISPLAY));
	ioService.post(boost::bind(&CoreAnimationConnection::core_animation_connection,
			core_animation_connection ));

	/*match breakpoint_trap connection*/
	BreakpointTrapConnection breakpoint_trap_connection(get_list_of_op(BREAKPOINT_TRAP));
	ioService.post(boost::bind(&BreakpointTrapConnection::breakpoint_trap_connection,
				breakpoint_trap_connection));

	/*match rl work*/
	RunLoopTaskConnection runloop_connection(get_list_of_op(RL_BOUNDARY), tid_lists);
	ioService.post(boost::bind(&RunLoopTaskConnection::runloop_connection, runloop_connection));

	/*fill wait -- makerun info*/
	MakeRunnableWaitPair mkrun_wait_pair(get_list_of_op(MACH_WAIT),
			get_list_of_op(MACH_MK_RUN));
	ioService.post(boost::bind(&MakeRunnableWaitPair::pair_wait_mkrun,  mkrun_wait_pair));

    work.reset();
    threadpool.join_all();
}

void Groups::para_thread_divide(void)
{
    boost::asio::io_service ioService;
    boost::thread_group threadpool;
    asio_worker work(new asio_worker::element_type(ioService));

    for (int i = 0 ; i < LoadData::meta_data.nthreads; i++)
        threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));

    std::vector<uint64_t> host_idxes;
    host_idxes.clear();

	for (auto element : tid_lists) {
        if ((element.second).size() == 0)
            continue;
        if (LoadData::pid2comm(tid2pid(element.first)) == LoadData::meta_data.host)
            host_idxes.push_back(element.first);

		if (is_runloop_thread(element.first)) {
            RunLoopThreadDivider rl_thread_divider(element.first, sub_results, element.second);
            ioService.post(boost::bind(&RunLoopThreadDivider::divide, rl_thread_divider));
		} else {
            ThreadDivider general_thread_divider(element.first, sub_results, element.second);
            ioService.post(boost::bind(&ThreadDivider::divide, general_thread_divider));
		}
    }
    
    work.reset();
    threadpool.join_all();
    
	for (auto element : sub_results)
		collect_groups(element.second);
    
	for (auto idx : host_idxes) 
		host_groups.insert(sub_results[idx].begin(), sub_results[idx].end());

	if (sub_results.find(main_thread) != sub_results.end())
		main_groups = sub_results[main_thread];
}

void Groups::collect_groups(gid_group_map_t &sub_groups)
{
    if (sub_groups.size())
        groups.insert(sub_groups.begin(), sub_groups.end());
}

Group *Groups::group_of(EventBase *event)
{
    assert(event);
    uint64_t gid = event->get_group_id();
    if (groups.find(gid) != groups.end()) { 
        return groups[gid];
    } 
    return nullptr;
}

bool Groups::repeated_timeout(std::vector<Group *> &candidates)
{
	int n = 0;
	double wait_time = 0.0;
	for (auto element : candidates) {
		if (element->wait_timeout()) {
			n++;
			wait_time += element->wait_time();
		}
	}

	if (n > 1 && wait_time > 1000000)
		return true;
	return false;
}

Group *Groups::spinning_group()
{
//special for detect spinning beachcall
	Group *ret = nullptr;
    if (nsevent_thread == -1) {
        return nullptr;
    }

    double nsthread_spin_timestamp  = 0.0;
    event_list_t backtrace_events = get_list_of_op(BACKTRACE);
    for (auto event : backtrace_events) {
        if (event->get_tid() != nsevent_thread)
			continue;
        BacktraceEvent *backtrace = dynamic_cast<BacktraceEvent *>(event);
        if (backtrace->spinning() == true) {
            nsthread_spin_timestamp = backtrace->get_abstime();
			break;
        }
    }
    if (nsthread_spin_timestamp > 0.0) {
		auto mainthread_groups = sub_results[main_thread];
		double countdown_begin_timestamp = nsthread_spin_timestamp - 1500000;

		std::vector<Group *> candidates;
		for (auto element: mainthread_groups) {
			double group_begin = element.second->get_first_event()->get_abstime();
			double group_end = element.second->get_last_event()->get_abstime();
			Group *next = nullptr;
			if ((next = get_group_by_gid(element.second->get_gid() + 1)) != nullptr)
				group_end = next->get_first_event() ? next->get_first_event()->get_abstime() : group_end;
			else
				group_end = LoadData::meta_data.last_event_timestamp;
			if (group_begin < nsthread_spin_timestamp && group_end > countdown_begin_timestamp){
				candidates.push_back(element.second);
			}
		}
		for (auto element: candidates) {
			if (element->wait_over())
				ret = element;
		}
		if (repeated_timeout(candidates)) {
			ret = candidates.front();
		}
		if (ret == nullptr && candidates.size() > 0) {
			ret = candidates.front();
		}
    }
    return ret;
}

bool Groups::matched(Group *target)
{
	for (auto element: groups) {
        if (*(element.second) == *target) {
            return true;
        }
    }
    return false;
}

void Groups::partial_compare(Groups *peer_groups, std::string proc_name, std::string &output_path)
{
    std::ofstream output(output_path);
	for (auto element: groups) {
        if (proc_name.size() != 0 && element.second->get_first_event()->get_procname() != proc_name)
            continue;
            if (!peer_groups->matched(element.second)) {
                element.second->streamout_group(output);        
            }
    }
    output.close();
}

int Groups::decode_groups(std::string &output_path)
{
    std::ofstream output(output_path);
    output << "Total number of Groups = " << groups.size() << std::endl << std::endl;
	for (auto element: groups) {
        Group *cur_group = element.second;
        output << "#Group " << std::hex << cur_group->get_group_id();
        output << "(length = " << std::hex << cur_group->get_size() << "):\n";
        cur_group->decode_group(output);
    }
    output.close();
    return 0;
}

int Groups::streamout_groups(std::string &output_path)
{
    std::ofstream output(output_path);
    output << "Total number of Groups = " << groups.size() << std::endl;
	for (auto element: groups) {
        Group *cur_group = element.second;
        output << "#Group " << std::hex << cur_group->get_group_id();
        output << "(length = " << std::hex << cur_group->get_size() <<"):\n";
        cur_group->streamout_group(output);
    }
    output.close();
    return 0;
}
