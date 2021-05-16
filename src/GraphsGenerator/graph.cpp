#include "graph.hpp"
#include <math.h>
#include <boost/filesystem.hpp>

Edge::Edge(EventBase *e1, EventBase *e2, uint32_t rel)
{
    e_from = e1;
    e_to = e2;
    rel_type = rel;
    from = to = nullptr;
    if (e_to != nullptr && e_from != nullptr)
        weight = e_to->get_abstime() - e_from->get_abstime();
    else
        weight = -1.0;
}

std::string Edge::decode_edge_type()
{
    switch(rel_type) {
		case MSGP_REL:
            return "MSG";
		case MSGP_NEXT_REL:
            return "MSG_PASS";
		case MKRUN_REL:
            return "WAKE_THREAD";
		case DISP_EXE_REL:
            return "DISPATCH_EXE";
		case TIMERCALLOUT_REL:
            return "DELAYED_CALL";
		case RLITEM_REL:
            return "RUNLOOP_SRC";
		case CA_REL:
            return "COREANIMATE";
		case HWBR_REL:
            return "DATA_FLAG";
		case WEAK_BOOST_REL:
			return "WEAK_BOOST";
		case WEAK_MKRUN_REL:
            return "WEAK_WAKE";
		case TIMERCANCEL_REL:
            return "WEAK_CALLCANCEL";
		case WEAK_REL:
            return "WEAK";

		case IMPORT_REL:
            return "WEAK_IMPORT";
    }
    return "NONE";
}

Edge::~Edge()
{
    from = to = nullptr;
    e_from = e_to = nullptr;
}
//////////////////////////////////

Node::Node(Graph *_p, Group *_g)
:parent(_p), group(_g)
{
    out_edges.clear();
    in_edges.clear();
    in_weak_edges.clear();
    out_weak_edges.clear();
    in = out = 0;
    weight = get_end_time() - get_begin_time();
}

Node::~Node()
{
    parent = nullptr;
    group  = nullptr;
}

bool Node::add_in_edge(Edge *e)
{
	if (e->e_from->get_abstime() > e->e_to->get_abstime())
		return false;

    Edge *exist_edge = parent->check_and_add_edge(e);
    bool ret = exist_edge == nullptr ? true : false;
    if (exist_edge == nullptr) {
		assert(e->e_to->get_group_id() == get_gid());
		in_edges.insert(std::pair<EventBase *, Edge*> (e->e_to, e));
		e->set_edge_to_node(this);
    } else {
		assert(exist_edge->e_to == e->e_to);
		assert(exist_edge->e_to->get_group_id() == get_gid());
		in_edges.insert(std::pair<EventBase *, Edge*> (e->e_to, exist_edge));
		exist_edge->set_edge_to_node(this);
    }
	inc_in();
    return ret;
}

bool Node::add_in_weak_edge(Edge *e)
{
	assert(e);
	if (e->e_from->get_abstime() > e->e_to->get_abstime())
		return false;

    Edge *exist_edge = parent->check_and_add_edge(e);
    bool ret = exist_edge == nullptr ? true : false;
    if (exist_edge == nullptr) {
		assert(e);
        assert(e->e_to);
		std::pair<EventBase *, Edge *> item(e->e_to, e);
		in_weak_edges.insert(item);
		e->set_edge_to_node(this);
    } else {
		assert(exist_edge);
		assert(exist_edge->e_to);
		std::pair<EventBase *, Edge*> item(exist_edge->e_to, exist_edge);
		in_weak_edges.insert(item);
		exist_edge->set_edge_to_node(this);
    }
	inc_weak_in();
    return ret;
}

bool Node::add_out_edge(Edge *e)
{
	if (e->e_from->get_abstime() > e->e_to->get_abstime())
		return false;
    Edge *exist_edge = parent->check_and_add_edge(e);
    bool ret = exist_edge == nullptr ? true : false;
    if (exist_edge == nullptr) {
		std::pair<EventBase *, Edge*> item(e->e_from, e);
		out_edges.insert(item);
		e->set_edge_from_node(this);
    } else {
		std::pair<EventBase *, Edge*> item(e->e_to, exist_edge);
		out_edges.insert(item);
		exist_edge->set_edge_from_node(this);
    }
	inc_out();
    return ret;
}

bool Node::add_out_weak_edge(Edge *e)
{
	if (e->e_from->get_abstime() > e->e_to->get_abstime())
		return false;
    Edge *exist_edge = parent->check_and_add_edge(e);
    bool ret = exist_edge == nullptr ? true : false;
    if (exist_edge == nullptr) {
		out_weak_edges.insert(std::pair<EventBase *, Edge*> (e->e_from, e));
        e->set_edge_from_node(this);
    } else {
		out_weak_edges.insert(std::pair<EventBase *, Edge*> (e->e_from, exist_edge));
        exist_edge->set_edge_from_node(this);
    }
	inc_weak_out();
    return ret;
}

Node::edge_map_t &Node::get_in_edges()
{
    return in_edges;
}

Node::edge_map_t &Node::get_out_edges()
{
    return out_edges;
}

void Node::remove_edges()
{
    while (in_edges.size() > 0) {
        assert(in_edges.begin() != in_edges.end());
        std::pair<EventBase *, Edge*> edge = *(in_edges.begin());
        remove_edge(edge, false);
    }
    while (out_edges.size() > 0) {
        assert(out_edges.begin() != out_edges.end());
        std::pair<EventBase *, Edge*> edge = *(out_edges.begin());
        remove_edge(edge, true);
    }
    assert(in_edges.size() == 0 && out_edges.size() == 0);
}

bool Node::find_edge_in_map(std::pair<EventBase *, Edge *> target, bool out_edge)
{
	auto map = out_edge? out_edges: in_edges;
    
    if (map.find(target.first) != map.end()) {
        return true;
    }
    return false;
}

bool Node::find_edge_in_weak_map(std::pair<EventBase *, Edge *> target, bool out_edge)
{
	auto map = out_edge? out_weak_edges: in_weak_edges;
    auto range = map.equal_range(target.first);
    if (range.first != map.end()) {
        for (auto it = range.first; it != range.second; it++) {
           if (it->second == target.second)
                return true;
        }
    }
    return false;
}

void Node::remove_edge_weak(std::pair<EventBase *, Edge *> edge, bool out_edge)
{
    assert(edge.first != nullptr && edge.second != nullptr);
    assert(edge.second->e_from == edge.first || edge.second->e_to == edge.first);

    if (find_edge_in_weak_map(edge, out_edge) == false)
        return;

    Edge *e = edge.second;
    assert(e->is_weak_edge());
    
    if (e->from == e->to) {
        in_weak_edges.erase(e->e_to);
        out_weak_edges.erase(e->e_from);
        dec_weak_in();
        dec_weak_out();
    }
    else if (out_edge) {
        Node *peer = e->to;
        if (peer) {
            out_weak_edges.erase(e->e_from);
            peer->remove_edge_weak(std::make_pair(e->e_to, e), false);
            dec_weak_out();
        }
    } else {
        Node *peer = e->from;
        if (peer) {
            in_weak_edges.erase(e->e_to);
            peer->remove_edge_weak(std::make_pair(e->e_from, e), true);
            dec_weak_in();
        }
    }
    parent->remove_edge(e);
}


void Node::remove_edge(std::pair<EventBase *, Edge *> edge, bool out_edge)
{
    assert(edge.first != nullptr && edge.second != nullptr);
    assert(edge.second->e_from == edge.first || edge.second->e_to == edge.first);
    Edge *e = edge.second;
    if (e->is_weak_edge())
        remove_edge_weak(edge, out_edge);
    
    if (find_edge_in_map(edge, out_edge) == false)
        return;
    
    if (e->from == e->to) {
        in_edges.erase(e->e_to);
        out_edges.erase(e->e_from);
        dec_in();
        dec_out();
    }
    else if (out_edge) {
        Node *peer = e->to;
        if (peer) {
            out_edges.erase(e->e_from);
            peer->remove_edge(std::make_pair(e->e_to, e), false);
            dec_out();
        }
    } else {
        Node *peer = e->from;
        if (peer) {
            in_edges.erase(e->e_to);
            peer->remove_edge(std::make_pair(e->e_from, e), true);
            dec_in();
        }
    }
    parent->remove_edge(e);
}

int Node::compare_syscall_ret(Node *peer)
{
    return get_group()->compare_syscall_ret(peer->get_group());
}

int Node::compare_timespan(Node *peer)
{
    return get_group()->compare_timespan(peer->get_group());
}

int Node::compare_wait_cost(Node *peer)
{
    return get_group()->compare_wait_time(peer->get_group());
}

int Node::compare_wait_ret(Node *peer)
{
    return get_group()->compare_wait_ret(peer->get_group());
}

double Node::get_begin_time()
{
    return group->get_first_event()->get_abstime();
}

double Node::get_end_time()
{
    EventBase *last_event = group->get_last_event();
	assert(last_event);
    if (last_event->get_event_type() == WAIT_EVENT) {
        WaitEvent *wait = dynamic_cast<WaitEvent *>(last_event);
        MakeRunEvent* mkrun = dynamic_cast<MakeRunEvent *>(wait->get_event_peer());
        if (mkrun && mkrun->is_external())
            return mkrun->get_abstime();       
    }
    return last_event->get_abstime();
}

NSAppEventEvent* Node::contains_nsapp_event()
{
    return dynamic_cast<NSAppEventEvent *> (group->contains_nsappevent());
}

bool Node::contains_view_update()
{
    return group->contains_view_update() != nullptr;
}

bool Node::wait_over()
{
    return group->wait_over();
}

EventBase *Node::index_to_event(int index)
{
    std::list<EventBase *> &container = group->get_container();
    std::list<EventBase *>::iterator it = container.begin();

    if (index >= container.size() || index < 0)
        return nullptr;

    std::advance(it, index);
    return *it;    
}

void Node::show_event_detail(int index, std::ostream &out)
{
	EventBase *ev = index_to_event(index);
	if (ev)
		ev->streamout_event(out);
	else
		out << "Invalid index in current node 0x" << get_gid() << std::endl;
}

void Node::recent_backtrace()
{
	group->recent_backtrace();
}

void Node::busy_backtrace()
{
	group->busy_backtrace();
}
///////////////////////////////////
std::string Graph::create_output_path(std::string input_path)
{
    boost::filesystem::path outdir("./output/");
    if (!(boost::filesystem::exists(outdir)))  
        boost::filesystem::create_directory(outdir);

    std::string filename;
    size_t dir_pos = input_path.find_last_of("/");
    if (dir_pos != std::string::npos) {
        filename = input_path.substr(dir_pos + 1);
    } else {
        filename = input_path;
    }

    size_t pos = filename.find(".");
    if (pos != std::string::npos)
        return outdir.c_str() + filename.substr(0, pos);
    else
        return outdir.c_str() + filename;
}

Graph::Graph()
{
    std::string out_file = create_output_path(LoadData::meta_data.data);
    std::string event_output_path = out_file + "_events.stream";
    std::string group_output_path = out_file + "_groups.stream";

    event_lists_ptr = new EventLists(LoadData::meta_data);
    construct_inside = true;

    groups_ptr = new Groups(event_lists_ptr);
    create_graph = true;



	edges.clear();
	nodes.clear();
    nodes_map.clear();
    edge_map.clear();

#if ALLOW_WEAK_EDGE
	weak_edge_kernel = 0;
	weak_wait_edge_BSC_workq = 0;
	weak_edge_diff_stacks = 0;
	
    weak_wait_edge_boost= 0;
    weak_msg_edge_boost= 0;

    weak_mkrun_edge = 0;
    dup_mkrun = 0;
#endif
}

Graph::Graph(Groups *_groups_ptr)
{
    event_lists_ptr = nullptr;
    groups_ptr = _groups_ptr;
    construct_inside = false;
    create_graph = true;

	edges.clear();
	nodes.clear();
    nodes_map.clear();
    edge_map.clear();
#if ALLOW_WEAK_EDGE
	weak_edge_kernel = 0;
	weak_wait_edge_BSC_workq = 0;
	weak_edge_diff_stacks = 0;
	
    weak_wait_edge_boost= 0;
    weak_msg_edge_boost= 0;

    weak_mkrun_edge = 0;
    dup_mkrun = 0;
#endif
}

Graph::~Graph(void)
{
    nodes_map.clear();
    edge_map.clear();

    if (create_graph) {
		for (auto node : nodes) {
            assert(node);
            delete(node);
        }

		for (auto edge : edges) {
            assert(edge);
            delete(edge);
        }
    }

    if (construct_inside) {
        assert(groups_ptr && event_lists_ptr);
        delete groups_ptr;
        delete event_lists_ptr;
    }


    nodes.clear();
    edges.clear();
#if ALLOW_WEAK_EDGE
	weak_edge_kernel = 0;
	weak_wait_edge_BSC_workq = 0;
	weak_edge_diff_stacks = 0;
	
    weak_wait_edge_boost= 0;
    weak_msg_edge_boost= 0;

    weak_mkrun_edge = 0;
    dup_mkrun = 0;
#endif
    LOG_S(INFO) << "Clear graph";
}

// thread unsafe
void Graph::remove_node(Node *node)
{
    assert(node != nullptr);
    node->remove_edges();

	auto it = find(nodes.begin(), nodes.end(), node);
    assert(it != nodes.end() && *it == node);
    nodes.erase(it);

	auto it_1 = nodes_map.find(node->get_group());
	assert(it_1 != nodes_map.end() && it_1->second == node);
	nodes_map.erase(it_1);
	assert(find(nodes.begin(), nodes.end(), node) == nodes.end());
    delete node;
}

void Graph::remove_edge(Edge *edge)
{
    assert(edge != nullptr);

    if (edge_map.find(edge->e_from) != edge_map.end()) {
        edge_map.erase(edge->e_from);
    }

	auto it = find(edges.begin(), edges.end(), edge);
    if (it != edges.end()) {
        edges.erase(it);
		assert(find(edges.begin(), edges.end(), edge) == edges.end());
        delete edge;
    }
}

Node *Graph::check_and_add_node(Node *node)
{
    Node *ret = nullptr;
	if (nodes_map.find(node->get_group()) != nodes_map.end()) {
        ret = nodes_map[node->get_group()];
    } else {
        nodes.push_back(node);
		nodes_map[node->get_group()] = node;
    }
    return ret;
}

Edge *Graph::get_edge(EventBase *e_from, EventBase *e_to)
{
    Edge *ret = nullptr;
    graph_edge_mtx.lock();
	auto range = edge_map.equal_range(e_from);
	for (auto it = range.first; it != range.second; it++) {
		ret = it->second;
		if (ret->e_to == e_to)
			break;
		ret = nullptr;
	}
	graph_edge_mtx.unlock();
	return ret;
}

Edge *Graph::check_and_add_edge(Edge *e)
{
    Edge *ret = nullptr;
    graph_edge_mtx.lock();
	auto range = edge_map.equal_range(e->e_from);
	for (auto it = range.first; it != range.second; it++) {
		if (*(it->second) == *e) {
			ret = it->second;
			graph_edge_mtx.unlock();
			return ret;
		}
	}
    edge_map.emplace(e->e_from, e);
    edges.push_back(e);
    graph_edge_mtx.unlock();
    return ret;
} 

Node *Graph::recent_event_node(group_id_t gid)
{
	Node *ret = nullptr;

	for (; (gid & ((1 << GROUP_ID_BITS) - 1)) > 0; gid--) {

		ret = id_to_node(gid);
		if (ret == nullptr)
			continue;
		NSAppEventEvent *app_event = ret->contains_nsapp_event();
		if (app_event != NULL) {
			LOG_S(INFO) << "get ui event in node " << std::hex << gid;
			app_event->print_event();
			return ret;
		}
	}
	return nullptr;
}

Node *Graph::id_to_node(group_id_t gid)
{
	assert(groups_ptr);
    Group *id_to_group = groups_ptr->get_group_by_gid((uint64_t)gid);
    if (id_to_group == nullptr) {
        LOG_S(INFO) << "Unknown group id 0x" << std::hex << gid;
        return nullptr;
    }

    if (nodes_map.find(id_to_group) != nodes_map.end())
        return nodes_map[id_to_group];
    
    LOG_S(INFO)  << " Node 0x" << std::hex << gid << " not included in graph";
    return nullptr;
}

EventBase *Graph::get_event(group_id_t gid, int index)
{
	Node *node = id_to_node(gid);
	if (!node)
		return nullptr;
	return node->index_to_event(index);
}

std::vector<Node *> Graph::get_nodes_for_tid(tid_t tid)
{
    std::vector<Node *> result;
	result.clear();

	for (auto element : groups_ptr->get_groups_by_tid(tid)) {
        assert(nodes_map.find(element.second) != nodes_map.end());
        result.push_back(nodes_map[element.second]);
    }
    return result;
}

std::vector<Node *> Graph::nodes_between(Node *cur_node, double timestamp)
{
    std::vector<Node *> ret = get_nodes_for_tid(cur_node->get_tid());
	auto it = find(ret.begin(), ret.end(), cur_node);
    if (it == ret.end()) {
        ret.clear();
        return ret;
    }

    ret.erase(ret.begin(), ++it);
    for (it = ret.begin(); it != ret.end(); it++)
		if ((*it)->get_begin_time() > timestamp)
            break;
    ret.erase(it, ret.end());

    return ret;
}

Node *Graph::node_of(EventBase *event)
{
	Group *group = groups_ptr->group_of(event);
	if (group == nullptr)
		return nullptr;
	if (nodes_map.find(group) != nodes_map.end())
		return nodes_map[group];
	return nullptr;
}

Group *Graph::group_of(EventBase *event)
{
	if (groups_ptr == nullptr)
		return nullptr;
	return groups_ptr->group_of(event);
}

Node *Graph::normal_waken(Node *node)
{
    Node *cur_node = node;
    uint64_t gid = node->get_gid();
    while (cur_node != NULL) {
        WaitEvent *wait = dynamic_cast<WaitEvent *>(cur_node->get_group()->get_last_event());
        if (!wait || !(wait->get_mkrun())) {
			LOG_S(INFO) << "Not end with wait 0x" << std::hex << cur_node->get_gid(); 
            break;
		}
		MakeRunEvent *mr = wait->get_mkrun();
		assert(mr);
        if (!(mr->is_timeout()
				|| mr->is_external())) {
			LOG_S(INFO) << "Get not timeout node 0x" << std::hex << cur_node->get_gid() << " for 0x" << node->get_gid();
			return cur_node;
        }
		cur_node = id_to_node(++gid);
    }
    return NULL;
}

Node *Graph::get_spinning_node()
{
    Group *spinning_group = groups_ptr->spinning_group();
    if (spinning_group == nullptr)
        return nullptr;
    if (nodes_map.find(spinning_group) != nodes_map.end()) {
        assert(spinning_group);
        assert(nodes_map[spinning_group]);
        return nodes_map[spinning_group];
    }
    return nullptr;
} 

Node *Graph::get_last_node_in_main_thread()
{
    tid_t tid = get_main_tid();
    if (tid != -1) {
        return get_nodes_for_tid(tid).back();
    }
    return nullptr;
}

void Graph::graph_statistics()
{
    LOG_S(INFO) << "Scull out edges(node):";
	LOG_S(INFO) << "\ts1.time_share_maintanence " << std::dec << groups_ptr->get_wait_tsm();
	LOG_S(INFO) << "\ts2.interrupt " << std::dec << groups_ptr->get_wait_intr();
    LOG_S(INFO) << std::endl;

    LOG_S(INFO) << "Graph size: ";
    LOG_S(INFO) << "Nodes number = " << std::dec << nodes.size();

    const int msg = 0, wait = 1, mkrun = 2, disp = 3;
    std::map<int, int> weak_edge_categories;
    std::map<uint32_t, uint32_t> edge_categories;
    std::set<std::string> procs_set;
    std::vector<uint64_t> multin_grs; 
    std::map<int, int> multin_dist;
    std::map<int, int> peer_dist;

    edge_categories.clear();
    procs_set.clear();
    multin_grs.clear();
    multin_dist.clear();
    peer_dist.clear();
	
	for (auto cur_edge : edges) {
		edge_categories[cur_edge->rel_type]++;
		if (cur_edge->rel_type == WEAK_REL) {

			if (cur_edge->from->is_divide_by_msg() & DivideOldGroup) {
				weak_edge_categories[msg]++;
			}
			if (cur_edge->from->is_divide_by_wait() & DivideOldGroup) {
				weak_edge_categories[wait]++;
			}
			if (cur_edge->from->is_divide_by_disp() & DivideOldGroup) {
				weak_edge_categories[disp]++;
			}
		}

        if (cur_edge->rel_type == WEAK_MKRUN_REL)
            weak_edge_categories[mkrun]++;
    }

	for (auto node : nodes) {
        assert(node->get_group());
        if (node->get_in() > 1) {
            multin_grs.push_back(node->get_gid());
            multin_dist[node->get_in()]++;
        }
        peer_dist[node->get_group_peer().size()]++;
        procs_set.insert(node->get_procname());
    }

	int weak = 0, strong = 0;
	for (auto element : edge_categories) {
		if (element.first <= 8)
			strong += element.second;
		else
			weak += element.second;
    }

    LOG_S(INFO) << "Edges number = " << std::dec << edges.size() << " Strong edge "\
		<< strong << " , Weak edge " << weak;

#if ALLOW_WEAK_EDGE
    LOG_S(INFO) << std::endl;
	LOG_S(INFO) << "Weak Edge optimized with system call, call stack and kernel pattern";

	LOG_S(INFO) << "Weak Edge eliminated: ";
	LOG_S(INFO) << "\tw1. elminated kernel weak edge " << std::dec << weak_edge_kernel;
	LOG_S(INFO) << "\tw2. weak wait edges [task finished] " << std::dec << weak_wait_edge_BSC_workq;
	LOG_S(INFO) << "\tw3. lose similarity / known batching api in callstack " << std::dec << weak_edge_diff_stacks;
	LOG_S(INFO) << "\tw4. weak mkrun [thread pool management] " << std::dec << weak_mkrun_edge;
	LOG_S(INFO) << "\tw5. dup mkrun [wakeup to receive msg, disp and etc] " << std::dec << dup_mkrun; 
    LOG_S(INFO) << std::endl;

	LOG_S(INFO) << "Weak Edge boost with similar callstack: ";
	LOG_S(INFO) << "\tb1. boost weak edge between wait and wakeup " << std::dec << weak_wait_edge_boost;
	LOG_S(INFO) << "\tb2. boost weak_edge between different peer messages " << std::dec << weak_msg_edge_boost;
#endif

	LOG_S(INFO) << "Number of multiple-incoming-edge nodes = " << std::dec << multin_grs.size(); 
	for (auto elem: multin_dist)
        LOG_S(INFO) << "[" << std::dec << elem.first << "]\t" << std::dec << elem.second;
    LOG_S(INFO) << std::endl;

    LOG_S(INFO) << "Peer distribution";
    for (auto elem: peer_dist)
        LOG_S(INFO) << "[" << std::dec << elem.first << "]\t" << std::dec << elem.second;
    LOG_S(INFO) << std::endl;

	LOG_S(INFO) << "Number of procs = " << std::dec <<  procs_set.size(); 
	for (auto proc : procs_set) 
        LOG_S(INFO) << std::left << std::setw(32) << proc;
    LOG_S(INFO) << std::endl;
}

void Graph::direct_communicate_procs(std::string procname, std::ostream &out)
{
    std::set<std::string> procs_set;
	for (auto cur_edge : edges) {
		if (cur_edge->e_from->get_procname() == procname)
			procs_set.insert(cur_edge->e_to->get_procname());
		else if(cur_edge->e_to->get_procname() == procname)
			procs_set.insert(cur_edge->e_from->get_procname());
	}

	out << "Interaction to proc " << procname << ":\n";
	for (auto proc : procs_set)
		out << "\t" << proc << std::endl;
}

void Graph::streamout_nodes(std::string path) 
{
    std::string node_path(path + "_nodes");
    std::ofstream output(node_path);
    
	LOG_S(INFO) << "number of nodes = " << std::dec << nodes.size(); 
	for (auto node : nodes) {
        assert(node->get_group());
		output << "#Group " << node->get_gid() << std::endl;
        node->get_group()->streamout_group(output);
    }
    output.close();
}

void Graph::streamout_edges(std::string path)
{
    std::string edge_path(path + "_edges");
    std::ofstream output_edge(edge_path);

	LOG_S(INFO) << "number of edge = " << std::dec << edges.size(); 

	int i = 0;
	for (auto cur_edge : edges) {
		assert(cur_edge);
		assert(cur_edge->from);
        output_edge << "\t" << cur_edge->e_from->get_procname()\
        	<< ' ' << std::hex << cur_edge->from->get_gid()\
			<< ' ' << std::fixed << std::setprecision(1) << cur_edge->e_from->get_abstime()\
        	<< " -> "\
        	<< cur_edge->e_to->get_procname()\
        	<< ' ' << std::hex << cur_edge->e_to->get_group_id()\
			<< ' ' << std::fixed << std::setprecision(1) << cur_edge->e_to->get_abstime()\
			<< std::endl;
		++i;
	}
    output_edge.close();
}

void Graph::streamout_nodes_and_edges(std::string path)
{
    streamout_nodes(path);
    streamout_edges(path);
}

//////////////////////////////////////////////////////////////////////////////////////////////
void Graph::check_weak_edges(std::ofstream &out)
{
	out << "\ttime_share_m " << std::dec << groups_ptr->get_wait_tsm();
	out << "\tinterrupt " << std::dec << groups_ptr->get_wait_intr() << std::endl;
	
    const int msg = 0, wait = 1, mkrun = 2, disp = 3;
    std::map<int, std::vector<Edge *> > weak_edge_categories;
    std::vector<Edge *> empty;
	empty.clear();

    weak_edge_categories[msg] = empty;
    weak_edge_categories[wait] = empty;
    weak_edge_categories[mkrun] = empty;
    weak_edge_categories[disp] = empty;
    
	for (auto cur_edge : edges) {

		if (cur_edge->rel_type == WEAK_REL) {

			if (cur_edge->from->is_divide_by_msg() & DivideOldGroup) {
				weak_edge_categories[msg].push_back(cur_edge);
			}
			if (cur_edge->from->is_divide_by_wait() & DivideOldGroup) {
				weak_edge_categories[wait].push_back(cur_edge);
			}
			if (cur_edge->from->is_divide_by_disp() & DivideOldGroup) {
				weak_edge_categories[disp].push_back(cur_edge);
			}
		}

        if (cur_edge->rel_type == WEAK_MKRUN_REL)
            weak_edge_categories[mkrun].push_back(cur_edge);
    }

    out << "WeakEdge[MKRUN] = " << std::dec << weak_edge_categories[mkrun].size() << std::endl;
    out << "WeakEdge[MSG] = " << std::dec << weak_edge_categories[msg].size() << std::endl;
    out << "WeakEdge[WAIT] = " << std::dec << weak_edge_categories[wait].size() << std::endl;
    out << "WeakEdge[DISP] = " << std::dec << weak_edge_categories[disp].size() << std::endl;

    for (auto &v : weak_edge_categories) {
        out << "Weak Edge Type ";
        switch(v.first) {
            case msg: out << "MSG"; break;
            case wait: out << "WAIT"; break;
            case mkrun: out << "MKRUN"; break;
            case disp: out << "DISP"; break;
        }
        out << std::endl;

        for (auto cur_edge : v.second) {
            out << "Edge from 0x" << std::hex << cur_edge->from->get_gid()\
                << " to 0x" << std::hex << cur_edge->to->get_gid();
        	out << std::endl;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////
void Graph::check_heuristics(std::ofstream &out)
{
	out << "number of nodes = " << std::dec << nodes.size() << std::endl; 

	for (auto node : nodes) {
        assert(node->get_group());
        int divide_by_msg = node->get_group()->is_divide_by_msg();
        if (!divide_by_msg) continue;

        Group *peer_group = groups_ptr->get_group_by_gid(node->get_gid() + 1);
		if (peer_group && (divide_by_msg & DivideOldGroup)) {
			out << node->get_group()->get_procname()\
				<< " Node 0x" << std::hex << node->get_gid()\
				<< " and "\
				<< " Node 0x" << std::hex << peer_group->get_group_id()\
				<< " are divided by msg heuristics"\
				<< std::endl;

			for (auto proc : node->get_group_peer())
				out << "\t" << proc;
			out << std::endl;

			for (auto proc : peer_group->get_group_peer())
				out << "\t" << proc;
			out << std::endl;
		}

        peer_group = groups_ptr->get_group_by_gid(node->get_gid() - 1);
		if (peer_group && (divide_by_msg & DivideNewGroup)) {
			out << node->get_group()->get_procname()\
				<< " Node 0x" << std::hex << peer_group->get_group_id()\
				<< " and "\
				<< " Node 0x" << std::hex << node->get_gid()\
				<< " are divided by msg heuristics"\
				<< std::endl;

			for (auto proc : peer_group->get_group_peer())
				out << "\t" << proc;
			out << std::endl;

			for (auto proc : node->get_group_peer())
				out << "\t" << proc;
			out << std::endl;
		}
	}//end of for
}
