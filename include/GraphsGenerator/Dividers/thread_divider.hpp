#ifndef THREAD_DIVIDER_HPP
#define THREAD_DIVIDER_HPP

#include <stack>

#include "group.hpp"

#define DEBUG_THREAD_DIVIDER 0

#define HEURISTICS 1
#define HEURISTICS_MSG 	1
#define HEURISTICS_MIG 	1
#define HEURISTICS_WAIT 1

#define OVERRIDE_DISP 0

#define THD_BACKTRACE   	1
#define THD_TIMERCREATE 	2
#define THD_TIMERCANCEL 	3
#define THD_TIMERCALL    	4

class DividerHelper {
public:
    VoucherEvent *voucher_for_hook;
	SyscallEvent *syscall_event;
    FakedWokenEvent *faked_wake_event;
    std::stack<DispatchQueueMIGServiceEvent *> dispatch_mig_servers;
    std::stack<BlockInvokeEvent *> current_disp_invokers;

	DividerHelper():
		voucher_for_hook(nullptr),
		syscall_event(nullptr),
		faked_wake_event(nullptr){}
};

class ThreadDivider: public DividerHelper{
public:
	typedef std::map<uint64_t, Group*> groups_t;
	typedef std::map<uint64_t, groups_t> divides_t;
	typedef std::list<EventBase *> event_list_t;

protected:
	groups_t local_result;
    Group *cur_group;

    uint64_t gid_base;
    uint64_t index;
    event_list_t tid_list;
	divides_t &submit_result;

    Group *create_group(uint64_t id, EventBase *root_event);
    Group *gid2group(uint64_t gid);
    void delete_group(Group *del_group);

    void store_event_to_group_handler(EventBase *event);
    void add_general_event_to_group(EventBase *event);
	void add_backtrace_event_to_group(EventBase *event);
    void add_tsm_event_to_group(EventBase *event);
    bool matching_mr_syscall(MakeRunEvent *event);
    void add_mr_event_to_group(EventBase *event);
    bool matching_wait_syscall(WaitEvent *wait);
    void add_wait_event_to_group(EventBase *event);
    void add_timercallout_event_to_group(EventBase *event);
    void add_disp_invoke_begin_event(BlockInvokeEvent *invoke_event);
    void add_disp_invoke_end_event(BlockInvokeEvent *invoke_event, BlockInvokeEvent *begin_invoke);
    void add_disp_invoke_event_to_group(EventBase *event);
    void check_hooks(MsgEvent *msg_event);
    void add_events(MsgEvent *msg_event, bool need_calculate);
    void add_msg_event_into_group(EventBase *event);
    void add_disp_mig_event_to_group(EventBase *event);
    std::set<std::string> calculate_msg_peer_set(MsgEvent *, VoucherEvent *);

public:
    uint32_t msg_induced_node;
    uint32_t wait_block_disp_node;

    ThreadDivider(int index, divides_t &sub_results, event_list_t ev_list);
    void decode_groups(groups_t &local_groups, std::string filepath);
    void streamout_groups(groups_t &local_groups, std::string filepath);

    virtual void divide();
};

class RunLoopThreadDivider: public ThreadDivider {
    Group *save_cur_rl_group_for_invoke;
    Group *rl_event_process;
    BlockInvokeEvent *invoke_in_rl;

public:
    RunLoopThreadDivider(int index, divides_t &sub_results, event_list_t &ev_list);
    ~RunLoopThreadDivider();
    void add_nsappevent_event_to_group(EventBase *event);
    void add_rlboundary_event_to_group(EventBase *event);
    void decode_groups(std::string filepath) {ThreadDivider::decode_groups(local_result, filepath);}
    void streamout_groups(std::string filepath) {ThreadDivider::streamout_groups(local_result, filepath);}
    virtual void divide();
};

#endif
