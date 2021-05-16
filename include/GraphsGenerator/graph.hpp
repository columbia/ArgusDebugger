#ifndef GRAPH_HPP
#define GRAPH_HPP
#include "group.hpp"

#define ALLOW_WEAK_EDGE		1

#define MSGP_REL            0
#define MSGP_NEXT_REL		1
#define MKRUN_REL           2
#define DISP_EXE_REL        3
#define TIMERCALLOUT_REL    4
#define RLITEM_REL          6
#define CA_REL              7
#define HWBR_REL            8
#define WEAK_BOOST_REL 	    10
#define TIMERCANCEL_REL     11
#define WEAK_MKRUN_REL      12
#define WEAK_REL            13
#define IMPORT_REL          16

class Edge;
class Node;
class Graph;

class Edge{
    double weight;

public:
    Node *from;
    Node *to;
    EventBase *e_from;
    EventBase *e_to;
    uint32_t rel_type;

    Edge(EventBase *, EventBase*, uint32_t);
    ~Edge();

    bool operator==(const Edge &edge) const
    {
        return (e_from == edge.e_from 
            && e_to == edge.e_to 
            && rel_type == edge.rel_type);
    }
    bool is_weak_edge() {return rel_type >= WEAK_MKRUN_REL;}
	void set_edge_to_node(Node *t_to) {to = t_to;}
	void set_edge_from_node(Node *t_from) {from = t_from;}
    double get_weight() {return weight;}
    std::string decode_edge_type();
};

class Node{
public:
    typedef std::unordered_multimap<EventBase *, Edge *> edge_map_t;
    typedef std::pair<EventBase *, Edge *> edge_pair_t;

private:
    Graph *parent;
    Group *group;
    uint32_t in;
    uint32_t out;
	uint32_t weak_in;
	uint32_t weak_out;
    double weight;

    edge_map_t in_edges; //sink event -> edge
    edge_map_t out_edges; //source event -> edge
    edge_map_t out_weak_edges; //source event -> edge
    edge_map_t in_weak_edges; // sink event -> edge

    void inc_in(){in++;}
    void inc_out(){out++;}
    void inc_weak_in(){weak_in++;}
    void inc_weak_out(){weak_out++;}
    void dec_in(){in--;}
    void dec_out(){out--;}
    void dec_weak_in(){weak_in--;}
    void dec_weak_out(){weak_out--;}
    bool find_edge_in_map(edge_pair_t, bool);
    bool find_edge_in_weak_map(edge_pair_t, bool);

public:
    Node(Graph *, Group *);
    ~Node();

    bool add_in_edge(Edge *e);
    bool add_out_edge(Edge *e);
    bool add_out_weak_edge(Edge *e);
    bool add_in_weak_edge(Edge *e);

    edge_map_t &get_in_edges();
    edge_map_t &get_out_edges();
    edge_map_t &get_in_weak_edges(){return in_weak_edges;}
    edge_map_t &get_out_weak_edges(){return out_weak_edges;}

    Group *get_group(){return group;}
    group_id_t get_gid() {return group->get_group_id();}
    tid_t get_tid() {return group->get_tid();}
	pid_t get_pid() {return group->get_pid();}
	uint64_t get_size() {return group->get_size();}
    std::set<std::string> &get_group_peer(){return group->get_group_peer();}
    
    uint32_t get_in(){return in;}
    uint32_t get_out(){return out;}
    uint32_t get_weak_in(){return weak_in;}
    uint32_t get_weak_out(){return weak_out;}

   	int is_divide_by_msg() {return group->is_divide_by_msg();}
    int is_divide_by_wait() {return group->is_divide_by_wait();}
    int is_divide_by_disp() {return group->is_divide_by_disp();}

    void remove_edges();
    void remove_edge_weak(edge_pair_t, bool out);
    void remove_edge(edge_pair_t, bool out);

    int compare_syscall_ret(Node *peer);
    int compare_timespan(Node *peer);
    int compare_wait_cost(Node *peer);
    int compare_wait_ret(Node *peer);
    double get_begin_time();
    double get_end_time();
    double time_span() {return weight;}

    bool wait_over();
	bool execute_over() {return get_end_time() - get_begin_time() > 1500000;}
    bool is_node_from_thread(tid_t tid) {return get_tid() == tid;} 
    NSAppEventEvent *contains_nsapp_event();
    bool contains_view_update();
	std::string get_procname() {return index_to_event(0)->get_procname();}
    EventBase *index_to_event(int index);
    int event_to_index(EventBase *e) {return group->event_to_index(e);}
	void show_event_detail(int index, std::ostream &out);
    void pattern_check();
	void recent_backtrace();
	void busy_backtrace();
};

class Graph
{
protected:
    EventLists *event_lists_ptr;
    Groups *groups_ptr;
    bool construct_inside;
    bool create_graph;
    std::mutex graph_edge_mtx;
#if ALLOW_WEAK_EDGE
	/*categories of weak edges:
	 1. kernel weak edges <- kernel batching
	 2. wait <- thread pause (call stack identification)
		wait <- thread work finished (BSC_workq_kernreturn, and etc.)
	 3. msg peers <- related messages for different peers(call stacks)
		msg peers <- unknown batching (different peers)
	 4. make run <- interrupts, kernel share maintainace
		make run <- duplicated with msg, delayed callout, and etc.
		make run <- thread pool management (BSC_workq_kernreturn, and etc)
	*/
	int weak_edge_kernel;
	int weak_wait_edge_BSC_workq;
	int weak_edge_diff_stacks;
	int weak_wait_edge_boost;
    int weak_msg_edge_boost;
	int weak_mkrun_edge;
    int dup_mkrun;
#endif

    virtual void init_graph() {}
    virtual Edge *add_weak_edge(Node *) {return nullptr;}

public:
    typedef std::list<EventBase *> event_list_t;
    typedef std::unordered_multimap<EventBase *, Edge *> edge_map_t;
    typedef std::unordered_map<Group *, Node *> node_map_t;

    node_map_t nodes_map;
	edge_map_t edge_map; /* mapping source_event -> edge */
    std::vector<Node *> nodes;
    std::vector<Edge *> edges;

    Graph();
    Graph(Groups *groups);
    virtual ~Graph();
	
	std::vector<Node *> get_nodes() {return nodes;}
	std::vector<Edge *> get_edges() {return edges;}

    void remove_node(Node *node);
    void remove_edge(Edge *e);
    Node *check_and_add_node(Node *node);
    Edge *check_and_add_edge(Edge *edge);
    Edge *check_and_add_weak_edge(Edge *edge);
	Edge *get_edge(EventBase *from, EventBase *to);

	Node *normal_waken(Node *node);
	Node *recent_event_node(group_id_t gid);
    Node *id_to_node(group_id_t gid);
    Node *next_node(Node *node) {return id_to_node(node->get_gid() + 1);}
	Node *node_of(EventBase *event);
	Group *group_of(EventBase *event);
	EventBase *get_event(group_id_t gid, int index);

    std::vector<Node *> get_nodes_for_tid(tid_t tid);
	std::vector<Node *> nodes_between(Node *cur_node, double timestamp);
    Node *get_spinning_node();
    Node *get_last_node_in_main_thread();
    uint64_t get_main_tid() {return groups_ptr->get_main_thread();}
    event_list_t &get_list_of_UI_event() {return groups_ptr->get_list_of_op(NSAPPEVENT);}
	//event_list_t &get_list_of_applogs() {return groups_ptr->get_list_of_op(APP_LOG);}
    event_list_t &get_event_list() {return groups_ptr->get_list_of_op(0);}
    EventLists *get_event_lists() {return event_lists_ptr;}
    Groups* get_groups_ptr() {return groups_ptr;}

    std::string create_output_path(std::string input_path);
    void graph_statistics();
    void check_weak_edges(std::ofstream &out);
    void check_heuristics(std::ofstream &out);
	void direct_communicate_procs(std::string procname, std::ostream &out);
    void streamout_nodes(std::string path);
    void streamout_edges(std::string path);
    void streamout_nodes_and_edges(std::string path);
};

class EventGraph : public Graph
{
    void init_graph();
    void complete_vertices_for_edges();
    void add_edges(Node *);

    bool similarity_on_stack(Group *cur, Group* peer);
    Edge *add_weak_edge(Node *);
    void add_edge_for_msg(Node *, MsgEvent *);
    void add_edge_for_disp_enq(Node *, BlockEnqueueEvent *);
    void add_edge_for_disp_invoke(Node *, BlockInvokeEvent *);
    void add_edge_for_callcreate(Node *, TimerCreateEvent *);
    void add_edge_for_callout(Node *, TimerCalloutEvent *timer_callout);
    void add_edge_for_callcancel(Node *, TimerCancelEvent *timer_callout);
    void add_edge_for_caset(Node *, CASetEvent *);
    void add_edge_for_cadisplay(Node *, CADisplayEvent *ca_display_event);
    void add_edge_for_rlitem(Node *, RunLoopBoundaryEvent *);
    void add_edge_for_hwbr(Node *, BreakpointTrapEvent *event);
    void add_edge_for_mkrun(Node *, MakeRunEvent *);
    void add_edge_for_fakedwoken(Node *, FakedWokenEvent *);
    void add_edge_for_wait(Node *, WaitEvent *);
    
public:
    EventGraph();
    EventGraph(Groups *groups);
    ~EventGraph();
    void optimize_mkrun_edges();
};

class TransactionGraph : public Graph
{
private:
    Group *root;
    uint64_t id;
    Node *root_node;
    Node *end_node;
    std::map<uint64_t, Node *> main_nodes;

    std::unordered_map<Node *, EventBase *> search_node;
    std::unordered_map<Group *, Node *> search_group;

    void init_graph();
	tid_t get_main_tid();
    Edge *add_weak_edge(Node *);
	Edge *get_outgoing_connection_for_msg(Node *, MsgEvent *msg_event);
	Edge *get_outgoing_connection_for_disp_enq(Node *, BlockEnqueueEvent *enq_event);
	Edge *get_outgoing_connection_for_callcreate(Node *, TimerCreateEvent *timer_create);
	Edge *get_outgoing_connection_for_caset(Node *, CASetEvent *ca_set_event);
	Edge *get_outgoing_connection_for_rlboundary(Node *, RunLoopBoundaryEvent *rl_event);
	Edge *get_outgoing_connection_for_hwbr(Node *, BreakpointTrapEvent *event);
	Edge *get_outgoing_connection_for_mkrun(Node *, MakeRunEvent *mkrun_event);
	std::map<Group *, Edge *> get_outgoing_connections(Node *node, EventBase *event_after);
	Node *augment_node(Group *cur_group, EventBase *event_after);
    void BFS(Node *node, EventBase *event_after);
    Node *add_node(Group *cur_group);

public:
    TransactionGraph(uint64_t gid);
    TransactionGraph(Groups *groups, uint64_t gid);
    ~TransactionGraph();
    Node *get_root_node() {return root_node;}
    Node *get_end_node();
    uint64_t get_id() {return id;}
}; 
#endif

