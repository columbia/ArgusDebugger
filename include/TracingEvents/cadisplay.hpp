#ifndef CA_DISP_HPP
#define CA_DISP_HPP
#include "base.hpp"

class CADisplayEvent : public EventBase {
    uint64_t object_addr;
    uint64_t is_serial;
    std::vector<CASetEvent *> object_set_events;
public:
    CADisplayEvent(double timestamp, std::string op, uint64_t tid,
            uint64_t object, uint32_t coreid, std::string procname = "");
    void push_set(CASetEvent * set) {object_set_events.push_back(set);}
    void set_serial(uint64_t serial) {is_serial = serial;}
    uint64_t get_serial(void) {return is_serial;}
    int ca_set_event_size(void) {return object_set_events.size();}
    std::vector<CASetEvent *>& get_ca_set_events(void) {return object_set_events;}
    uint64_t get_object_addr(void) {return object_addr;}
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};
#endif
