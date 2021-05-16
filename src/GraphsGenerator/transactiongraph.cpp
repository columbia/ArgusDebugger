#include "graph.hpp"
#define CONNECT_TIMER_CANCEL	0

TransactionGraph::TransactionGraph(uint64_t gid)
:Graph()
{
    if (!groups_ptr) return;
    root = groups_ptr->get_group_by_gid(gid);
    if (!root) return;
    id = root->get_cluster_id();
    main_nodes.clear();
    init_graph();
}

TransactionGraph::TransactionGraph(Groups *_groups_ptr, uint64_t gid)
:Graph(_groups_ptr)
{
    if (!groups_ptr) return;
    root = groups_ptr->get_group_by_gid(gid);
    if (!root) return;
    id = root->get_cluster_id();
    main_nodes.clear();
    init_graph();
}

TransactionGraph::~TransactionGraph()
{
    root_node = end_node = nullptr;
}

Edge *TransactionGraph::get_outgoing_connection_for_msg(Node *node, MsgEvent *msg_event)
{
    MsgEvent *next_msg = msg_event->get_next();
    MsgEvent *peer_msg = dynamic_cast<MsgEvent*>(msg_event->get_event_peer());

    if (msg_event->get_header()->check_recv() == false) {
		if (peer_msg == nullptr)
			return nullptr;

        Edge *e = new Edge(msg_event, peer_msg, MSGP_REL);
        if (!node->add_out_edge(e))
            delete e;
		else
			return e;
    } else {
		if (next_msg == nullptr)
			return nullptr;
        Edge *e = new Edge(msg_event, next_msg, MSGP_REL);
        if (!node->add_out_edge(e))
            delete e;
		else
			return e;
    } 
	return nullptr;
}

Edge *TransactionGraph::get_outgoing_connection_for_disp_enq(Node *node, BlockEnqueueEvent *enq_event)
{
    BlockInvokeEvent *inv_event = dynamic_cast<BlockInvokeEvent *>(enq_event->get_consumer());
    if (!inv_event)
        return nullptr;
    Edge *e = new Edge(enq_event, inv_event, DISP_EXE_REL);
    if (!node->add_out_edge(e))
        delete e;
	else
		return e;
	return nullptr;
}

Edge *TransactionGraph::get_outgoing_connection_for_callcreate(Node *node, TimerCreateEvent *timer_create)
{
    TimerCalloutEvent *timercallout_event = timer_create->get_called_peer();
    if (timercallout_event) {
        Edge *e = new Edge(timer_create, timercallout_event, TIMERCALLOUT_REL);
        if (!node->add_out_edge(e))
            delete e;
		else
			return e;
    }
#if CONNECT_TIMER_CANCEL
    TimerCancelEvent *timercancel_event = timer_create->get_cancel_peer();
    if (timercancel_event) {
        Edge *e = new Edge(timer_create, timercancel_event, TIMERCANCEL_REL);
        if (!node->add_out_edge(e))
            delete e;
        else
            return e;
    }
#endif
	return nullptr;
}

Edge *TransactionGraph::get_outgoing_connection_for_caset(Node *node, CASetEvent *ca_set_event)
{
    CADisplayEvent *ca_display_event = ca_set_event->get_display_event();
    if (ca_display_event) {
        Edge *e = new Edge(ca_set_event, ca_display_event, CA_REL);
        if (!node->add_out_edge(e))
            delete e;
		else
			return e;
    }
	return nullptr;
}

Edge *TransactionGraph::get_outgoing_connection_for_rlboundary(Node *node, RunLoopBoundaryEvent *rl_event)
{
    EventBase *consumer = rl_event->get_consumer();
    if (consumer != nullptr) {
        Edge *e = new Edge(rl_event, consumer, RLITEM_REL);
        if (!node->add_out_edge(e))
            delete e;
		else
			return e;
    }
	return nullptr;
}

Edge *TransactionGraph::get_outgoing_connection_for_hwbr(Node *node, BreakpointTrapEvent *event)
{
    BreakpointTrapEvent *rw_peer = dynamic_cast<BreakpointTrapEvent *>(event->get_event_peer());
    if (rw_peer == nullptr)
        return nullptr;
    if (event->check_read() == false) {
        Edge *e = new Edge(event, rw_peer, HWBR_REL);
        if (!node->add_out_edge(e))
            delete e;
		else
			return e;
    }
	return nullptr;
}

Edge *TransactionGraph::get_outgoing_connection_for_mkrun(Node *node, MakeRunEvent *mkrun_event)
{
    EventBase *peer_event = mkrun_event->get_event_peer();
    if (peer_event) {
        Edge *e = new Edge(mkrun_event, peer_event, MKRUN_REL);
        if (!node->add_out_edge(e))
            delete e;
		else
			return e;
    }
	return nullptr;
}

Edge *TransactionGraph::add_weak_edge(Node *node)
{
#if ALLOW_WEAK_EDGE
	Group *cur_group = node->get_group();
    Group *next_group = groups_ptr->get_group_by_gid(node->get_gid() + 1);

    if (!(cur_group->is_divide_by_msg() & DivideOldGroup)
        && !(cur_group->is_divide_by_disp() & DivideOldGroup)
        && !(cur_group->is_divide_by_wait() & DivideOldGroup)
        )
        return nullptr;

	if (next_group == nullptr)
		return nullptr;

    EventBase *from_event = cur_group->get_last_event();
    EventBase *peer_event = next_group->get_first_event();
    if (peer_event) {
        Edge *e = new Edge(from_event, peer_event, WEAK_REL);
        if (!node->add_out_weak_edge(e))
            delete e;
        else
            return e;
    }
#endif
    return nullptr;
}


std::map<Group *, Edge *> TransactionGraph::get_outgoing_connections(Node *node, EventBase *event_after)
{
	Group *cur_group = node->get_group();
	std::list<EventBase *> &lists = cur_group->get_container();
	std::list<EventBase *> ::iterator it;
	std::map<Group *, Edge *> ret;

	ret.clear();
	it = find(lists.begin(), lists.end(), event_after);
	if (it == lists.end())
		return ret;

	for (; it != lists.end(); it++) {
		EventBase *event = *it;
		Edge *edge = nullptr;
        switch (event->get_event_type()) {
            case MSG_EVENT:
                edge = get_outgoing_connection_for_msg(node, dynamic_cast<MsgEvent *>(event));
                break;
            case DISP_ENQ_EVENT:
                edge = get_outgoing_connection_for_disp_enq(node, dynamic_cast<BlockEnqueueEvent *>(event));
                break;
            case TMCALL_CREATE_EVENT:
               	edge = get_outgoing_connection_for_callcreate(node, dynamic_cast<TimerCreateEvent *>(event));
                break;
            case CA_SET_EVENT:
                edge = get_outgoing_connection_for_caset(node, dynamic_cast<CASetEvent *>(event));
                break;
            case RL_BOUNDARY_EVENT: {
                RunLoopBoundaryEvent *rlboundary_event = dynamic_cast<RunLoopBoundaryEvent *>(event);
				edge = get_outgoing_connection_for_rlboundary(node, rlboundary_event);
                break;
            }
            case BREAKPOINT_TRAP_EVENT:
                edge = get_outgoing_connection_for_hwbr(node, dynamic_cast<BreakpointTrapEvent *>(event));
                break;
            case MR_EVENT:
                edge = get_outgoing_connection_for_mkrun(node, dynamic_cast<MakeRunEvent *>(event));
                break;
            default:
                break;
        }

		if (edge != nullptr) {
			assert(dynamic_cast<EventBase*>(edge->e_to));
			Group *peer_group = groups_ptr->group_of(edge->e_to);
			if (ret.find(peer_group) == ret.end() && peer_group != cur_group)
                ret[peer_group] = edge;
		}
	}// end of for

#if ALLOW_WEAK_EDGE
	Edge *weak_edge = add_weak_edge(node);
	if (weak_edge) {
		assert(dynamic_cast<EventBase*>(weak_edge->e_to));
		Group *peer_group = groups_ptr->group_of(weak_edge->e_to);
		if (ret.find(peer_group) == ret.end())
			ret[peer_group] = weak_edge;
	}
#endif

	return ret;
}

void TransactionGraph::BFS(Node *node, EventBase *event_after)
{
	std::map<Group *, Edge *> outgoing = get_outgoing_connections(node, event_after);
    for (auto element: outgoing) {
        if (element.first->contains_nsappevent()) {
            LOG_S(INFO) << "APPEVENT in " << std::hex << element.first->get_group_id()\
                << " connected to " << std::hex << root->get_group_id()\
                << " edge type " << std::hex << element.second->rel_type\
                << " from " << std::fixed << std::setprecision(1)\
                << element.second->e_from->get_abstime() << std::endl; 
            continue;
        }
        Node *to_node = add_node(element.first);
        if (to_node == nullptr)
            continue;
         search_node[to_node] = element.second->e_to;
    }
}

Node *TransactionGraph::add_node(Group *cur_group)
{
	if (cur_group == nullptr)
		return nullptr;
    if (nodes_map.find(cur_group) != nodes_map.end()
        ||search_group.find(cur_group) != search_group.end())
        return nullptr;

    Node *node = new Node(this, cur_group);
    if (node == nullptr) {
        LOG_S(INFO) << "OOM" << std::endl;
        return node;
    }

    assert(check_and_add_node(node) == nullptr);
    cur_group->set_cluster_id(id);
    search_group[cur_group] = node;

    if (node->is_node_from_thread(get_main_tid())) {
        main_nodes[node->get_gid()] = node;
        end_node = node->contains_view_update() ? node : end_node;
    }
    return node;
}


Node *TransactionGraph::augment_node(Group *cur_group, EventBase *event_after)
{
    Node *node = add_node(cur_group);
    if (node == nullptr)
        return nullptr;
	//BFS search to generate transaction DAG
    search_node[node] = event_after;
    while(search_node.size() > 0) {
        Node *target = search_node.begin()->first;
        EventBase *event_after = search_node.begin()->second;
        BFS(target, event_after);
        search_node.erase(target);
    }
    return node;
}

tid_t TransactionGraph::get_main_tid()
{
	tid_t ret = Graph::get_main_tid();
	return ret == (tid_t)-1 ? root->get_tid() : ret;
}

void TransactionGraph::init_graph()
{
    //from root_node expand to end_node
	if (root == nullptr) return;

    uint64_t cur_gid = root->get_group_id();
    bool begin = false;
	end_node = nullptr;
    
    LOG_S(INFO)<< "Begin augment graph from input node id 0x" << std::hex << cur_gid << " ..." << std::endl;
	for(Group *next = groups_ptr->get_group_by_gid(cur_gid); next != nullptr;
		++cur_gid, next = groups_ptr->get_group_by_gid(cur_gid)) {
        if (end_node) {
            LOG_S(INFO) << "0x" << std::hex << end_node->get_gid() << " updates views in the graph" << std::endl;
            break;
        }
        if (begin == true && next->contains_nsappevent()) {
            LOG_S(INFO) << "0x" << std::hex << cur_gid << " has another AppKit event" << std::endl;
            break;
        }
        if (main_nodes.find(cur_gid) != main_nodes.end()) {
            LOG_S(INFO) << "0x" << std::hex << cur_gid << " has been included in the graph" << std::endl;
            continue;
        }

        LOG_S(INFO) << "Augment main thread node 0x" << std::hex << cur_gid << " ..." << std::endl;
		EventBase *event_after = next->get_first_event();
	    Node *cur_node = augment_node(next, event_after);
        if (begin == false) {
            root_node = cur_node;
            begin = true;
			continue;
        } 

#if ALLOW_WEAK_EDGE
        if (begin == true){
            Edge *e = add_weak_edge(main_nodes[cur_gid -1]);
			if (e)
				LOG_S(INFO) << "WeakEdge from 0x" << std::hex << cur_gid - 1 \
							<< " to 0x" << std::hex << cur_gid << std::endl;
        }
#endif
	} 

    end_node = get_end_node();
    LOG_S(INFO) << "Finish augment graph";
    LOG_S(INFO) << " from 0x" << std::hex << root_node->get_gid();
    if (end_node != nullptr)
        LOG_S(INFO) << " to 0x" << std::hex << end_node->get_gid();
    LOG_S(INFO) << std::endl; 
    graph_statistics();
}

Node *TransactionGraph::get_end_node()
{
    if (end_node != nullptr)
        return end_node;
    for (auto rit = main_nodes.rbegin(); rit != main_nodes.rend(); rit++)
        if (rit->second->contains_view_update())
            end_node = main_nodes[rit->first];
    return end_node;
}
