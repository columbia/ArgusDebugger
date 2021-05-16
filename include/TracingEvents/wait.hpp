#ifndef WAIT_HPP
#define WAIT_HPP

#include "base.hpp"
#include "mkrunnable.hpp"
#include "syscall.hpp"

#define THREAD_WAITING          -1        /* thread is waiting */
#define THREAD_AWAKENED          0        /* normal wakeup */
#define THREAD_TIMED_OUT         1        /* timeout expired */
#define THREAD_INTERRUPTED       2        /* aborted/interrupted */
#define THREAD_RESTART           3        /* restart operation entirely */
#define THREAD_NOT_WAITING      10       /* thread didn't need to wait */

class WaitEvent : public EventBase {
    int wait_result;
    uint64_t wait_resource;
    std::string wait_resource_desc;
    SyscallEvent *syscall_event;

    const char *decode_wait_result(void);
public :
    WaitEvent(double timestamp, std::string op, uint64_t tid, uint64_t wait_resource,
        uint32_t coreid, std::string procname = "");
    void pair_syscall(SyscallEvent *syscall) {syscall_event = syscall;}
    SyscallEvent *get_syscall(void) {return syscall_event;}
    void pair_mkrun(MakeRunEvent *mkrun) {set_event_peer(mkrun);}
    MakeRunEvent *get_mkrun(void) {return dynamic_cast<MakeRunEvent *>(get_event_peer());} 
    void set_wait_result(int result) {wait_result = result;}
    int get_wait_result(void) {return wait_result;}
    uint64_t get_wait_event() {return wait_resource;}

    void set_wait_resource(char *s) {wait_resource_desc = s;}
    std::string get_wait_resource(void) {return wait_resource_desc;}

    double get_time_cost(void);
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};
#endif
