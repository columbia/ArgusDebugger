#include "graph.hpp"

EventGraph::EventGraph()
	:Graph()
{
	if (!groups_ptr) return;
	init_graph();
}

EventGraph::EventGraph(Groups *groups)
	:Graph(groups)
{
	init_graph();
}

EventGraph::~EventGraph()
{
}

typedef std::unique_ptr<boost::asio::io_service::work> asio_worker;

void EventGraph::add_edge_for_msg(Node *node, MsgEvent *msg_event)
{
	MsgEvent *next_msg = msg_event->get_next();
	MsgEvent *prev_msg = msg_event->get_prev();
	MsgEvent *peer_msg = dynamic_cast<MsgEvent*>(msg_event->get_event_peer());

	if (next_msg != nullptr) {
		Edge *e = new Edge(msg_event, next_msg, MSGP_NEXT_REL);
		if (!node->add_out_edge(e))
			delete e;
	} else if (prev_msg != nullptr) {
		Edge *e = new Edge(prev_msg, msg_event, MSGP_NEXT_REL);
		if (!node->add_in_edge(e))
			delete e;
	}

	if (peer_msg == nullptr)
		return;
	if (msg_event->get_header()->check_recv() == true) {
		Edge *e = new Edge(peer_msg, msg_event, MSGP_REL);
		if (!node->add_in_edge(e))
			delete e;

	} else {
		Edge *e = new Edge(msg_event, peer_msg, MSGP_REL);
		if (!node->add_out_edge(e))
			delete e;
	}
}

void EventGraph::add_edge_for_disp_enq(Node *node, BlockEnqueueEvent *enq_event)
{
	BlockInvokeEvent *invoke_event = dynamic_cast<BlockInvokeEvent *>(enq_event->get_consumer());
	if (!invoke_event)
		return;
	Edge *e = new Edge(enq_event, invoke_event, DISP_EXE_REL);
	if (!node->add_out_edge(e))
		delete e;
}

void EventGraph::add_edge_for_disp_invoke(Node *node, BlockInvokeEvent *invoke_event)
{
	BlockEnqueueEvent *enq_event = dynamic_cast<BlockEnqueueEvent *>(invoke_event->get_root());
	if (!enq_event)
		return;
	Edge *e = new Edge(enq_event, invoke_event, DISP_EXE_REL);
	if (!node->add_in_edge(e))
		delete e;
}

void EventGraph::add_edge_for_callcreate(Node *node, TimerCreateEvent *timer_create)
{
	TimerCalloutEvent *timercallout_event = timer_create->get_called_peer();
	if (timercallout_event) {
		Edge *e = new Edge(timer_create, timercallout_event, TIMERCALLOUT_REL);
		if (!node->add_out_edge(e))
			delete e;
	}
#ifdef CALLCANCEL
	TimerCancelEvent *timercancel_event = timer_create->get_cancel_peer();
	if (timercancel_event) {
		Edge *e = new Edge(timer_create, timercancel_event, TIMERCANCEL_REL);
		if (!node->add_out_edge(e))
			delete e;
	}
#endif
}

void EventGraph::add_edge_for_callout(Node *node, TimerCalloutEvent *timer_callout)
{
	TimerCreateEvent *timer_create = timer_callout->get_timercreate();
	if (timer_create) {
		Edge *e = new Edge(timer_create, timer_callout, TIMERCALLOUT_REL);
		if (!node->add_in_edge(e))
			delete e;
	}
}

void EventGraph::add_edge_for_callcancel(Node *node, TimerCancelEvent *timer_cancel)
{
#ifdef CALLCANCEL
	TimerCreateEvent *timer_create = timer_cancel->get_timercreate();
	if (timer_create) {
		Edge *e = new Edge(timer_create, timer_cancel, TIMERCANCEL_REL);
		if (!node->add_in_edge(e))
			delete e;
	}
#endif
}

#ifdef CA_REL
void EventGraph::add_edge_for_caset(Node *node, CASetEvent *ca_set_event)
{
	CADisplayEvent *ca_display_event = ca_set_event->get_display_event();
	if (ca_display_event) {
		Edge *e = new Edge(ca_set_event, ca_display_event, CA_REL);
		if (!node->add_out_edge(e))
			delete e;
	}
}

void EventGraph::add_edge_for_cadisplay(Node *node, CADisplayEvent *ca_display_event)
{
	for (auto ca_set_event : ca_display_event->get_ca_set_events()) {
		assert(ca_set_event);
		Edge *e = new Edge(ca_set_event, ca_display_event, CA_REL);
		if (!node->add_in_edge(e))
			delete e;
	}
}
#endif

void EventGraph::add_edge_for_rlitem(Node *node, RunLoopBoundaryEvent *rl_event)
{
	EventBase *owner = rl_event->get_owner();
	if (owner != nullptr) {
		Edge *e = new Edge(owner, rl_event, RLITEM_REL);
		if (!node->add_in_edge(e))
			delete e;

	}
	EventBase *consumer = rl_event->get_consumer();
	if (consumer != nullptr) {
		Edge *e = new Edge(rl_event, consumer, RLITEM_REL);
		if (!node->add_out_edge(e))
			delete e;
	}
}

void EventGraph::add_edge_for_hwbr(Node *node, BreakpointTrapEvent *event)
{
	BreakpointTrapEvent *rw_peer = dynamic_cast<BreakpointTrapEvent *>(event->get_event_peer());
	if (rw_peer == nullptr)
		return;
	if (event->check_read()) {
		Edge *e = new Edge(rw_peer, event, HWBR_REL);
		if (!node->add_in_edge(e))
			delete e;
	} else {
		Edge *e = new Edge(event, rw_peer, HWBR_REL);
		if (!node->add_out_edge(e))
			delete e;
	}
}

void EventGraph::add_edge_for_mkrun(Node *node, MakeRunEvent *mkrun_event)
{
	EventBase *peer_event = mkrun_event->get_event_peer();
	if (!peer_event)
		return;
	bool is_weak = mkrun_event->is_weak();
	if (is_weak) {
#if ALLOW_WEAK_EDGE
		Edge *e = new Edge(mkrun_event, peer_event, WEAK_MKRUN_REL);
		if (!node->add_out_weak_edge(e))
			delete e;
#endif
	} else {
		Edge *e = new Edge(mkrun_event, peer_event, MKRUN_REL);
		if (!node->add_out_edge(e))
			delete e;
	}
}

void EventGraph::add_edge_for_fakedwoken(Node *node, FakedWokenEvent *fakewoken)
{
	MakeRunEvent *mkrun_event = dynamic_cast<MakeRunEvent*>(fakewoken->get_event_peer());
	if (!mkrun_event || mkrun_event->get_group_id() == (uint64_t)(-1))
		return;

	assert(mkrun_event->get_event_type() == MR_EVENT);
	assert(fakewoken->get_event_type() == FAKED_WOKEN_EVENT);
	assert(fakewoken->get_group_id() != (uint64_t)(-1));

	bool is_weak = mkrun_event->is_weak();
	if (is_weak) {
#if ALLOW_WEAK_EDGE
		Edge *e = new Edge(mkrun_event, fakewoken, WEAK_MKRUN_REL);
		if (!node->add_in_weak_edge(e))
			delete e;
		weak_mkrun_edge++;
#endif
	} else {
		Edge *e = new Edge(mkrun_event, fakewoken, MKRUN_REL);
		assert(e);
		if (!node->add_in_edge(e))
			delete e;
	}
}

bool EventGraph::similarity_on_stack(Group *cur, Group* peer)
{
	std::set<std::string> cur_sym = cur->get_syms_freq_under(100);
	std::set<std::string> peer_sym = peer->get_syms_freq_under(100);
	std::set<std::string> intersect;
	std::set<std::string> unionset;
	std::set_intersection(cur_sym.begin(), cur_sym.end(),
			peer_sym.begin(), peer_sym.end(), std::inserter(intersect, intersect.begin()));
	if (intersect.size() > 3)
		return true;
	return false;
}

Edge *EventGraph::add_weak_edge(Node *node)
{
#if ALLOW_WEAK_EDGE
	Group *cur_group = node->get_group();
	Group *next_group = groups_ptr->get_group_by_gid(node->get_gid() + 1);
	Edge *e = nullptr;

	if (next_group == nullptr 
		|| nodes_map.find(next_group) == nodes_map.end()) {
		return nullptr;
	}

	EventBase *from_event = cur_group->get_last_event();
	EventBase *peer_event = next_group->get_real_first_event();
	WaitEvent *wait = dynamic_cast<WaitEvent *>(from_event);

	if (!from_event || !peer_event) {
		return nullptr;
	}
	assert(from_event->get_tid() == peer_event->get_tid());

	/* Weak Edge Optimization */
	/* Invalid weak edge:
	   1. kernel weak edges : heart beat or delegate
	   2. BSC_workq_kernreturn
	   3. callstacks unsimilarity
	 */

	if (from_event->get_procname() == "kernel_task"
		&& wait != nullptr 
		&& wait->get_mkrun()
		&& (wait->get_mkrun()->is_timeout()
			|| wait->get_mkrun()->is_external())){
		weak_edge_kernel++;
		return nullptr;
	}

	if (cur_group->is_divide_by_wait()
			&& wait != nullptr
			&& wait->get_syscall() 
			&& wait->get_syscall()->get_op() == "BSC_workq_kernreturn") {
		weak_wait_edge_BSC_workq++;
		return nullptr;
	}

	if (similarity_on_stack(cur_group, next_group) == false) {
		e = new Edge(from_event, peer_event, WEAK_REL);
		weak_edge_diff_stacks++;
	} else {
		if (/*known batching APIs*/ false) {
			return nullptr;
		}

		e = new Edge(from_event, peer_event, WEAK_BOOST_REL);
		if (cur_group->is_divide_by_msg())
			weak_msg_edge_boost++;
		if (cur_group->is_divide_by_wait())
			weak_wait_edge_boost++;
	} 

	bool new_edge = node->add_out_weak_edge(e);
	if (!new_edge) {
		delete e;
		return nullptr;
	}

	auto it = nodes_map.find(next_group);
	return e;
#endif

	return nullptr;
}

void EventGraph::add_edges(Node *node)
{
	for (auto event : node->get_group()->get_container()) {
		switch (event->get_event_type()) {
			case MSG_EVENT:
				add_edge_for_msg(node, dynamic_cast<MsgEvent *>(event));
				break;
			case DISP_ENQ_EVENT:
				add_edge_for_disp_enq(node, dynamic_cast<BlockEnqueueEvent *>(event));
				break;
			case DISP_INV_EVENT:
				add_edge_for_disp_invoke(node, dynamic_cast<BlockInvokeEvent *>(event));
				break;
			case TMCALL_CREATE_EVENT:
				add_edge_for_callcreate(node, dynamic_cast<TimerCreateEvent *>(event));
				break;
			case TMCALL_CALLOUT_EVENT:
				add_edge_for_callout(node, dynamic_cast<TimerCalloutEvent *>(event));
				break;
			case TMCALL_CANCEL_EVENT:
				add_edge_for_callcancel(node, dynamic_cast<TimerCancelEvent *>(event));
				break;
			case CA_SET_EVENT:
				add_edge_for_caset(node, dynamic_cast<CASetEvent *>(event));
				break;
			case CA_DISPLAY_EVENT:
				add_edge_for_cadisplay(node, dynamic_cast<CADisplayEvent *>(event));
				break;
			case RL_BOUNDARY_EVENT: {
				RunLoopBoundaryEvent *rlboundary_event = dynamic_cast<RunLoopBoundaryEvent *>(event);
				add_edge_for_rlitem(node, rlboundary_event);
				break;
			}
			case BREAKPOINT_TRAP_EVENT:
				add_edge_for_hwbr(node, dynamic_cast<BreakpointTrapEvent *>(event));
				break;
			case MR_EVENT:
				add_edge_for_mkrun(node, dynamic_cast<MakeRunEvent *>(event));
				break;
			case FAKED_WOKEN_EVENT:
				add_edge_for_fakedwoken(node, dynamic_cast<FakedWokenEvent*>(event));
				break;
			default:
				break;
		}
	} //end of for
}

void EventGraph::optimize_mkrun_edges()
{
	std::vector<Edge *> dup_edges;
	dup_edges.clear();

	for (auto edge: edges) {
		if (edge->rel_type != MKRUN_REL)
			continue;
		Node *from = edge->from;
		Node *to = edge->to;
		for (auto node_edge: from->get_out_edges()) {
			if (node_edge.second->to == to && node_edge.second != edge) {
				dup_edges.push_back(edge);
				break;
			}
		}
	}
#if ALLOW_WEAK_EDGE
	dup_mkrun = dup_edges.size();
#endif
}

void EventGraph::complete_vertices_for_edges()
{
	for (auto cur_edge : edges) {
		assert(cur_edge);
		assert(cur_edge->e_from && cur_edge->e_to);
		Group *cur_edge_from_group = groups_ptr->group_of(cur_edge->e_from);
		Group *cur_edge_to_group = groups_ptr->group_of(cur_edge->e_to);
		if (!cur_edge_from_group || !cur_edge_to_group)
			continue;
		Node *cur_edge_from = nodes_map[cur_edge_from_group];
		Node *cur_edge_to = nodes_map[cur_edge_to_group];
		assert(cur_edge_from && cur_edge_to);
		if (cur_edge->from != cur_edge_from)
			cur_edge->from = cur_edge_from;
		if (cur_edge->to != cur_edge_to)
			cur_edge->to = cur_edge_to;
	}
}

void EventGraph::init_graph()
{
	time_t time_begin, time_check;
	time(&time_begin);
	LOG_S(INFO) << "Begin generate graph...";

	for (auto it : groups_ptr->get_groups()) {
		Group* cur_group = it.second;
		Node *node = new Node(this, cur_group);
		nodes.push_back(node);
		nodes_map[cur_group] = node;
	}

	boost::asio::io_service ioService;
	boost::thread_group threadpool;
	asio_worker work(new asio_worker::element_type(ioService));
	for (int i = 0 ; i < LoadData::meta_data.nthreads; i++)
		threadpool.create_thread(boost::bind(&boost::asio::io_service::run, &ioService));
	for (auto node : nodes) {
		ioService.post(boost::bind(&EventGraph::add_edges, this, node));
	}
	work.reset();
	threadpool.join_all();

	for (auto node : nodes) {
		add_weak_edge(node);
	}
	optimize_mkrun_edges();
	complete_vertices_for_edges();
	time(&time_check);
	LOG_S(INFO) << "graph generation costs " << std::fixed << std::setprecision(1)\
		<< difftime(time_check, time_begin) << " seconds";
	graph_statistics();
}
