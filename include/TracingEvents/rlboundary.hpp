#ifndef RLBOUNDARY_HPP
#define RLBOUNDARY_HPP

#include "base.hpp"

#define CategoryBegin    0
#define CategoryEnd        1
#define ItemBegin        2
#define ItemEnd            3
#define SetSignalForSource0        4
#define UnsetSignalForSource0    5
#define BlockPerform    7
#define EventsBegin      8
#define EventsEnd        9

class RunLoopBoundaryEvent:public EventBase {
    uint32_t state;
    EventBase *owner;
    EventBase *consumer;
    uint64_t rls;
    uint64_t item;
    uint64_t block;
    std::string func_symbol;
public:
    RunLoopBoundaryEvent(double timestamp, std::string op, uint64_t tid, uint32_t state, uint32_t event_core, std::string procname="");
    void set_func_symbol(std::string _symbol) {func_symbol = _symbol;}
    
    void set_item(uint64_t _item) {item = _item;}
    void set_rls(uint64_t _rls) {rls = _rls;}
    void set_block(uint64_t _block) {block = _block;}
    uint32_t get_state(void) {return state;}
    uint64_t get_func_ptr(void) {return block;}
    uint64_t get_rls(void) {return rls;}
    uint64_t get_item(void) {return item;}
    uint64_t get_block(void) {return block;}
    
    void set_owner(EventBase *_owner) {owner = _owner; set_event_peer(owner);}
    void set_consumer(EventBase *_c) {consumer = _c; set_event_peer(consumer);}
    EventBase *get_owner(void) {return owner;}
    EventBase *get_consumer(void) {return consumer;}
    const char * decode_state(int state);
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};
#endif
