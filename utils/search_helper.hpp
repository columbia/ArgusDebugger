#ifndef _SEARCH_HPP
#define _SEARCH_HPP

#include "graph.hpp"
#include "canonization.hpp"
#include <sstream>

class Searcher {
public:
    typedef std::map<EventType::event_type_t, bool> key_events_t;
    typedef std::unordered_map<tid_t, std::vector<NormNode *> > normnodes_map_t;
    typedef std::unordered_map<Node *, bool> visit_map_t;

protected:
    Graph *graph;
    key_events_t key_events;
    visit_map_t searched_node;
    /*baseline comparision type*/
    static const int COMPARE_SYSCALL_RET = 0;
    static const int COMPARE_NODE_TIME = 1;
    static const int COMPARE_WAIT_TIME = 2;
    static const int COMPARE_YIELDS = 3;

    normnodes_map_t normnodes_map; 

    key_events_t load_default_key_events();
    NormNode *normnode(Node *node);
    std::vector<NormNode *> normnodes_for_thread(tid_t thread);
    std::vector<Node *> get_similar_nodes_before(Node *node, int number);
    std::vector<Node *> get_similar_nodes_before(Node *node);
    std::vector<Node *> get_similar_nodes_after(Node *node, double deadline);
    std::vector<Node *> get_baseline_on_syscall(Node *node, std::vector<Node *> candidates); 
    std::vector<Node *> get_baseline_on_timespan(Node *node, std::vector<Node *> candidates);
    std::vector<Node *> get_baseline_on_waitcost(Node *node, std::vector<Node *> candidates); 
    std::vector<Node *> get_baseline_on_waitreturn(Node *node, std::vector<Node *> candidates); 
    std::vector<Node *> get_baseline_on_waken(Node *anomaly, std::vector<Node *> candidates);
    std::vector<Node *> search_baseline_nodes(Node *node, const int type);
    Node *node_exists(Node *cur_normal_node, std::vector<Node *>&);

    /*seach algorithm helper functions*/
    static bool cmp_edge_by_intime(Edge *e1, Edge *e2);
    static bool cmp_edge_by_weight(Edge *e1, Edge *e2);

    //do not need to includes all edges having the same ends, only the latest ones.
    std::vector<Edge *> collapse_edges(std::vector<Edge *> incoming_edges);
	std::vector<Edge *> get_incoming_edges_before(Node *cur_node, double deadline);
	std::vector<Edge *> get_strong_edges_before(Node *cur_node, double deadline);
	std::vector<Edge *> get_incoming_edges(Node *cur_node);
	std::vector<Edge *> get_strong_edges(Node *cur_node);

    void print_nodes(std::vector<Node *> t_nodes);
    void print_edges(std::vector<Edge *> edges);
	void print_culprit(Node *node, std::string desc);

public:
    Searcher(Graph *_graph, key_events_t event_for_compare);
    Searcher(Graph *_graph);
    ~Searcher();
};
#endif

