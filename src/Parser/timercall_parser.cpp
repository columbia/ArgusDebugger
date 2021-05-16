#include "parser.hpp"
#include "timer_call.hpp"

Parse::TimercallParser::TimercallParser(std::string filename)
:Parser(filename)
{}


#if defined(__APPLE__)
void Parse::TimercallParser::symbolize_func_ptr_for_proc(BacktraceParser *backtrace_parser,
	key_t proc)
{
    images_t *image = nullptr;
    debug_data_t cur_debugger;
    event_list_t event_list = get_events_for_proc(proc);

    if (!event_list.size() || !backtrace_parser || !(image = backtrace_parser->proc_to_image(proc.first, proc.second)))
        return;

    if ((backtrace_parser->setup_lldb(&cur_debugger, image))) {
        std::cerr << "load lldb for symbolize rips ..." << std::endl;
        std::string outfile("intr_bt.log");
        std::ofstream out_bt(outfile, std::ofstream::out | std::ofstream::app);

        event_list_t::iterator it;
        TimerCreateEvent *timer_event;

        for (it = event_list.begin(); it != event_list.end(); it++) {
            timer_event = dynamic_cast<TimerCreateEvent *>(*it);

            if (!timer_event || !timer_event->get_func_ptr())
                continue;
            Frames frameinfo(0, 1, timer_event->get_tid());
            frameinfo.add_frame((uint64_t)(timer_event->get_func_ptr()));
            if (frameinfo.symbolize_with_lldb(0, &cur_debugger))
                timer_event->set_func_symbol(frameinfo.get_sym(0) + "_at_" + frameinfo.get_filepath(0));
        }
    } 
    if (cur_debugger.debugger.IsValid()) {
        if (cur_debugger.cur_target.IsValid())
            cur_debugger.debugger.DeleteTarget(cur_debugger.cur_target);
		cur_debugger.debugger.Clear();
        lldb::SBDebugger::Destroy(cur_debugger.debugger);
    }
}
#endif


bool Parse::TimercallParser::process_timercreate(std::string opname, double abstime,
        std::istringstream &iss)
{
    uint64_t func_ptr, param0, param1, q_ptr, tid, coreid;
    std::string procname = "";
    if (!(iss >> std::hex >> func_ptr >> param0 >> param1 >> \
                q_ptr >> tid >> coreid))
        return false;
    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = "";
    TimerCreateEvent *timercreate_event = new TimerCreateEvent(abstime,
            opname, tid, coreid, (void*)func_ptr,
            param0, param1, (void*)q_ptr, procname);
    local_event_list.push_back(timercreate_event);
	Parse::key_t key = {.first = LoadData::tid2pid(tid), .second = procname};
	add_to_proc_map(key, timercreate_event);
    timercreate_event->set_complete();
    return true;
}

bool Parse::TimercallParser::process_timercallout(std::string opname, double abstime,
        std::istringstream &iss)
{
    uint64_t func_ptr, param0, param1, q_ptr, tid, coreid;
    std::string procname = "";

    if (!(iss >> std::hex >> func_ptr >> param0 >> param1 >> q_ptr \
                >> tid >> coreid))
        return false;

    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = "";

    TimerCalloutEvent *timercallout_event = new TimerCalloutEvent(abstime, \
            opname, tid, coreid, (void*)func_ptr,
            param0, param1, (void*)q_ptr, procname);

    local_event_list.push_back(timercallout_event);
    timercallout_event->set_complete();
    return true;
}

bool Parse::TimercallParser::process_timercancel(std::string opname, double abstime,
        std::istringstream &iss)
{
    uint64_t func_ptr, param0, param1, q_ptr, tid, coreid;
    std::string procname = "";
    if (!(iss >> std::hex >> func_ptr >> param0 >> param1 >> q_ptr >> tid \
                >> coreid))
        return false;
    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = "";
    TimerCancelEvent * timercancel_event = new TimerCancelEvent(
            abstime - 0.05, opname, tid, coreid, (void*)func_ptr, param0,
            param1, (void*)q_ptr, procname);
    local_event_list.push_back(timercancel_event);
    timercancel_event->set_complete();
    return true;
}

void Parse::TimercallParser::process()
{
    std::string line, deltatime, opname;
    double abstime;
    bool ret = false;

    while (getline(infile, line)) {
        std::istringstream iss(line);
        if (!(iss >> abstime >> deltatime >> opname)) {
            outfile << line << std::endl;
            continue;
        }
        switch (LoadData::map_op_code(0, opname)) {
            case MACH_CALLCREATE:
                ret = process_timercreate(opname, abstime, iss);
                break;
            case MACH_CALLOUT:
                ret = process_timercallout(opname, abstime, iss);
                break;
            case MACH_CALLCANCEL:
                ret = process_timercancel(opname, abstime, iss);
                break;
            default:
                ret = false;
                std::cerr << "unknown op" << std::endl;
        }
        if (ret == false)
            outfile << line << std::endl;
    }
}
