#include "rlboundary.hpp"

RunLoopBoundaryEvent::RunLoopBoundaryEvent(double timestamp, std::string op, uint64_t tid, uint32_t _state, uint32_t event_core, std::string procname)
:EventBase(timestamp, RL_BOUNDARY_EVENT, op, tid, event_core, procname)
{
    state = _state;
    consumer = owner = nullptr;
    rls = block = item = 0;
    //rls = func_ptr = block = item = 0;
}

const char * RunLoopBoundaryEvent::decode_state(int state)
{
    switch(state) {
        case CategoryBegin:
            return "CategoryBegin";
        case CategoryEnd:
            return "CategoryEnd";
        case ItemBegin:
            return "ItemBegin";
        case ItemEnd:
            return "ItemEnd";
        case SetSignalForSource0:
            return "SetRLSource0";
        case UnsetSignalForSource0:
            return "UnsetRLSource0";
        case BlockPerform:
            return "BlockPerform";
        case EventsBegin:
            return "EventBegin";
        case EventsEnd:
            return "EventEnd";
        default:
            return "unknown";
    }
}

void RunLoopBoundaryEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\nstate = " << decode_state(state) << std::endl;
    if (func_symbol.size())
        outfile << "\nCallback = " << func_symbol << std::endl;
    if (owner)
        outfile << "\nConnected by event at " << std::fixed << std::setprecision(1) << owner->get_abstime() << std::endl;
    if (rls)
        outfile << "\nRls = " << rls << std::endl; 
}

void RunLoopBoundaryEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\tstate = " << decode_state(state);
    if (func_symbol.size())
        outfile << "\n\tCallback = " << func_symbol;
    if (owner)
        outfile << "\n\tConnected by event at " << std::fixed << std::setprecision(1) << owner->get_abstime();
    if (rls)
        outfile << "\n\tRls = " << rls;
    if (block)
        outfile << "\n\tblock = " << block;
    outfile << std::endl;
}
