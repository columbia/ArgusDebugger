#include "parser.hpp"
#include "rlboundary.hpp"

Parse::RLBoundaryParser::RLBoundaryParser(std::string filename)
:Parser(filename)
{}

void Parse::RLBoundaryParser::process()
{
    std::string line, deltatime, opname, procname;
    uint64_t state, object, block, unused, tid, coreid;
    double abstime;

    while (getline(infile, line)) {
        std::istringstream iss(line);
        if (!(iss >> abstime >> deltatime >> opname \
               >> std::hex >> state >> object >> block >> unused \
               >> std::dec >> tid >> coreid)) {
            outfile << line << std::endl;
            continue;
        }
        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = ""; 
        RunLoopBoundaryEvent *rl_boundary_event = nullptr;

        /*
        if (state == SetSignalForSource0 || state == UnsetSignalForSource0) {
            rl_boundary_event = new RunLoopBoundaryEvent(abstime, opname, tid, state, 0, coreid, procname);
            rl_boundary_event->set_rls(object);
        } else {
            rl_boundary_event = new RunLoopBoundaryEvent(abstime, opname, tid, state, object, coreid, procname);
            //if (opname =="ARGUS_RL_DoBlocks" && (state == ItemEnd || state == BlockPerform))
            if (opname =="ARGUS_RL_DoBlocks") {
                rl_boundary_event->set_item(object);
                rl_boundary_event->set_block(block);
            }
        }
        */
        rl_boundary_event = new RunLoopBoundaryEvent(abstime, opname, tid, state, coreid, procname);
        if (!rl_boundary_event) {
            std::cerr << "OOM " << __func__ << std::endl;
            exit(EXIT_FAILURE);
        }
        if (state == SetSignalForSource0 || state == UnsetSignalForSource0) {
            rl_boundary_event->set_rls(object);
        }
        if (opname == "ARGUS_RL_DoBlocks") {
            rl_boundary_event->set_item(object);
            rl_boundary_event->set_block(block);
        }
        rl_boundary_event->set_complete();
        local_event_list.push_back(rl_boundary_event);
		Parse::key_t key = {.first = LoadData::tid2pid(tid), .second = procname};
		add_to_proc_map(key, rl_boundary_event);
    }
}

#if defined(__APPLE__)
void Parse::RLBoundaryParser::symbolize_rlblock_for_proc(BacktraceParser *parser,
	key_t proc)
{

    images_t *image = nullptr;
    debug_data_t cur_debugger;
    event_list_t event_list = get_events_for_proc(proc);
    
    if (!event_list.size() || !parser || !(image = parser->proc_to_image(proc.first, proc.second)))
        return;
    if (parser->setup_lldb(&cur_debugger, image)) {
        std::cerr << "load lldb for rlboundary events ..." << std::endl;
        std::string outfile("rlboundary_func_ptr.log");
        std::ofstream out_bt(outfile, std::ofstream::out | std::ofstream::app);
        event_list_t::iterator it;
        RunLoopBoundaryEvent *rl_boundary_event;
        
        for (it = event_list.begin(); it != event_list.end(); it++) {
            rl_boundary_event = dynamic_cast<RunLoopBoundaryEvent *>(*it);
            if (!rl_boundary_event)
                continue;
            Frames frameinfo(0, 1, rl_boundary_event->get_tid());
            frameinfo.add_frame(rl_boundary_event->get_func_ptr());
            if (frameinfo.symbolize_with_lldb(0, &cur_debugger))
                rl_boundary_event->set_func_symbol(frameinfo.get_sym(0) + frameinfo.get_filepath(0));
            rl_boundary_event->streamout_event(out_bt);
        }

        out_bt.close();
        std::cerr << "finish dispatch event symbolization." << std::endl;
    }

    if (cur_debugger.debugger.IsValid()) {
        if (cur_debugger.cur_target.IsValid())
            cur_debugger.debugger.DeleteTarget(cur_debugger.cur_target);
        lldb::SBDebugger::Destroy(cur_debugger.debugger);
    }
}
#endif
