#include "parser.hpp"
#include "breakpoint_trap.hpp"

Parse::BreakpointTrapParser::BreakpointTrapParser(std::string filename)
:Parser(filename)
{
}

#if 0
void Parse::BreakpointTrapParser::symbolize_addr(
    BacktraceParser *backtrace_parser,
    debug_data_t *cur_debugger,
    images_t *image, 
    BreakpointTrapEvent *breakpoint_trap_event)
{
    std::vector<uint64_t> addrs = breakpoint_trap_event->get_addrs();
    Frames frame_info(0, addrs.size(), breakpoint_trap_event->get_tid());
    frame_info.set_image(image);
    
    for (int i = 0; i < addrs.size(); i++) {
        frame_info.add_frame(addrs[i]);
    //
    //}
    //symbolication(cur_debugger, backtrace_parser->get_symbol_map());
    ///for (int i = 0; i < addrs.size(); i++) {
        frame_info.update_frame_path(i, image->search_path(addrs[i]));
        frame_info.correct_symbol(i, backtrace_parser->get_symbol_map());
        breakpoint_trap_event->update_target(i, frame_info.get_sym(i));
    }
}


void Parse::BreakpointTrapParser::symbolize_eip(
        BacktraceParser *backtrace_parser,
        debug_data_t *cur_debugger,
        images_t *image,
        BreakpointTrapEvent *breakpoint_trap_event)
{
    Frames frame_info(0, 1, breakpoint_trap_event->get_tid());
    frame_info.add_frame(breakpoint_trap_event->get_eip());
    frame_info.set_image(image);
    frame_info.symbolication(cur_debugger, backtrace_parser->get_symbol_map());
    breakpoint_trap_event->set_caller_info(frame_info.get_sym(0));
}
#endif

#if defined (__APPLE__)
void Parse::BreakpointTrapParser::symbolize_hwbrtrap_for_proc(
        BacktraceParser *backtrace_parser,
		key_t proc)
{
    images_t *image = nullptr;
    debug_data_t cur_debugger;
    event_list_t event_list = get_events_for_proc(proc);
       //proc_event_list_map[procname];

    if (!event_list.size() || !backtrace_parser || !(image = backtrace_parser->proc_to_image(proc.first, proc.second))) {
        std::cerr << "warning: lldb fail to get image for " << proc.first << " " << proc.second << std::endl;
        return;
    }

    if (backtrace_parser->setup_lldb(&cur_debugger, image)) {
        std::string outfile("hwbrtrap_bts.log");
        std::ofstream out_bt(outfile, std::ofstream::out | std::ofstream::app);
        std::cerr << "load lldb for symbolize hwbr caller... in " << proc.second << std::endl;

        event_list_t::iterator it;
        BreakpointTrapEvent *breakpoint_trap_event;
        for (it = event_list.begin(); it != event_list.end(); it++) {
            breakpoint_trap_event = dynamic_cast<BreakpointTrapEvent *>(*it);
            std::vector<addr_t> addrs = breakpoint_trap_event->get_addrs();
            Frames frame_info(0, addrs.size() + 1, breakpoint_trap_event->get_tid());
            frame_info.set_image(image);

            frame_info.add_frame(breakpoint_trap_event->get_eip());
            frame_info.symbolication(&cur_debugger, backtrace_parser->get_symbol_map());
            breakpoint_trap_event->set_caller(frame_info.get_sym(0));
    
            for (int i = 0; i < addrs.size(); i++) {
                frame_info.add_frame(addrs[i]);
                frame_info.update_frame_path(i + 1, image->search_path(addrs[i]));
                frame_info.correct_symbol(i + 1, backtrace_parser->get_symbol_map());
                breakpoint_trap_event->update_target(i, frame_info.get_sym(i + 1));
            }
            breakpoint_trap_event->streamout_event(out_bt);
        }
        out_bt.close();
    }

    if (cur_debugger.debugger.IsValid()) {
        if (cur_debugger.cur_target.IsValid())
            cur_debugger.debugger.DeleteTarget(cur_debugger.cur_target);
		cur_debugger.debugger.Clear();
        lldb::SBDebugger::Destroy(cur_debugger.debugger);
    }
}

void Parse::BreakpointTrapParser::symbolize_hwbrtrap(BacktraceParser *backtrace_parser)
{
    for (auto it = proc_event_list_map.begin(); it != proc_event_list_map.end(); it++)
        symbolize_hwbrtrap_for_proc(backtrace_parser, it->first);
}
#endif

void Parse::BreakpointTrapParser::process()
{
    std::string line, deltatime, opname, procname;
    uint64_t eip, arg1, arg2, arg3, tid, coreid;
    double abstime;

    while (getline(infile, line)) {
        std::istringstream iss(line);

        if (!(iss >> abstime >> deltatime >> opname \
                >> std::hex >> eip >> arg1 >> arg2 >> arg3 \
                >> std::dec >> tid >> coreid)) {
            outfile << line << std::endl;
            continue;
        }
        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = ""; 

        if (breakpoint_trap_events.find(tid) == breakpoint_trap_events.end()) {
            BreakpointTrapEvent *breakpoint_trap_event
                = new BreakpointTrapEvent(abstime, opname, tid, eip, coreid, procname);
            if (!breakpoint_trap_event) {
                std::cerr << "OOM " << __func__ << std::endl;
                exit(EXIT_FAILURE);
            }
            breakpoint_trap_events[tid] = breakpoint_trap_event;
            breakpoint_trap_event->add_value(arg1 >> 32);
            breakpoint_trap_event->add_value((uint32_t)arg1);
            breakpoint_trap_event->add_value(arg2 >> 32);
            breakpoint_trap_event->add_value((uint32_t)arg2);
            if (arg3)
                breakpoint_trap_event->add_addr(arg3);
            local_event_list.push_back(breakpoint_trap_event);
			key_t key = {.first = LoadData::tid2pid(tid), .second = procname};
            add_to_proc_map(key, breakpoint_trap_event);
        } else {
            BreakpointTrapEvent *breakpoint_trap_event = breakpoint_trap_events[tid];
            if (arg1)
                breakpoint_trap_event->add_addr(arg1);
            if (arg2)
                breakpoint_trap_event->add_addr(arg2);
            if (arg3)
                breakpoint_trap_event->add_addr(arg3);
            breakpoint_trap_events.erase(tid);
            breakpoint_trap_event->set_complete();
        }
    }
}

