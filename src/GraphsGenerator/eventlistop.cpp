#include "eventlistop.hpp"
#include "mach_msg.hpp"
#include <time.h>

EventLists::EventLists(LoadData::meta_data_t &meta_data)
{
	owner = true;
    generate_event_lists(meta_data);
	meta_data.last_event_timestamp = event_lists[0].size() > 0 ?
		event_lists[0].back()->get_abstime() : 0.0;
}

EventLists::EventLists(std::list<EventBase *> &eventlist)
{
	owner = false;
    event_lists[0] = eventlist;    
}

EventLists::~EventLists()
{
	if (owner == true)
        clear_event_list();
}

std::map<uint64_t, std::list<EventBase *>> &EventLists::get_event_lists()
{
    return event_lists;
}

static bool lldb_initialized = false;

int EventLists::generate_event_lists(LoadData::meta_data_t meta_data)
{
    LOG_S(INFO) << "Loading sysinfo...";
    LoadData::preload();
#if defined(__APPLE__)
	if (!lldb_initialized) {
    	lldb::SBDebugger::Initialize();
		lldb_initialized = true;
	}
#endif
	LOG_S(INFO) << "Finish loading sysinfo. Begin divide_and_parse tracing log...";
    event_lists = Parse::divide_and_parse();
	LOG_S(INFO) << "Finish divide_and_parse. Begin sorting event list...";
#if defined(__APPLE__)
    lldb::SBDebugger::Terminate();
#endif
	event_lists[0].sort(Parse::EventComparator::compare_time);
	LOG_S(INFO) << "Finished event list generation.";
    return 0;
}

void EventLists::sort_event_list(std::list<EventBase *> &evlist)
{
    evlist.sort(Parse::EventComparator::compare_time);
}

void EventLists::dump_all_event_to_file(std::string filepath)
{
    std::ofstream dump(filepath);
    if (dump.fail()) {
        std::cerr << "unable to open file " << filepath << std::endl;
		return;
    }

    std::list<EventBase *> &evlist = event_lists[0];
    std::list<EventBase *>::iterator it;
    for(it = evlist.begin(); it != evlist.end(); it++) {
        EventBase *cur_ev = *it;
        cur_ev->decode_event(1, dump);
    }
    dump.close();
}

void EventLists::streamout_all_event(std::string filepath)
{
    std::ofstream dump(filepath);
    if (dump.fail()) {
        std::cerr << "unable to open file " << filepath << std::endl;
		return;
    }
    std::list<EventBase *> &evlist = event_lists[0];
    std::list<EventBase *>::iterator it;
    for(it = evlist.begin(); it != evlist.end(); it++) {
        EventBase *cur_ev = *it;
        cur_ev->streamout_event(dump);
    }
    dump.close();
}

void EventLists::streamout_backtrace(std::string filepath)
{
    std::ofstream dump(filepath);
    if (dump.fail()) {
        std::cerr << "unable to open file " << filepath << std::endl;
		return;
    }

    std::list<EventBase *> &evlist = event_lists[BACKTRACE];
    std::list<EventBase *>::iterator it;
    for(it = evlist.begin(); it != evlist.end(); it++) {
        EventBase *cur_ev = *it;
#if defined(__APPLE__)
        if (dynamic_cast<BacktraceEvent *>(cur_ev))
            cur_ev->streamout_event(dump);
#endif
    }
    dump.close();
}

void EventLists::clear_event_list()
{
    std::list<EventBase *> &evlist = event_lists[0];
    std::list<EventBase *>::iterator it;
    for(it = evlist.begin(); it != evlist.end(); it++) {
        EventBase *cur_ev = *it;
        delete(cur_ev);
    }
    evlist.clear();
    event_lists.clear();
}

void EventLists::binary_search(std::list<EventBase *> list, std::string timestamp, std::list<EventBase *> &result)
{
}

void EventLists::filter_list_with_tid(std::list<EventBase *> &list, uint64_t tid)
{
}

void EventLists::filter_list_with_procname(std::list<EventBase *> &list, std::string procname)
{
}

void EventLists::show_events(std::string timestamp, const std::string type, std::string procname, uint64_t tid)
{
    
    std::list<EventBase *> result;
    if (timestamp != "0"){
        binary_search(event_lists[0], timestamp, result);   
        goto out;
    }

    if (type != "null" 
        && (LoadData::op_code_map.find(type) != LoadData::op_code_map.end()))
        result = event_lists[LoadData::op_code_map.at(type)];
    else
        result = event_lists[0];
    
    if (tid != 0)
        filter_list_with_tid(result, tid);

    if (procname != "null")
        filter_list_with_procname(result, procname);   
out:
 	return;      
}
