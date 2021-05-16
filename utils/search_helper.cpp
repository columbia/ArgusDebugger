#include "search_helper.hpp"
#include "loguru.hpp"

Searcher::Searcher(Graph *_graph, Searcher::key_events_t event_for_compare)
:graph(_graph) 
{
	key_events = event_for_compare.size() == 0\
			? load_default_key_events()\
			: event_for_compare;

    normnodes_map.clear();
    searched_node.clear();
}

Searcher::Searcher(Graph *_graph)
:graph(_graph)
{
    key_events = load_default_key_events();
    normnodes_map.clear();
    searched_node.clear();
}

Searcher::~Searcher()
{
	for (auto element: normnodes_map){
        auto &norm_group_vector = element.second;
		for (auto normnode:norm_group_vector)
            delete normnode;
    }
    normnodes_map.clear();
    key_events.clear();
}

Searcher::key_events_t Searcher::load_default_key_events()
{
    Searcher::key_events_t key_events;
    key_events.insert(std::make_pair(MSG_EVENT, true));
    key_events.insert(std::make_pair(MR_EVENT, false));
    key_events.insert(std::make_pair(FAKED_WOKEN_EVENT, true));
    key_events.insert(std::make_pair(INTR_EVENT, false));
    key_events.insert(std::make_pair(TSM_EVENT, false));
    key_events.insert(std::make_pair(WAIT_EVENT, true));
    key_events.insert(std::make_pair(DISP_ENQ_EVENT, true));
    key_events.insert(std::make_pair(DISP_INV_EVENT, true));
    key_events.insert(std::make_pair(TMCALL_CREATE_EVENT, false));
    key_events.insert(std::make_pair(TMCALL_CANCEL_EVENT, false));
    key_events.insert(std::make_pair(TMCALL_CALLOUT_EVENT, false));
    key_events.insert(std::make_pair(BACKTRACE_EVENT, true));
    key_events.insert(std::make_pair(SYSCALL_EVENT, true));
    key_events.insert(std::make_pair(BREAKPOINT_TRAP_EVENT, true));
    key_events.insert(std::make_pair(NSAPPEVENT_EVENT, true));
    key_events.insert(std::make_pair(DISP_MIG_EVENT, true));
    key_events.insert(std::make_pair(RL_BOUNDARY_EVENT, true));
    return key_events;
}

NormNode *Searcher::normnode(Node *node)
{
    assert(node);
    tid_t thread = node->get_tid();
    if (normnodes_map.find(thread) == normnodes_map.end()) 
        normnodes_for_thread(thread);
    
    auto nodes = graph->get_nodes_for_tid(thread);
    auto it = find(nodes.begin(), nodes.end(), node);
    int index = distance(nodes.begin(), it);
    assert(index >= 0 && index < nodes.size());
    assert(normnodes_map[thread][index]->get_node() == node);
    return normnodes_map[thread][index];
}

std::vector<NormNode *> Searcher::normnodes_for_thread(tid_t tid)
{
    if (normnodes_map.find(tid) != normnodes_map.end())
        return normnodes_map[tid];

    std::vector<NormNode *> result;
    auto nodes = graph->get_nodes_for_tid(tid);
    for (auto node: nodes) {
        assert(node != nullptr);
        NormNode *norm_node = new NormNode(node, key_events);
        result.push_back(norm_node);
    }
    normnodes_map[tid] = result;
    return result;
}


std::vector<Node *> Searcher::get_similar_nodes_before(Node *node, int number)
{
    std::vector<Node *> ret;
    ret.clear();

    if (number == 0)
        return get_similar_nodes_before(node);

    auto nodes = graph->get_nodes_for_tid(node->get_tid());
    auto normnodes = normnodes_for_thread(node->get_tid());

    auto it = find(nodes.begin(), nodes.end(), node);
    int index = distance(nodes.begin(), it);
    if (index <= 0) {
        LOG_S(INFO) << "No proceedings for search" << std::endl;
        return ret;
    }
    assert(index > 0);

    number = index / 2 >  number ? number: index / 2;
    for (int i = 0; i <= index - 2 * number; i++) {
        bool matched = true, check = false;
        for (int j = i; j < i + number; j++) {
            if (normnodes[j]->is_empty() == false)
                check = true;
			NormNode *cur_node = normnodes[j];
			NormNode *target_node = normnodes[index - number + (j - i)];
        	if (!(*target_node == *cur_node)) {
                matched = false;
                break;            
            }
        }

        if (check == false) {
            return ret;
        }

        if (matched == true)
            ret.push_back(normnodes[i + number]->get_node());
    }
    return ret;
}

std::vector<Node *> Searcher::get_similar_nodes_before(Node *node)
{
    std::vector<Node *> ret;
    ret.clear();

    NormNode *target_node = normnode(node);
    assert(target_node != nullptr);

    if(target_node->is_empty()) {
        LOG_S(INFO) << "Node 0x" << std::hex << node->get_gid()\
            << "is empty with given key events" << std::endl;
        return get_similar_nodes_before(node, 3);
    }

    auto normnodes = normnodes_for_thread(node->get_tid());
    auto end = find(normnodes.begin(), normnodes.end(), target_node);
    assert(end != normnodes.end() && *end == target_node);

    for (auto it = normnodes.begin(); it != end; it++) {
        NormNode *cur_node = *it;

        if (cur_node->is_empty()) {
            continue;
		}
        if (cur_node->get_node()->get_end_time() > node->get_begin_time()) {
            break; // only get similar group before current group
		}
        if (*target_node == *cur_node) {
            ret.push_back(cur_node->get_node());
		}
    }
    return ret;
}

std::vector<Node *> Searcher::get_similar_nodes_after(Node *node, double deadline)
{
    std::vector<Node *> ret;
    ret.clear();

    auto target_node = normnode(node);
    auto normnodes = normnodes_for_thread(node->get_tid());

    if(target_node->is_empty()) {
        LOG_S(INFO) << "Node 0x" << std::hex << node->get_gid()\
            << "is empty with given key events" << std::endl;
        return ret;
    }

    auto it = std::find(normnodes.begin(), normnodes.end(), target_node);
    assert(it != normnodes.end() && *it == target_node);
    for (it++; it != normnodes.end(); it++) {
        NormNode *cur_node = *it;
        if (deadline > 0.0 && cur_node->get_node()->get_begin_time() > deadline)
            break; 
        if (cur_node->is_empty())
            continue;
        if (*target_node == *cur_node)
            ret.push_back(cur_node->get_node());
    }
    return ret;
}

std::vector<Node *> Searcher::get_baseline_on_syscall(Node *node, std::vector<Node *> candidates)
{
    std::vector<Node *> return_nodes;
    return_nodes.clear();
    for (auto cur_node : candidates) {
        int ret = node->compare_syscall_ret(cur_node);
        if (ret != 0) 
            return_nodes.push_back(cur_node);
    }
    return return_nodes;
}

std::vector<Node *> Searcher::get_baseline_on_timespan(Node *node, std::vector<Node *> candidates)
{
    std::vector<Node *> return_nodes;
    return_nodes.clear();

    for (auto cur_node : candidates) {
        int ret = node->compare_timespan(cur_node);
        if (ret != 0) 
            return_nodes.push_back(cur_node);
    }
    return return_nodes;
}

std::vector<Node *> Searcher::get_baseline_on_waitcost(Node *node, std::vector<Node *> candidates)
{
    std::vector<Node *> return_nodes;
    return_nodes.clear();

    for (auto cur_node : candidates) {
        int ret = node->compare_wait_cost(cur_node);
        if (ret != 0) 
            return_nodes.push_back(cur_node);
    }
    return return_nodes;
}

std::vector<Node *> Searcher::get_baseline_on_waitreturn(Node *node, std::vector<Node *> candidates)
{
    std::vector<Node *> return_nodes;
    return_nodes.clear();

    for (auto cur_node : candidates) {
		int ret = node->compare_wait_ret(cur_node);
        if (ret != 0) 
            return_nodes.push_back(cur_node);
    }
	if (return_nodes.size() == 0) {
		LOG_S(INFO) << "No baseline node found.";
	}
    return return_nodes;
}

std::vector<Node *> Searcher::get_baseline_on_waken(Node *anomaly, std::vector<Node *> candidates)
{
	std::vector<Node *> ret;
	std::set<Node *> tmp;
	ret.clear();
	tmp.clear();
	Node *anomaly_waken = graph->normal_waken(anomaly);
	LOG_S(INFO) << "Search basenode with woken node";
	for (auto node: candidates) {
		Node *waken = graph->normal_waken(node);
		if (waken && (!anomaly_waken || waken != anomaly_waken))
			tmp.insert(waken);
	}
	for (auto node : tmp)
		ret.push_back(node);
		
	if (anomaly_waken)
		ret.push_back(anomaly_waken);
	LOG_S(INFO) << "#basenode = " << ret.size();
	return ret;
}

std::vector<Node *> Searcher::search_baseline_nodes(Node *cur_anomaly, const int type)
{
    std::vector<Node *> similars;
    similars.clear();
    
	if (cur_anomaly == nullptr)
		return similars;

    LOG_S(INFO) << "Search baseline for 0x"\
        << std::hex << cur_anomaly->get_gid();

    switch (type) {
        case COMPARE_WAIT_TIME: {
            LOG_S(INFO) << "Get similar nodes on wait";
            similars = get_similar_nodes_before(cur_anomaly);   
            if (similars.size() > 0) {
				std::vector<Node *> baselines;
				baselines = get_baseline_on_waitreturn(cur_anomaly, similars);
				if (baselines.size() == 0) {
					baselines = get_baseline_on_waken(cur_anomaly, similars);
				}
				return baselines;
            } else {
				similars.push_back(cur_anomaly);
				std::vector<Node *> baselines;
				baselines = get_baseline_on_waken(cur_anomaly, similars);
				if (baselines.size() == 0)
                	LOG_S(INFO) << "No prev similars" << std::endl;
				return baselines;
            }
            break;
        }       
        case COMPARE_YIELDS: {
            LOG_S(INFO) << "Get similar nodes on yeild";
            similars = get_similar_nodes_before(cur_anomaly, 3);
            if (similars.size() > 0) {
                return get_baseline_on_timespan(cur_anomaly, similars);
            } else {
                LOG_S(INFO) << "No prev similar nodes" << std::endl;
            }
            break;
        }
        default:
            LOG_S(INFO) << "No similar node found." << std::endl;
            break;
    }
    return similars;
}


bool Searcher::cmp_edge_by_intime(Edge *e1, Edge *e2)
{
    return e1->e_to->get_abstime() > e2->e_to->get_abstime();
}

bool Searcher::cmp_edge_by_weight(Edge *e1, Edge *e2)
{
    assert(e1);
    assert(e2);
    return e1->rel_type <= e2->rel_type;
}

std::vector<Edge *> Searcher::collapse_edges(std::vector<Edge *> incoming_edges)
{
    std::map<uint64_t, Edge *> dict;
    std::vector<Edge *> ret;
    ret.clear();

    if (incoming_edges.size() == 0)
        return ret;
    for (auto edge : incoming_edges)
        dict[edge->from->get_gid()] = edge;
    for (auto elem : dict)
        ret.push_back(elem.second);
    return ret;
}

std::vector<Edge *> Searcher::get_incoming_edges_before(Node *cur_node, double deadline)
{
	std::vector<Edge *> ret;
    ret.clear();
	for (auto element: cur_node->get_in_edges()) {
        if (element.second->from != element.second->to
			&& element.first->get_abstime() < deadline)
            ret.push_back(element.second);
    }

	for (auto element: cur_node->get_in_weak_edges()) {
        if (element.second->from != element.second->to)
            ret.push_back(element.second);
	}

	sort(ret.begin(), ret.end(), Searcher::cmp_edge_by_intime);
    return ret;
}

std::vector<Edge *> Searcher::get_strong_edges_before(Node *cur_node, double deadline)
{
	std::vector<Edge *> ret;
    ret.clear();
	for (auto element: cur_node->get_in_edges()) {
        if (element.second->from != element.second->to
			&& element.first->get_abstime() < deadline)
            ret.push_back(element.second);
    }

	sort(ret.begin(), ret.end(), Searcher::cmp_edge_by_intime);
    return ret;
}

std::vector<Edge *> Searcher::get_incoming_edges(Node *cur_node)
{
	std::vector<Edge *> ret;
    ret.clear();
	for (auto element: cur_node->get_in_edges()) {
        if (element.second->from != element.second->to)
            ret.push_back(element.second);
    }
	for (auto element: cur_node->get_in_weak_edges()) {
        if (element.second->from != element.second->to)
            ret.push_back(element.second);
	}

	sort(ret.begin(), ret.end(), Searcher::cmp_edge_by_intime);
    return ret;
}

std::vector<Edge *> Searcher::get_strong_edges(Node *cur_node)
{
	std::vector<Edge *> ret;
    ret.clear();
	for (auto element: cur_node->get_in_edges()) {
        if (element.second->from != element.second->to)
            ret.push_back(element.second);
    }
	sort(ret.begin(), ret.end(), Searcher::cmp_edge_by_intime);
    return ret;
}

void Searcher::print_nodes(std::vector<Node *> t_nodes)
{
    int i = 0;
    for (auto node : t_nodes) { 
        LOG_S(INFO)<< "\tNode [" << i << "] 0x" << std::hex << node->get_gid();
        i++;
    }
    LOG_S(INFO) << std::endl;
}

void Searcher::print_edges(std::vector<Edge *> edges)
{
    for (auto edge: edges) {
        LOG_S(INFO) << "\t0x" << std::hex << edge->from->get_gid()\
            << "[" << std::dec << edge->from->get_group()->event_to_index(edge->e_from)\
            << "] " << edge->from->get_procname()\
			<< "-- " << std::dec << edge->decode_edge_type()\
            << " -> 0x" << std::hex << edge->to->get_gid()\
            << "[" << std::dec << edge->to->get_group()->event_to_index(edge->e_to)\
            << "] " << edge->to->get_procname();
    }
    LOG_S(INFO) << std::endl;
}

void Searcher::print_culprit(Node *node, std::string desc)
{
	std::unordered_map<std::string, int> desc_map;
	desc_map["RecentCallstack"] = 0;
	desc_map["BusyCallstack"] = 1;
	desc_map["UIEvent"] = 2;

	switch(desc_map[desc]) {
		case 0: {
			node->recent_backtrace();
			break;
		}
		case 1: {
			node->busy_backtrace();
			break;
		}
		case 2: {
			NSAppEventEvent *nsappevent = node->contains_nsapp_event();
			if (nsappevent != nullptr) {
				nsappevent->print_event();
			}
			break;
		}
		default:
			LOG_S(INFO) << "Invalid Desc " << desc; 
	}
}

Node *Searcher::node_exists(Node *cur_normal_node, std::vector<Node *> &candidates)
{
    if (candidates.size() == 0)
        return nullptr;
    if (cur_normal_node->wait_over())
        return candidates.back();
    for (auto node: candidates) {
        if (!node->wait_over() && !node->execute_over())
            return node;
    }
    return nullptr;
}
