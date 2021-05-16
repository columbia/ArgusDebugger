#include "mkrunnable.hpp"
#include "wait.hpp"
#include <cstring>

MakeRunEvent::MakeRunEvent(double timestamp, std::string op, uint64_t _tid, uint64_t _peer_tid,
    uint64_t _event_source, uint64_t _mr_type, pid_t _pid, pid_t _peer_pid,
    uint32_t _coreid, std::string procname)
:EventBase(timestamp, MR_EVENT, op, _tid, _coreid, procname)
{
    mr_type = _mr_type;
    peer_tid = _peer_tid;
    peer_pid = _peer_pid;
    event_source = _event_source;
    set_pid(_pid);
    wait = nullptr;
    syscall_event = nullptr;
}

bool MakeRunEvent::check_interrupt(IntrEvent *potential_root)
{
	if (potential_root == nullptr) return false;
    double begin = potential_root->get_abstime();
    double end = potential_root->get_finish_time();
    double mr_time = get_abstime();
    if (mr_time - begin >= 10e-8 && end - mr_time >= 10e-8) {
        if (!strcmp(potential_root->decode_interrupt_num(), "TIMER"))
            mr_type = time_out;
        mr_type = external; //count into the weight in node
        return true;
    }
    return false;
}

bool MakeRunEvent::is_weak()
{
    if (syscall_event != nullptr && syscall_event->get_op() == "BSC_workq_kernreturn")
        return true;
	if (mr_type == CLEAR_WAIT)
		return true;
    return false;
}

void MakeRunEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);

    outfile << "\n\tpeer_tid 0x" << std::hex << peer_tid;
    if (wait) {
        outfile << "\n\t" << std::fixed << std::setprecision(1) << "wait_at " << wait->get_abstime();
        outfile << "\n\t" << std::fixed << std::setprecision(1) << "(period = ";
        outfile << get_abstime() - wait->get_abstime() << ")";
    } else {
        outfile << "\n\t[missing wait event]" << std::endl;
    }
    outfile << "\n\tmr_type = " << std::hex << mr_type << std::endl;
    outfile << std::endl;
}

void MakeRunEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
	outfile << std::endl;
}
