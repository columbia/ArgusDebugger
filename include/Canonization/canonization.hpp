#ifndef CANONIZATION_HPP
#define CANONIZATION_HPP

#include "group.hpp"
#include <math.h>

class NormEvent {
    EventBase *event;
public:
    std::string proc_name;
    EventType::event_type_t event_type;
    pid_t peer;

    NormEvent(EventBase *event);
    bool operator==(NormEvent &other);
    bool operator!=(NormEvent &other);
    EventBase *get_real_event(void);
};

class NormGroup {
    Group *group;
    std::map<EventType::event_type_t, bool> &key_events;
    std::list<NormEvent *> normalized_events;
    void normalize_events(void);
public:
    NormGroup(Group *g,std::map<EventType::event_type_t, bool> &key_events);
    ~NormGroup();

    uint64_t original_size(void) {return group->get_container().size();}
    uint64_t normalized_size() {return normalized_events.size();}
    uint64_t get_group_id(void) {return group->get_group_id();}
    std::list<NormEvent *> &get_normalized_events(void) {return normalized_events;}
    std::map<std::string, uint32_t> &get_group_tags(void) {return group->get_group_tags();}

    bool is_subset_of(std::list<NormEvent *>);
    bool is_empty() {return normalized_events.size() == 0;}
	NormEvent *last_event_of_type(int type);
	bool operator<=(NormGroup &other);
    bool operator==(NormGroup &other);
    bool operator!=(NormGroup &other);
	std::string signature();
	void print_events();
};

#include "graph.hpp"
class NormNode {
    Node *node;
    NormGroup *norm_group;
public:
    NormNode(Node *_node,std::map<EventType::event_type_t, bool> &key_events);
    ~NormNode();
    bool is_empty();
	bool operator<=(NormNode &other);
    bool operator==(NormNode &other);
    bool operator!=(NormNode &other);
	uint64_t get_gid(void) {return node->get_gid();}
    Node *get_node(void) {return node;}
    NormGroup *get_norm_group() {return norm_group;}
	std::string signature(){return norm_group->signature();}
	int edit(std::string, std::string, int, int);
	int distance_to(NormNode &other);
};

#endif
