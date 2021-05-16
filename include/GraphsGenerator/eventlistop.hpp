#ifndef EVENTLIST_HPP
#define EVENTLIST_HPP

#include "parser.hpp"
class EventLists
{
	typedef std::list<EventBase *> event_list_t;
    typedef std::map<uint64_t, std::list<EventBase *> > event_lists_t;
    event_lists_t event_lists;
    int generate_event_lists(LoadData::meta_data_t);
    void clear_event_list();
	bool owner;
public:
    EventLists(LoadData::meta_data_t &);
    EventLists(std::list<EventBase *> &eventlist);
    ~EventLists();

	event_lists_t &get_event_lists();
    static void sort_event_list(event_list_t &);
    void binary_search(event_list_t list, std::string timestamp, event_list_t &result);
    void filter_list_with_tid(event_list_t &list, uint64_t tid);
    void filter_list_with_procname(event_list_t &list, std::string procname);
    void show_events(std::string timestamp, const std::string type, std::string procname, tid_t tid);
    void dump_all_event_to_file(std::string filepath);
    void streamout_all_event(std::string filepath);
    void streamout_backtrace(std::string filepath);
};
#endif
