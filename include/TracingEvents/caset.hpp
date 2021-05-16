#ifndef CA_SET_HPP
#define CA_SET_HPP
#include "base.hpp"
#include "cadisplay.hpp"

class CASetEvent : public EventBase {
    uint64_t object_addr;
    CADisplayEvent *display;
public:
    CASetEvent(double timestamp, std::string op, uint64_t tid, uint64_t object, uint32_t coreid, std::string procname = "");
    uint64_t get_object_addr(void) {return object_addr;}
    void set_display(CADisplayEvent *disp) {display = disp;}
    CADisplayEvent *get_display_object(void) {return display;}
    CADisplayEvent *get_display_event(void) {return display;}
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};
#endif
