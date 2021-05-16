#include "parser.hpp"
Parse::SyscallParser::SyscallParser(std::string filename)
:Parser(filename)
{
    syscall_events.clear();
}

bool Parse::SyscallParser::new_msc(SyscallEvent *syscall_event, uint64_t tid,
	std::string opname, uint64_t *args, int size)
{

    std::string syscall_name = opname.substr(opname.find("MSC_") + 4);
    if (LoadData::msc_name_index_map.find(syscall_name)
            != LoadData::msc_name_index_map.end()) {
        const struct syscall_entry *entry
            = &LoadData::mach_syscall_table[LoadData::msc_name_index_map[syscall_name]];
        syscall_event->set_entry(entry);
    } else {
        outfile << "Check: unrecognize syscall entry " << opname << std::endl;
        return false;
    }

    if (syscall_event->set_args(args, size) == false) {
        LOG_S(INFO) << "Fail to set args for syscall at "\
			<< std::fixed << std::setprecision(1) << syscall_event->get_abstime();
        assert(false);
    }
    local_event_list.push_back(syscall_event);
    syscall_events[tid] = syscall_event;
    return true;
}

bool Parse::SyscallParser::process_msc(std::string opname, double abstime, bool is_end, std::istringstream &iss)
{
    uint64_t args[4], tid, coreid;
    std::string procname = "";
    SyscallEvent *syscall_event = nullptr;

    if (!(iss >> std::hex >> args[0] >> args[1] >> args[2] >> args[3] >> tid >> coreid))
        return false;

    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = "";

    if (syscall_events.find(tid) != syscall_events.end()) {
        //explit remove from event map when is_end is reached
        syscall_event = syscall_events[tid];
        if ((syscall_event->get_op() != opname) || (!is_end && !syscall_event->audit_args_num(1))) {
            syscall_event->set_complete();
            syscall_events.erase(tid);
            outfile << "Check: when processing " << opname << " at " << std::fixed << std::setprecision(1) << abstime \
                <<", possibly previous syscall " << syscall_event->get_op() << " at "\
                << std::fixed << std::setprecision(1) << syscall_event->get_abstime()\
                << " hasn't return" << std::endl;
        } else {
            if (is_end) {
                syscall_event->set_ret(args[0]);
                syscall_event->set_complete();
                syscall_event->set_ret_time(abstime);
                syscall_events.erase(tid);
            }
            else if (syscall_event->set_args(args, 4) == false) {
                LOG_S(INFO) << "Error: Fail to set args for syscall at "\
					<< std::fixed << std::setprecision(1) << abstime;
                assert(false);
                exit(EXIT_FAILURE);
            } 
            return true;
        }
    }

    syscall_event = new SyscallEvent(abstime, opname, tid,
            MSC_SYSCALL, coreid, procname);
    if (!syscall_event) {
        LOG_S(INFO) << "OOM";
        exit(EXIT_FAILURE);
    }
    if (new_msc(syscall_event, tid, opname, args, 4) == false) {
        delete(syscall_event);
        return false;
    }

    if (is_end) {
        syscall_event->set_ret(args[0]);
        syscall_event->set_complete();
        syscall_event->set_ret_time(abstime);
        syscall_events.erase(tid);
    }

    return true;
}

bool Parse::SyscallParser::new_bsc(SyscallEvent *syscall_event, uint64_t tid, std::string opname, uint64_t *args, int size)
{
    std::string syscall_name = opname.substr(opname.find("BSC_") + 4);
    if (LoadData::bsc_name_index_map.find(syscall_name)
            != LoadData::bsc_name_index_map.end()) {
        const struct syscall_entry *entry
            = &LoadData::bsd_syscall_table[LoadData::bsc_name_index_map[syscall_name]];
        syscall_event->set_entry(entry);
    } else {
        outfile << "Check: unrecognize syscall entry " << opname << std::endl;
        return false;
    }

    if (syscall_event->set_args(args, size) == false) {
        LOG_S(INFO) << "Fail to set args for syscall at "\
			<< std::fixed << std::setprecision(1) << syscall_event->get_abstime();
        assert(false);
    }

    local_event_list.push_back(syscall_event);
    syscall_events[tid] = syscall_event;
    return true;
}

bool Parse::SyscallParser::process_bsc(std::string opname, double abstime, bool is_end, std::istringstream &iss)
{
    uint64_t args[4], tid, coreid;
    std::string procname = "";
    SyscallEvent *syscall_event = nullptr;

    if (!(iss >> std::hex >> args[0] >> args[1] >> args[2] >> args[3] >> tid >> coreid)) {
        outfile << "Check: Input failed " << std::fixed << std::setprecision(1) << abstime << std::endl;
        return false;
    }

    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = "";

    if (syscall_events.find(tid) != syscall_events.end()) {
        syscall_event = syscall_events[tid];
        if ((syscall_event->get_op() != opname) || (!is_end && !syscall_event->audit_args_num(1))) {
            syscall_event->set_complete();
            syscall_events.erase(tid);
            outfile << "Check: when processing " << opname << " at " << std::fixed << std::setprecision(1) << abstime \
                <<", " << syscall_event->get_op() << " at " \
				<< std::fixed << std::setprecision(1) << syscall_event->get_abstime()\
                << " doesn't return in kernel" << std::endl;
        } else {
            if (is_end) {
                syscall_event->set_ret(args[0]);
                syscall_event->set_complete();
                syscall_event->set_ret_time(abstime);
                syscall_events.erase(tid);
            }
            else if (syscall_event->set_args(args, 4) == false) {
                LOG_S(INFO) << "Error: Fail to set args for syscall at "\
					<< std::fixed << std::setprecision(1) << abstime;
                exit(EXIT_FAILURE);
            } 
            return true;
        }
    }

    syscall_event = new SyscallEvent(abstime, opname, tid,
            BSC_SYSCALL, coreid, procname);

    if (!syscall_event) {
        LOG_S(INFO) << "OOM";
        exit(EXIT_FAILURE);
    }

    if (new_bsc(syscall_event, tid, opname, args, 4) == false) {
        delete(syscall_event);
        outfile << "Fail to construct BSD syscall "<< std::fixed << std::setprecision(1) << abstime << std::endl;
        return false;
    }

    if (is_end) {
        syscall_event->set_ret(args[0]);
        syscall_event->set_complete();
        syscall_event->set_ret_time(abstime);
        syscall_events.erase(tid);
    }

    return true;
}

void Parse::SyscallParser::process()
{
    std::string line, deltatime, opname;
    double abstime;
    bool ret;

    while(getline(infile, line)) {
        std::istringstream iss(line);

        if (!(iss >> abstime >> deltatime >> opname)) {
            outfile << line << std::endl;
            continue;
        }

        bool is_end = deltatime.find("(") != std::string::npos ? true : false;
        switch (LoadData::map_op_code(0, opname)) {
            case MACH_SYS:
                ret = process_msc(opname, abstime, is_end, iss);
                break;
            case BSD_SYS:
                ret = process_bsc(opname, abstime, is_end, iss);
                break;
            default:
                outfile << "Check unknown input at "  << std::fixed << std::setprecision(1) << abstime << std::endl;
                ret = false;
                break;
        }
        if (ret == false)
            outfile << line << std::endl;
    }
}
