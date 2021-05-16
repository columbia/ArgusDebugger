#include "breakpoint_trap.hpp"
BreakpointTrapEvent::BreakpointTrapEvent(double abstime, std::string _op, uint64_t _tid, uint64_t eip,  uint32_t _core_id, std::string _procname)
:EventBase(abstime, BREAKPOINT_TRAP_EVENT, _op, _tid, _core_id, _procname)
{
    targets.clear();
    addrs.clear();
    vals.clear();

    caller.first = eip;
    caller.second = "";
    is_read = false;
    trigger_symbol = "";
}

void BreakpointTrapEvent::update_target(int index, std::string key)
{
    if (index < vals.size())
        targets[key] = vals[index];
}

void BreakpointTrapEvent::set_trigger_var(std::string _var)
{
    if (trigger_symbol.size() == 0)
        trigger_symbol = _var;
}

 uint32_t BreakpointTrapEvent::get_trigger_val()
 {
        return trigger_symbol.size() > 0 
                ? targets[trigger_symbol] : -1;
 }

void BreakpointTrapEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\n\tcaller " << caller.second << std::endl;
    outfile << "\n\ttrigger " << trigger_symbol << std::endl;
    
    std::map<std::string, uint32_t>::iterator it;
    for (it = targets.begin(); it != targets.end(); it++)
        outfile << "\n\t" << it->first << " = " << it->second << std::endl;
}

void BreakpointTrapEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    if (caller.second.size())
        outfile << "\tcaller " << caller.second;
    
    std::vector<uint64_t>::iterator addr_it;
    for (addr_it = addrs.begin(); addr_it != addrs.end(); addr_it++) {
        outfile << "\t" << *addr_it;
    }
    std::vector<uint32_t>::iterator val_it;
    for (val_it = vals.begin(); val_it != vals.end(); val_it++) {
        outfile << "\t" << *val_it;
    }

    std::map<std::string, uint32_t>::iterator it;
    for (it = targets.begin(); it != targets.end(); it++) {
        outfile << "\t||" << it->first << " = " << it->second;
    }

    if (get_event_peer())
        outfile << "\trw_peer at: " << std::fixed << std::setprecision(1) << get_event_peer()->get_abstime();

    outfile << std::endl;
}
