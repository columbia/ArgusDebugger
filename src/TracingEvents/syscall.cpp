#include "syscall.hpp"
#define DEBUG_SYSCALL_EVT 0

SyscallEvent::SyscallEvent(double timestamp, std::string op, uint64_t tid, uint64_t sc_class, uint32_t event_core, std::string procname)
:EventBase(timestamp, SYSCALL_EVENT, op, tid, event_core, procname)
{
    syscall_class = sc_class;
    sc_entry = nullptr;
    memset(args, -1, sizeof(uint64_t) * MAX_ARGC);
    nargs = 0;
    ret = -1;
}

bool SyscallEvent::audit_args_num(int size)
{
    if (!sc_entry) {
#if DEBUG_SYSCALL_EVT
        LOG_S(INFO) << "sc is null at " << std::fixed << std::setprecision(1) << get_abstime();
#endif
        return false;
    }
    if (nargs + size > MAX_ARGC) { //|| !sc_entry || !sc_entry->args[nargs])
#if DEBUG_SYSCALL_EVT
        LOG_S(INFO) << "args number " << nargs + size << " exceeds " << MAX_ARGC\
            << " at " << std::fixed << std::setprecision(1) << get_abstime();
#endif
        return false;
    }
    if (!sc_entry->args[nargs]) {
#if DEBUG_SYSCALL_EVT
        LOG_S(INFO) << "args index " << nargs << " is invalid at "\
            << std::fixed << std::setprecision(1) << get_abstime()\
            << "; prev syscall doesn't returned. Most possible a new syscall event begins.";
#endif
        return false;
    }
    return true;
}

bool SyscallEvent::set_args(uint64_t *arg_array, int size)
{
    if (audit_args_num(1) == false)
        return false;
    std::copy(arg_array, arg_array + size, args + nargs);
    nargs += size;
    return true;
}

uint64_t SyscallEvent::get_arg(int idx)
{
    if (idx < nargs && sc_entry->args[idx] != nullptr)
        return args[idx];
    assert(idx < nargs && sc_entry->args[idx] != nullptr);
    return 0;
}

const char *SyscallEvent::class_name()
{
    switch(syscall_class) {
        case MSC_SYSCALL:
            return "mach_syscall";
        case BSC_SYSCALL:
            return "bsd_syscall";
        default:
            return "unknown_syscall";
    }
}

void SyscallEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\n\t" << class_name();
    if (get_finish_time() > 0)
        outfile << "\n\treturn = " << std::hex << ret << std::endl;
    if (!sc_entry)
        return;

    outfile << "\n\t" << sc_entry->syscall_name;
    for (int i = 0; i < MAX_ARGC; i++)
        if (sc_entry->args[i])
            outfile << "\n\t" << std::hex << sc_entry->args[i] << " = " << args[i];
}

void SyscallEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    if (get_finish_time() > 0) {
        outfile << "\t" << std::fixed << std::setprecision(1) << get_finish_time();
        outfile << "\tret = " << std::hex << ret;
    }
    if (!sc_entry) {
        outfile << std::endl;
        return;
    }

    for (int i = 0; i < MAX_ARGC; i++)
        if (sc_entry->args[i] != nullptr)
            outfile << ",\t" << std::hex << sc_entry->args[i] << " = " << args[i];

    outfile << std::endl;
}
