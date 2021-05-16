#include "beam_search.hpp"
#include "loguru.hpp"
#include "limits.h"

#define DEBUG_BEAMSEARCH 0

BeamSearcher::BeamSearcher(Graph *_graph, BeamSearcher::key_events_t event_for_compare, int _max)
:Searcher(_graph, event_for_compare), anomaly_node(nullptr), beam_widths(_max)
{
    causal_paths.clear();
	ui_nodes.clear();
	lookback_steps = 5;
}

BeamSearcher::BeamSearcher(Graph *_graph, int _max, int lookback)
:Searcher(_graph), anomaly_node(nullptr), beam_widths(_max)
{
    causal_paths.clear();
	ui_nodes.clear();
	lookback_steps = lookback;
}

BeamSearcher::BeamSearcher(Graph *_graph, int _a, int _b, int _max, int lookback)
:Searcher(_graph), anomaly_node(nullptr), beam_widths(_max)
{
    causal_paths.clear();
	ui_nodes.clear();
	a = _a;
	b = _b;
	lookback_steps = lookback;
}

BeamSearcher::~BeamSearcher()
{
    causal_paths.clear();
	ui_nodes.clear();
}

/////////////////////////
//locate anomaly
Node *BeamSearcher::get_anomaly_node()
{
	if (anomaly_node)
		return anomaly_node;
    
    anomaly_node = graph->get_spinning_node();
	if (anomaly_node)
		return anomaly_node;

    anomaly_node = graph->get_last_node_in_main_thread();
	if (anomaly_node) {
		return anomaly_node;
	}

	anomaly_node = nullptr;
    return nullptr;
}

bool BeamSearcher::set_anomaly_node(uint64_t gid)
{
	anomaly_node = graph->id_to_node(gid);
	if (anomaly_node) 
		LOG_S(INFO) << "set anomaly node 0x" << std::hex << gid;
	else
		LOG_S(INFO) << "anomaly node is not set" << std::endl;
	return anomaly_node != nullptr;
}

int BeamSearcher::spinning_type(Node *spinning_node)
{
    if (spinning_node == nullptr)
        return SPINNING_NONE;

    Group* spinning_group = spinning_node->get_group();
    assert(spinning_group);
    
    if (spinning_group->wait_over() || spinning_group->wait_timeout()) {
        LOG_S(INFO) << "spinning_wait";
        return SPINNING_WAIT;
    }

    int yield_freq = 0;
    for (auto event:spinning_group->get_container()) {
        if (event->get_event_type() == SYSCALL_EVENT
            && event->get_op().find("thread_switch") != std::string::npos)
            yield_freq++;
    }

    if (yield_freq > 0 && spinning_group->get_container().size() / yield_freq < 3) {
        LOG_S(INFO) << "spinning_yield";
        return SPINNING_YIELD;
    }

	if (spinning_group->execute_over())
    	return SPINNING_BUSY;

	return SPINNING_NONE;
}

bool BeamSearcher::is_wait_spinning()
{
	if (anomaly_node == nullptr)
        return false;
    return spinning_type(anomaly_node) == SPINNING_WAIT;
}

const int BeamSearcher::get_baseline_type(Node *spinning_node)
{
    switch(spinning_type(spinning_node)) {
        case SPINNING_WAIT:
            return COMPARE_WAIT_TIME;
        case SPINNING_YIELD:
            return COMPARE_YIELDS;
        default:
            break;
    }
    return COMPARE_NODE_TIME;
}

std::string BeamSearcher::decode_spinning_type(int type)
{
    switch(type) {
        case SPINNING_NONE:
            return "not spinning";
        case SPINNING_WAIT:
            return "long waiting";
        case SPINNING_YIELD:
            return "long yield";
        case SPINNING_BUSY:
            return "cpu busy";
    }
    return "not spinning";
}


/////////////////////
//beam search
int BeamSearcher::calculate_candidates()
{
	int i = 0;
    for (auto path: causal_paths) {
        assert(path.first.size() > 0);
        auto last_node = path.first.back()->from;

		if (ui_nodes.find(last_node) != ui_nodes.end())
			continue;

        auto last_event = path.first.back()->e_from;
        auto prevs = get_incoming_edges_before(last_node, last_event->get_abstime());
        auto prev_edges = collapse_edges(prevs);
		i += prev_edges.size();
	}
	return i;
}

int edge_val(int edge_type)
{
	switch(edge_type) {
		case MSGP_REL:
		case MSGP_NEXT_REL:
		case CA_REL:
		case HWBR_REL:
		case RLITEM_REL:
		case DISP_EXE_REL:
		case TIMERCALLOUT_REL:
		case MKRUN_REL:
		case TIMERCANCEL_REL:
			return -1;
		case WEAK_BOOST_REL:
			return 0;
		case WEAK_MKRUN_REL:
		case WEAK_REL:
			return 1;

		default:
			LOG_S(INFO) << "unknown edge type " << std::dec << edge_type; 
			break;
	}
	return 0;
}

int BeamSearcher::penalty_func(int edge_type)
{
	int x = edge_val(edge_type);
	return a * x + b;
}

void BeamSearcher::expand_paths()
{
    paths_t update_causal_paths;
    update_causal_paths.clear();

    for (auto path: causal_paths) {
        auto last_node = path.first.back()->from;
        auto last_event = path.first.back()->e_from;
        auto prevs = get_incoming_edges_before(last_node,
            last_event->get_abstime());
        prevs = collapse_edges(prevs);

        if (prevs.size() == 0 || last_node->contains_nsapp_event()) {
            update_causal_paths.push_back(path);
            continue;
        }

        for (auto edge: prevs) {
            auto new_state = path;
            new_state.first.push_back(edge);
			new_state.second = new_state.second + penalty_func(edge->rel_type);
            update_causal_paths.push_back(new_state);
        }
    }
    causal_paths.clear();
    causal_paths = update_causal_paths;
}

bool BeamSearcher::cmp_paths_by_scores(const BeamSearcher::elem_t &elem1, const BeamSearcher::elem_t& elem2)
{
    double score1 = (double)elem1.second;
    double score2 = (double)elem2.second;
    return score1 < score2;
}

void BeamSearcher::sort_paths()
{
    causal_paths.sort(BeamSearcher::cmp_paths_by_scores);
}

void BeamSearcher::select_paths()
{
    sort_paths();
    while (causal_paths.size() > beam_widths)
        causal_paths.pop_back();
}

uint64_t BeamSearcher::beam_search()
{
	int steps = 0;
    do {
        expand_paths();
		steps++;
        if (steps % lookback_steps == 0 && causal_paths.size() > beam_widths) {
              select_paths();
		}
    } while(calculate_candidates() > 0);

	if (causal_paths.size() > beam_widths) {
		select_paths();
	}
    return causal_paths.size();
}


uint64_t BeamSearcher::search_paths_to_node(Node *node)
{
	if (causal_paths.size() > 0) {
		beam_search();
		return causal_paths.size();
	}
	
	assert(node->get_group());
	
    double deadline = node->get_group()->event_before(2);
	assert(deadline > 0.0);
	
    auto candidates = get_incoming_edges_before(node, deadline);
    if (candidates.size() == 0)
        return causal_paths.size();

    auto edges = collapse_edges(candidates);

    LOG_S(INFO) << "Search causal paths to 0x" << std::hex << node->get_gid();
    for (auto edge: edges) {
        path_t new_path{edge};
        causal_paths.push_back(std::make_pair(new_path, penalty_func(edge->rel_type)));
    }
    beam_search();
    return causal_paths.size();
}

uint64_t BeamSearcher::search_from_baseline(Node *baseline)
{
    Node *wakeup_node = graph->next_node(baseline);
    if (wakeup_node == nullptr) {
        return -1;
    }

	if (wakeup_node->index_to_event(0)->get_event_type() != FAKED_WOKEN_EVENT) {
		return -1;
	}

	FakedWokenEvent *wake_event = dynamic_cast<FakedWokenEvent *>(wakeup_node->index_to_event(0)); 
	assert(wake_event && wake_event->get_event_peer());
	Node *wake_node = graph->node_of(wake_event->get_event_peer());
	Edge *edge = graph->get_edge(wake_event->get_event_peer(), wake_event);
    path_t new_path{edge};
    causal_paths.push_back(std::make_pair(new_path, penalty_func(edge->rel_type)));

    return search_paths_to_node(wake_node);
}

std::map<tid_t, Node *> BeamSearcher::condense_path(std::vector<Edge *> &path)
{
	std::map<tid_t, Node *> extract_path;
    for (auto edge : path) {
        if (extract_path.find(edge->from->get_tid()) == extract_path.end())
            extract_path[edge->from->get_tid()] = edge->from;
    }
    return extract_path;
}

std::vector<Node *> BeamSearcher::check_thread(Node *cur_normal_node)
{
    std::vector<Node *> suspicious_nodes;
    suspicious_nodes.clear();

    double anomaly_timestamp = anomaly_node->get_begin_time();
    auto inspecting_nodes = graph->nodes_between(cur_normal_node, anomaly_timestamp);
    if (inspecting_nodes.size() == 0)
        return suspicious_nodes;
    //check nodes
    int max_count = 64;
    Node *culprit = nullptr;
    for (auto rit = inspecting_nodes.rbegin(); 
        rit != inspecting_nodes.rend() && max_count > 0; rit++) {
        max_count--;
        if ((*rit)->wait_over() || (*rit)->execute_over()) {
            culprit = *rit;
            suspicious_nodes.push_back(culprit);
            LOG_S(INFO) << "Push long waiting culprit 0x"\
                << std::hex << culprit->get_gid()\
                << " compared to "\
                << std::hex << cur_normal_node->get_gid();
            break;
        }
    }
    return suspicious_nodes;
}

int BeamSearcher::path_compare(BeamSearcher::path_t &path)
{
    std::vector<Node *> suspicious_nodes;
    suspicious_nodes.clear();

    LOG_S(INFO) << "Compare path: ";
    print_edges(path);

    double anomaly_timestamp = anomaly_node->get_begin_time();
    auto check_path = condense_path(path);
    for (auto element: check_path) {
        std::vector<Node *> similar_nodes = get_similar_nodes_after(element.second, anomaly_timestamp);
        Node *same_node = node_exists(element.second, similar_nodes);
        if (same_node) {
            LOG_S(INFO) << "Find an identical node in anormaly case: 0x"\
                << std::hex << same_node->get_gid()\
                << " VS 0x"\
                << std::hex << element.second->get_gid();
                continue;
        }

        if (similar_nodes.size() > 0) {
            LOG_S(INFO) << "Check similar nodes (" << std::dec << similar_nodes.size()\
                << ") with different execution effects,"\
                <<" compared to 0x" << std::hex << element.second->get_gid();
            suspicious_nodes.insert(suspicious_nodes.end(), similar_nodes.begin(), similar_nodes.end());
        } else {
            auto nodes = check_thread(element.second);
            LOG_S(INFO) << "Check thread missing similarity to node 0x" << std::hex << element.second->get_gid()\
                << ", get " << std::dec << nodes.size() << " diffferent nodes."; 
            suspicious_nodes.insert(suspicious_nodes.end(), nodes.begin(), nodes.end());
        }
    }
    //comparison of causal path elements
	if (suspicious_nodes.size() > 0) {
    	LOG_S(INFO) << "Suspicious node for path:";
    	print_nodes(suspicious_nodes);
		for (auto node: suspicious_nodes) {
			LOG_S(INFO) << std::hex << node->get_gid();
			print_culprit(node, "RecentCallStack");
		}
	}
    return suspicious_nodes.size();
}

int BeamSearcher::path_comparison()
{
    int ret = 0;
    for (auto path_elem: causal_paths) {
       ret += path_compare(path_elem.first);
    }
    if (ret > 0)
        return 0;
    return -1;
}

uint64_t BeamSearcher::init_diagnose()
{
    if (get_anomaly_node() == nullptr) {
        LOG_S(INFO) << "No anomaly/spinning node found.";
        return 0;
    }
    assert(anomaly_node != nullptr);
	print_culprit(anomaly_node, "RecentCallstack");
	if (spinning_type(anomaly_node) != SPINNING_WAIT)
		print_culprit(anomaly_node, "BusyCallstack");

	/*First get the transaction lead to spinning*/
	int i = 0;
    search_paths_to_node(anomaly_node);
	for (auto path_elem: causal_paths) {
		LOG_S(INFO) << "Path " << std::dec << i++ << "(penalty = " << path_elem.second << ")";
		print_edges(path_elem.first);
		if (path_elem.first.back()->from->contains_nsapp_event())
			print_culprit(path_elem.first.back()->from, "UIEvent");
	}
	
	/*search baseline and do comparison*/
    auto baseline_nodes = search_baseline_nodes(anomaly_node, 
							get_baseline_type(anomaly_node));

    if (baseline_nodes.size() == 0) {
        LOG_S(INFO) << "No baseline nodes.";
		return 0;

	}

	for (auto baseline_node : baseline_nodes) {
		LOG_S(INFO) << "Searching path to baseline node 0x" << std::hex << baseline_node->get_gid();
		causal_paths.clear();
		ui_nodes.clear();
		int ret = search_from_baseline(baseline_node);
		if (ret == -1)
			return ret;
		int i = 0;
		for (auto path_elem: causal_paths) {
			LOG_S(INFO) << "Path " << std::dec << i++ << "(penalty = " << path_elem.second << ")";
			print_edges(path_elem.first);
			if (path_elem.first.back()->from->contains_nsapp_event())
				print_culprit(path_elem.first.back()->from, "UIEvent");
		}
		LOG_S(INFO) << "Path Comparision";
		path_comparison();
		return ret;
	}
	return 0;
}
