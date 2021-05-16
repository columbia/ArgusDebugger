#ifndef BREAKPOINT_TRAP_HPP
#define BREAKPOINT_TRAP_HPP
#include "base.hpp"

class BreakpointTrapEvent : public EventBase {
    std::vector<uint64_t> addrs;
    std::vector<uint32_t> vals;
    std::map<std::string, uint32_t> targets;
	std::pair<uint64_t, std::string> caller;
    std::string trigger_symbol;
    bool is_read;

public:
    BreakpointTrapEvent(double timestamp, std::string op, uint64_t tid,
        uint64_t eip, uint32_t coreid, std::string procname = "");
    void add_addr(uint64_t addr) {addrs.push_back(addr);}
    void add_value(uint32_t val) {vals.push_back(val);}
    std::vector<uint64_t> &get_addrs(void) {return addrs;}
    std::map<std::string, uint32_t> &get_targets(void) {return targets;}
    void set_trigger_var(std::string var);
    std::string get_trigger_var(void) {return trigger_symbol;}
    uint32_t get_trigger_val(void);
    void update_target(int index, std::string key);

	void set_caller(std::string _caller) {caller.second = _caller;}
    uint64_t get_eip(void) {return caller.first;}
    std::string get_caller(void) {return caller.second;}

    // referred by breakpoint connection
    void set_read(bool read) {is_read = read;}
    bool check_read(void) {return is_read;}

    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};
#endif
