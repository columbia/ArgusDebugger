#ifndef BEAM_SEARCH_HPP
#define BEAM_SEARCH_HPP

#include "search_helper.hpp"
#include <sstream>

class BeamSearcher: public Searcher{
public:
    typedef std::vector<Edge *> path_t;
    typedef std::pair<path_t, int> elem_t;
    typedef std::list<elem_t> paths_t;

private:
    Node *anomaly_node;
    int beam_widths;
    paths_t causal_paths;
	std::map<Node *, bool> ui_nodes;
	int lookback_steps;
    int a;
    int b;

    //locate anomaly
    static const int SPINNING_NONE = -1;
    static const int SPINNING_BUSY = 0;
    static const int SPINNING_YIELD = 1;
    static const int SPINNING_WAIT = 2;

    Node *get_anomaly_node();
	void clear_anomaly_node() {anomaly_node = nullptr;}

    bool is_wait_spinning();
    int spinning_type(Node *spinning_node);
    std::string decode_spinning_type(int);
    std::string decode_spinning_type();
    const int get_baseline_type(Node *spinning_node);

    /*seach algorithm helper functions*/
	int penalty_func(int edge_type);
    static bool cmp_paths_by_scores(const BeamSearcher::elem_t &elem1, const BeamSearcher::elem_t &elem2);
    void sort_paths();
	int calculate_candidates();
    void expand_paths();
    void select_paths();

public:
    BeamSearcher(Graph *_graph, key_events_t event_for_compare, int beam_widths);
    BeamSearcher(Graph *_graph, int beam_widths, int lookback_steps = 5);
    BeamSearcher(Graph *_graph, int a, int b, int beam_widths, int lookcak_steps = 5);
    ~BeamSearcher();

    //beam causal path search
	void set_lookback_steps(int val) {lookback_steps = val;}
	bool set_anomaly_node(uint64_t gid);
    uint64_t beam_search();
    uint64_t search_paths_to_node(Node *node);
	uint64_t search_from_baseline(Node *node);

    //comparison diagnosis
    std::map<tid_t, Node *> condense_path(BeamSearcher::path_t &path);
    std::vector<Node *> check_thread(Node *node);
    int path_compare(BeamSearcher::path_t &path);

    int path_comparison();
    uint64_t init_diagnose();
};
#endif

