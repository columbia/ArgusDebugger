#ifndef BREAKPOINT_TRAP_CONNECTION_HPP
#define BREAKPOINT_TRAP_CONNECTION_HPP
#include "breakpoint_trap.hpp"

#define FLIP_VAR 0
#define QUEUE_VAR 1

typedef std::map<std::string, std::vector<BreakpointTrapEvent *> > queue_hw_write_t;

class BreakpointTrapConnection {
    std::list<EventBase *> &breakpoint_trap_list;
    std::map<std::string, uint32_t> var_type;
public:
    BreakpointTrapConnection(std::list<EventBase *> &_breakpoint_trap_list);
    void flip_var_connection(std::string var, uint32_t prev_val, uint32_t val,
        queue_hw_write_t &prev_writes, BreakpointTrapEvent *cur_event);
    void queue_var_connection(std::string var, uint32_t prev_val, uint32_t val,
        queue_hw_write_t &prev_writes, BreakpointTrapEvent *cur_event);
    void breakpoint_trap_connection(void);
};
typedef BreakpointTrapConnection breakpoint_trap_connection_t;
#endif
