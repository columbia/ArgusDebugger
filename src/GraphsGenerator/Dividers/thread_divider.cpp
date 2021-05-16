#include "thread_divider.hpp"

ThreadDivider::ThreadDivider(int _index, divides_t &global_results, event_list_t _ev_list)
:DividerHelper(), submit_result(global_results)
{
    gid_base = _ev_list.front()->get_tid() << GROUP_ID_BITS;
    tid_list = _ev_list;
    index = _index;
    local_result.clear();
    cur_group = nullptr;
    msg_induced_node = wait_block_disp_node = 0;
}

Group *ThreadDivider::create_group(uint64_t id, EventBase *root_event)
{
    Group *new_gptr = new Group(id, root_event);
    if (new_gptr == nullptr) {
        exit(EXIT_FAILURE);
    }
    if (root_event) 
        new_gptr->add_to_container(root_event);
    return new_gptr;
}

Group *ThreadDivider::gid2group(uint64_t gid)
{
    if (local_result.find(gid) != local_result.end())
        return local_result[gid];
    return nullptr;
}

void ThreadDivider::delete_group(Group *del_group)
{
	assert(del_group);
    del_group->empty_container();
    delete(del_group);
}

void ThreadDivider::add_general_event_to_group(EventBase *event)
{
	EventBase *peer_event = event->get_event_peer();
	int event_type = event->get_event_type();
	std::string peer = "";

#define PEERS 1
#if PEERS
	if (event->get_event_type() != WAIT_EVENT
		&& peer_event != nullptr
		&& peer_event->get_pid() != event->get_pid())
		peer = peer_event->get_procname() + std::to_string(peer_event->get_pid());

	//checking peer set
	if (cur_group != nullptr
		&& peer.size() > 0
		&& event_type != MSG_EVENT
		&& event_type != FAKED_WOKEN_EVENT
		&& event_type != TMCALL_CANCEL_EVENT) {
		auto peer_set = cur_group->get_group_peer();
		if (peer_set.size() > 0
			&& peer_set.find(peer) == peer_set.end())
			cur_group = nullptr;
	}
#endif

    if (cur_group == nullptr) {
        cur_group = create_group(gid_base + local_result.size(), nullptr);
        local_result[cur_group->get_group_id()] = cur_group;
    }
	
	//update peer(process in std::string) of current group
	if (peer.size() > 0)
		cur_group->add_group_peer(peer);

	//faked waken event should be added with other events to avoid empty group
    if (faked_wake_event) {
        cur_group->add_to_container(faked_wake_event);
        faked_wake_event = nullptr;
    }
    cur_group->add_to_container(event);

	//update signature with callstack of current group
	if (event->get_event_type() == BACKTRACE_EVENT) {
		BacktraceEvent *backtrace_event = dynamic_cast<BacktraceEvent *>(event);
        for (int i = 0; i < backtrace_event->get_size(); i++)
            cur_group->add_group_tags(backtrace_event->get_sym(i), backtrace_event->get_freq(i));
	}
}

void ThreadDivider::add_backtrace_event_to_group(EventBase *event)
{
	add_general_event_to_group(event);
}

void ThreadDivider::store_event_to_group_handler(EventBase *event)
{
    switch (event->get_event_type()) {
        case VOUCHER_EVENT:
            voucher_for_hook = dynamic_cast<VoucherEvent *>(event);
            break;
        case FAKED_WOKEN_EVENT:
            faked_wake_event = dynamic_cast<FakedWokenEvent *>(event);
            break;
        default:
            break;
    }
}

void ThreadDivider::divide()
{
	for (auto event: tid_list) {
        switch (event->get_event_type()) {
            case VOUCHER_EVENT:
            case FAKED_WOKEN_EVENT:
                store_event_to_group_handler(event);
                break;
            case BACKTRACE_EVENT:
				add_backtrace_event_to_group(event);
				break;
            // traditional heurisitcs
            case DISP_INV_EVENT:
                add_disp_invoke_event_to_group(event);
                break;

            case TMCALL_CALLOUT_EVENT:
                add_timercallout_event_to_group(event);
                break;

#if HEURISTICS
#if HEURISTICS_MIG
            case DISP_MIG_EVENT: {
                add_disp_mig_event_to_group(event);
                break;
            }
#endif
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
        }
    }
    submit_result[index] = local_result;
}

void ThreadDivider::decode_groups(groups_t &local_groups, std::string filepath)
{
    std::ofstream output(filepath);
    if (output.fail()) {
        LOG_S(INFO) << "Error: unable to open file " << filepath << std::endl;
        return;
    }
	
    uint64_t index = 0;
	for (auto item: local_groups) {
        output << "#Group " << std::hex << index++ << std::endl;
       	item.second->decode_group(output);
    }
    output.close();
}

void ThreadDivider::streamout_groups(groups_t &local_groups, std::string filepath)
{
    std::ofstream output(filepath);
    if (output.fail()) {
        LOG_S(INFO) << "Error: unable to open file " << filepath << std::endl;
        return;
    }

    uint64_t index = 0;
	for (auto item: local_groups) {
        output << "#Group " << std::hex << index++ << std::endl;
        item.second->streamout_group(output);
    }
    output.close();
}
