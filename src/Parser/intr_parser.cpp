#include "parser.hpp"
#include "interrupt.hpp"

Parse::IntrParser::IntrParser(std::string filename)
:Parser(filename)
{
    intr_events.clear();
}

#if defined(__APPLE__)
void Parse::IntrParser::symbolize_intr_for_proc(BacktraceParser *backtrace_parser, 
	Parse::key_t proc)
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
        IntrEvent *intr_event;

        for (it = event_list.begin(); it != event_list.end(); it++) {
            intr_event = dynamic_cast<IntrEvent *>(*it);
            if (!intr_event || !intr_event->get_user_mode())
                continue;
            Frames frameinfo(0, 1, intr_event->get_tid());
            frameinfo.add_frame(intr_event->get_rip());
            if (frameinfo.symbolize_with_lldb(0, &cur_debugger))
                intr_event->set_context(frameinfo.get_sym(0) + "_at_" + frameinfo.get_filepath(0));
            intr_event->streamout_event(out_bt);
        }
        std::cerr << "finish rip symbolization." << std::endl;
        out_bt.close();
    }

    if (cur_debugger.debugger.IsValid()) {
        if (cur_debugger.cur_target.IsValid())
            cur_debugger.debugger.DeleteTarget(cur_debugger.cur_target);
		cur_debugger.debugger.Clear();
        lldb::SBDebugger::Destroy(cur_debugger.debugger);
    }
}
#endif

IntrEvent *Parse::IntrParser::create_intr_event(double abstime, std::string opname,
        std::istringstream & iss) 
{
    std::string procname;
    uint64_t intr_no, rip, user_mode, itype, tid, coreid;

    if (!(iss >> std::hex >> intr_no >> rip >> user_mode >> itype) ||
            !(iss >> std::hex >> tid >> coreid))
        return nullptr;
    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = ""; 
    if (intr_events.find(tid) != intr_events.end())
        return nullptr;
    IntrEvent *new_intr = new IntrEvent(abstime, opname, tid, intr_no, rip,
            user_mode, coreid, procname);
    if (!new_intr) {
        std::cerr << "OOM " << __func__ << std::endl;
        exit(EXIT_FAILURE);
    }
    intr_events[tid] = new_intr;
    local_event_list.push_back(new_intr);
	Parse::key_t key = {.first = LoadData::tid2pid(tid), .second = procname};
	add_to_proc_map(key, new_intr);
    return new_intr;
}

bool Parse::IntrParser::gather_intr_info(double abstime, std::istringstream &iss)
{
    std::string procname;
    uint64_t intr_no, ast, effective_policy, req_policy, tid, coreid;

    if (!(iss >> std::hex >> intr_no >> ast >> effective_policy >> req_policy)
            || !(iss >> std::hex >> tid >> coreid))
        return false;
    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = ""; 
    if (intr_events.find(tid) == intr_events.end())
        return false;
    IntrEvent * intr_event = intr_events[tid];
    if ((uint32_t)intr_no != intr_event->get_interrupt_num())
        return false;
    intr_event->set_sched_priority_post((int16_t)(intr_no >> 32));
    intr_event->set_ast(ast);
    //intr_event->set_effective_policy(effective_policy);
    //intr_event->set_request_policy(req_policy);
    intr_event->set_finish_time(abstime);
    intr_event->set_complete();
    intr_events.erase(tid);
    return true;
}

void Parse::IntrParser::process()
{
    std::string line, deltatime, opname;
    double abstime;
    bool is_begin;

    while (getline(infile, line)) {
        std::istringstream iss(line);

        if (!(iss >> abstime >> deltatime >> opname)) {
            outfile << line << std::endl;
            continue;
        }

        is_begin = deltatime.find("(") != std::string::npos ? false : true;
        if (is_begin) {
            if (!create_intr_event(abstime, opname, iss))
                outfile  << line << std::endl;
        } else {
            if (!gather_intr_info(abstime, iss))
                outfile << line << std::endl;
        }
    }
}
