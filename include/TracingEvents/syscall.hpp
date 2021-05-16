#ifndef SYSCALL_HPP
#define SYSCALL_HPP

#include "base.hpp"
#include "backtraceinfo.hpp"
#include "loader.hpp"

#define MSC_SYSCALL 0
#define BSC_SYSCALL 1
#define MAX_ARGC 12

/*define in XXXsyscalldef.cpp*/
extern const struct syscall_entry mach_syscall_table[];
extern const struct syscall_entry bsd_syscall_table[];
extern uint64_t msc_size;
extern uint64_t bsc_size;

class SyscallEvent : public EventBase {
    const struct syscall_entry *sc_entry;
    uint64_t args[MAX_ARGC];
    uint64_t syscall_class;
    int    nargs;
    uint64_t ret;
    BacktraceEvent *bt;

public :
    SyscallEvent(double abstime, std::string op, uint64_t _tid, uint64_t sc_class, uint32_t event_core, std::string proc="");
    bool audit_args_num(int size);
    bool set_args(uint64_t *array, int size);
    void set_entry(const struct syscall_entry *entry) { sc_entry = entry;}
    void set_ret_time(double time) {set_finish_time(time);}
    void set_ret(uint64_t _ret) {ret = _ret;}
    void set_bt(BacktraceEvent *_bt) {bt = _bt;}

    uint64_t get_arg(int idx);
    const struct syscall_entry *get_entry(void) {return sc_entry;}
    double get_ret_time(void) {return get_finish_time();}
    uint64_t get_ret(void) {return ret;}
    BacktraceEvent* get_bt(void) {return bt;}
    const char *class_name(void); 

    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};
#endif
