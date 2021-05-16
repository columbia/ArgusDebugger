#include "parser.hpp"
#include "dispatch.hpp"

Parse::DispatchParser::DispatchParser(std::string filename)
:Parser(filename)
{
    dequeue_events.clear();
}


bool Parse::DispatchParser::is_duplicate_deq(BlockDequeueEvent *prev, BlockDequeueEvent *cur)
{
    if (prev->get_ref() != 3)
        return false;
    if (prev->get_qid() != cur->get_qid() 
            || prev->get_item() != cur->get_item()
            || prev->get_func_ptr() != cur->get_func_ptr()
            || prev->get_invoke_ptr() != cur->get_invoke_ptr())
        return false;
    return true;
}

bool Parse::DispatchParser::checking_block_invoke_pair(BlockInvokeEvent *new_invoke) 
{
    if (new_invoke->is_begin()) {
        dispatch_block_invoke_begin_list.push_back(new_invoke);
        return true;
    } else {
        std::list<BlockInvokeEvent *>::reverse_iterator rit;
        for (rit = dispatch_block_invoke_begin_list.rbegin(); 
                rit != dispatch_block_invoke_begin_list.rend(); rit++) {
            BlockInvokeEvent *invokebegin = *rit;
            if (invokebegin->get_tid() == new_invoke->get_tid()
                    && invokebegin->get_func() == new_invoke->get_func()) {
                new_invoke->set_root(invokebegin);
                outfile << "Pair of blockexecution :" \
                    << "\nBegin at " << std::fixed << std::setprecision(1) \
                    << invokebegin->get_abstime() \
                    << "\nEnd  at " << std::fixed << std::setprecision(1) \
                    << new_invoke->get_abstime() \
                    << "\n------" << std::endl;
                //delete the item
                dispatch_block_invoke_begin_list.erase(next(rit).base());
                return true;
            }
        }
        return false;
    }
}

#if defined(__APPLE__)
void Parse::DispatchParser::symbolize_block_dequeue(debug_data_t *cur_debugger_ptr,
    event_list_t event_list)
{
    std::string outfile("dq_bt.log");
    std::ofstream out_bt(outfile, std::ofstream::out | std::ofstream::app);

    event_list_t::iterator it;
    BlockDequeueEvent *dequeue_event;
    std::string desc;

    for (it = event_list.begin(); it != event_list.end(); it++) {
        if (!(dequeue_event = dynamic_cast<BlockDequeueEvent *>(*it)))
            continue;
        Frames frame_info(0, 3, dequeue_event->get_tid());
        frame_info.add_frame(dequeue_event->get_func_ptr());
        frame_info.add_frame(dequeue_event->get_invoke_ptr());
        frame_info.add_frame(dequeue_event->get_ctxt());
        frame_info.symbolize_with_lldb(0, cur_debugger_ptr);
        frame_info.symbolize_with_lldb(1, cur_debugger_ptr);
        frame_info.symbolize_with_lldb(2, cur_debugger_ptr);
        desc = "_func_";
        desc.append(frame_info.get_sym(0));
        desc.append("_invoke_");
        desc.append(frame_info.get_sym(1));
            desc.append("_ctxt_");
        desc.append(frame_info.get_sym(2));
        dequeue_event->set_desc(desc);
        dequeue_event->streamout_event(out_bt);        
        desc.clear();
    }
    out_bt.close();
}

void Parse::DispatchParser::symbolize_block_invoke(debug_data_t *cur_debugger_ptr,
        event_list_t event_list)
{
    std::string outfile("dq_bt.log");
    std::ofstream out_bt(outfile, std::ofstream::out | std::ofstream::app);

    BlockInvokeEvent *block_invoke_event;
    event_list_t::iterator it;
    std::string desc;

    for (it = event_list.begin(); it != event_list.end(); it++) {
        if (!(block_invoke_event = dynamic_cast<BlockInvokeEvent *>(*it)))
            continue;
        Frames frame_info(0, 2, block_invoke_event->get_tid());
        frame_info.add_frame(block_invoke_event->get_func());
        frame_info.add_frame(block_invoke_event->get_ctxt());
        frame_info.symbolize_with_lldb(0, cur_debugger_ptr);
        frame_info.symbolize_with_lldb(1, cur_debugger_ptr);
        desc = "_func_";
        desc.append(frame_info.get_sym(0));
        desc.append(frame_info.get_filepath(0));
        desc.append("_ctxt_");
        desc.append(frame_info.get_sym(1));
        desc.append(frame_info.get_filepath(1));
        block_invoke_event->set_desc(desc);
        block_invoke_event->streamout_event(out_bt);
        desc.clear();
    }
    out_bt.close();
}


void Parse::DispatchParser::symbolize_block_for_proc(BacktraceParser *backtrace_parser,
	key_t proc)
{
    images_t *image = nullptr;
    debug_data_t cur_debugger;
    event_list_t event_list = get_events_for_proc(proc);

    if (!event_list.size() || !backtrace_parser || !(image = backtrace_parser->proc_to_image(proc.first, proc.second)))
        return;

   if (backtrace_parser->setup_lldb(&cur_debugger, image)) {
    
        std::cerr << "load lldb for dispatch events ..." << std::endl;

        switch (event_list.front()->get_event_type()) {
            case DISP_INV_EVENT:
                symbolize_block_invoke(&cur_debugger, event_list);
                break;
            case DISP_DEQ_EVENT:
                symbolize_block_dequeue(&cur_debugger, event_list);
                break;
            default:
                break;
        }
        std::cerr << "finish dispatch event symbolization." << std::endl;
    }

    if (cur_debugger.debugger.IsValid()) {
        if (cur_debugger.cur_target.IsValid())
            cur_debugger.debugger.DeleteTarget(cur_debugger.cur_target);
		cur_debugger.debugger.Clear();
        lldb::SBDebugger::Destroy(cur_debugger.debugger);
    }
}
#endif

bool Parse::DispatchParser::process_enqueue(std::string opname, double abstime,
        std::istringstream &iss)
{
    uint64_t q_id, item, ref = 0, unused, tid, coreid;
    std::string procname = "";
    if (!(iss >> std::hex >> ref >> q_id >> item >> unused >> tid >> coreid))
        return false;
    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = "";
    if (!(ref >> 32)) {
        BlockEnqueueEvent *new_enqueue = new BlockEnqueueEvent(abstime, opname,
                tid, q_id, item, ref, coreid, procname);

        if (!new_enqueue) {
            std::cerr << "OOM: " << __func__ << std::endl;
            return false;
        }

        new_enqueue->set_complete();
        local_event_list.push_back(new_enqueue);
		Parse::key_t key = {.first = LoadData::tid2pid(tid), .second = procname};
        add_to_proc_map(key, new_enqueue);
        return true;
    }
    return true;
}

bool Parse::DispatchParser::gather_dequeue_info(double abstime, uint32_t ref,
        std::istringstream &iss)
{
    uint64_t func_ptr, invoke_ptr, vtable_ptr, tid, corid;
    if (!(iss >> std::hex >> func_ptr >> invoke_ptr >> vtable_ptr >> \
                tid >> corid))
        return false;

    if (dequeue_events.find(tid) == dequeue_events.end()) {
        std::cerr << "Check : missing info ";
        std::cerr << std::fixed << std::setprecision(1) << abstime << std::endl;
        return false;
    }

    BlockDequeueEvent *dequeue_event = dequeue_events[tid];
    assert(dequeue_event->get_ref() == ref);
    dequeue_event->set_ptrs(func_ptr, invoke_ptr, vtable_ptr);
    dequeue_event->set_complete();
    dequeue_events.erase(tid);
    return true;
}

bool Parse::DispatchParser::create_dequeue_event(uint32_t ref, std::string opname,
        double abstime, std::istringstream &iss)
{
    uint64_t q_id, item, ctxt, tid, coreid;
    std::string procname;

    if (!(iss >> std::hex >> q_id >> item >> ctxt >> tid >> coreid))
        return false;
    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = "";
    if (dequeue_events.find(tid) != dequeue_events.end()) {
        std::cerr << "mismatched dequeue event at ";
        std::cerr << std::fixed << std::setprecision(1) << abstime << std::endl;
    }

    BlockDequeueEvent *new_dequeue = new BlockDequeueEvent(abstime,
            opname, tid, q_id, item, ctxt, ref, coreid, procname);

    if (!new_dequeue) {
        std::cerr << "OOM: " << __func__ << std::endl;
        return false;
    }
    local_event_list.push_back(new_dequeue);
    dequeue_events[tid] = new_dequeue;
	Parse::key_t key = {.first = LoadData::tid2pid(tid), .second = procname};
	add_to_proc_map(key, new_dequeue);
    return true;
}

bool Parse::DispatchParser::process_dequeue(std::string opname, double abstime,
        std::istringstream &iss)
{
    uint64_t ref;
    if (!(iss >> std::hex >> ref))
        return false;
    if (Parse::hi(ref) == 0) {
        return create_dequeue_event(Parse::lo(ref), opname, abstime, iss);
    } else {
        assert(Parse::hi(ref) == 1);
        return gather_dequeue_info(abstime, Parse::lo(ref), iss);
    }
}

bool Parse::DispatchParser::process_block_invoke(std::string opname, double abstime,
        std::istringstream &iss)
{
    uint64_t func, _ctxt, _begin, unused, tid, coreid;
    std::string procname = "";
    if (!(iss >> std::hex >> func >> _ctxt >> _begin >> unused >> tid >> coreid))
        return false;
    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = "";

    BlockInvokeEvent *new_invoke = new BlockInvokeEvent(abstime, opname,
            tid, func, _ctxt, _begin - 1, coreid, procname);

    if (!new_invoke) {
        std::cerr << "OOM: " << __func__ << std::endl;
        exit(EXIT_FAILURE);
    }

    if (checking_block_invoke_pair(new_invoke) == false) {
        outfile << "Check: a block_invoke end missing counterpart begin.";
        outfile << "It is normal if the invoke begins before tracing." << std::endl;
        delete (new_invoke);
        return false;
    }

    new_invoke->set_complete();
    local_event_list.push_back(new_invoke);
	Parse::key_t key = {.first = LoadData::tid2pid(tid), .second = procname};
	add_to_proc_map(key, new_invoke);
    return true;
}

bool Parse::DispatchParser::process_migservice(std::string opname, double abstime, std::istringstream &iss)
{
    uint64_t unused, tid, coreid;
    std::string procname = "";
    if (!(iss >> std::hex >> unused >> unused >> unused >> unused >> tid >> coreid))
        return false;
    if (!getline(iss >> std::ws, procname) || !procname.size())
        procname = "";
    DispatchQueueMIGServiceEvent *mig_service = new DispatchQueueMIGServiceEvent(abstime, opname, tid, coreid, procname);
    if (!mig_service) {
        std::cerr << "OOM: " << __func__ << std::endl;
        exit(EXIT_FAILURE);
    }
    mig_service->set_complete();
    local_event_list.push_back(mig_service);
    return true;
}

void Parse::DispatchParser::process()
{
    std::string line, deltatime, opname;
    double abstime;
    bool ret = false;
    while(getline(infile, line)) {
        std::istringstream iss(line);
        if (!(iss >> abstime >> deltatime >> opname)) {
            outfile << line << std::endl;
            continue;
        }
        switch (LoadData::map_op_code(0, opname)) {
            case DISP_ENQ:
                ret = process_enqueue(opname, abstime, iss);
                break;
            case DISP_DEQ:
                ret = process_dequeue(opname, abstime, iss);
                break;
            case DISP_EXE:
                ret = process_block_invoke(opname, abstime, iss);
                break;
            case DISP_MIG:
                ret = process_migservice(opname, abstime, iss);
                break;
            default:
                ret = false;
                outfile << "unknown operation" << std::endl;
        }
        if (ret == false)
            outfile << line << std::endl;
    }
    //TODO check remainingstd::maps
}
