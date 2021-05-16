#include "breakpoint_trap_connection.hpp"
#include "eventlistop.hpp"

#define DEBUG_BREAKPOINT_TRAP 0

BreakpointTrapConnection::BreakpointTrapConnection(std::list<EventBase *> &_breakpoint_trap_list)
:breakpoint_trap_list(_breakpoint_trap_list)
{
    var_type.clear();
    var_type["gOutMsgPending"] = FLIP_VAR;
    var_type["_ZL28sCGEventIsMainThreadSpinning"] = FLIP_VAR;
    var_type["_ZL32sCGEventIsDispatchedToMainThread"] = QUEUE_VAR;
    var_type["gCGSWillReconfigureSeen"] = FLIP_VAR;
    var_type["gCGSDidReconfigureSeen"] = FLIP_VAR;
	//add monitored variables here and indicates the value flip type
	//default is FLIP_VAR
}

void BreakpointTrapConnection::flip_var_connection(std::string var, uint32_t prev_val, uint32_t val,
        queue_hw_write_t &prev_writes, BreakpointTrapEvent *cur_event)
{
    if (prev_val == 0 && val == 1) {
		//store writes
        cur_event->set_read(false);
        if (prev_writes.find(var) == prev_writes.end()) {
            std::vector<BreakpointTrapEvent *> container;
            prev_writes[var] = container;
        }
        prev_writes[var].push_back(cur_event);
        cur_event->set_trigger_var(var);
    } else if (prev_val == 1 && val == 0) {
        cur_event->set_read(true);
        if (prev_writes.find(var) != prev_writes.end() && (prev_writes[var].size())) {
            cur_event->set_event_peer(prev_writes[var][0]);
            prev_writes[var][0]->set_event_peer(cur_event);
            prev_writes[var].erase(prev_writes[var].begin());
        }
        cur_event->set_trigger_var(var);
    }
}

void BreakpointTrapConnection::queue_var_connection(std::string var, uint32_t prev_val, uint32_t val,
        queue_hw_write_t &prev_writes, BreakpointTrapEvent *cur_event)
{
    if (val == 1) {
		//store writes
        cur_event->set_read(false);
        if (prev_writes.find(var) == prev_writes.end()) {
            std::vector<BreakpointTrapEvent *> container;
            prev_writes[var] = container;
        }
        prev_writes[var].push_back(cur_event);
    } else {
        cur_event->set_read(true);
        if (prev_writes.find(var) != prev_writes.end() && (prev_writes[var].size())) {
            cur_event->set_event_peer(prev_writes[var][0]);
            cur_event->set_trigger_var(var);
            prev_writes[var][0]->set_event_peer(cur_event);
            prev_writes[var][0]->set_trigger_var(var);
            prev_writes[var].erase(prev_writes[var].begin());
        }
    }
}

void BreakpointTrapConnection::breakpoint_trap_connection(void)
{
    std::map<std::string, uint32_t> prev_values;
    std::map<std::string, std::vector<BreakpointTrapEvent *> > prev_writes;
    std::map<std::string, std::vector<BreakpointTrapEvent *> >::iterator vec_it;
    std::list<EventBase *>::iterator it;
    BreakpointTrapEvent *cur_event;
    EventLists::sort_event_list(breakpoint_trap_list);
    prev_values.clear();
    prev_writes.clear();
    
    for (it = breakpoint_trap_list.begin(); it != breakpoint_trap_list.end(); it++) {
        cur_event = dynamic_cast<BreakpointTrapEvent *>(*it);
        std::map<std::string, uint32_t> cur_targets = cur_event->get_targets();
        std::map<std::string, uint32_t>::iterator target_it;
        for (target_it = cur_targets.begin(); target_it != cur_targets.end(); target_it++) {
            std::string var = target_it->first;
            uint32_t val = target_it->second;
            uint32_t prev_val = prev_values.find(var) != prev_values.end() ? prev_values[var] : 0;
            
            switch (var_type[var]) {
                case FLIP_VAR:
                    flip_var_connection(var, prev_val, val, prev_writes, cur_event);
                    break;
                case QUEUE_VAR:
                    queue_var_connection(var, prev_val, val, prev_writes, cur_event);
                    break;
                default:
                    break;
            }
            prev_values[var] = val;
        }
    }

    for (vec_it = prev_writes.begin(); vec_it != prev_writes.end(); vec_it++)
        (vec_it->second).clear();
    
    prev_writes.clear();
}
