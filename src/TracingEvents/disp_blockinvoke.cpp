#include "dispatch.hpp"

BlockInvokeEvent::BlockInvokeEvent(double abstime, std::string op, uint64_t _tid, uint64_t _func,
	uint64_t _ctxt, bool _begin, uint32_t _core, std::string procname)
:EventBase(abstime, DISP_INV_EVENT, op, _tid, _core, procname),
BlockInfo(-1, -1, -1),
FuncInfo(_ctxt, _func)
{
	begin = _begin;
}

void BlockInvokeEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    if (begin)
        outfile << "\n\t Begin_func_" << std::hex << func_ptr;
    else
        outfile << "\n\t End_func_" << std::hex << func_ptr;

    outfile << "\n\tctxt_" << std::hex << ctxt;

    EventBase *peer = get_event_peer();
    if (peer) {
        if (begin) 
            outfile << "\n\tEnqueue by " << std::hex << peer->get_tid()\
                << " at " << std::fixed << std::setprecision(1) << peer->get_abstime() << std::endl;
        else
            outfile  << "\n\tBegin at " << std::fixed << std::setprecision(1) << peer->get_abstime() << std::endl;
    }

    if (desc.size())
        outfile <<"\n\tdesc " << desc << std::endl;

    outfile << "\n\tnested " << nested_level;
    outfile << std::endl;
}

void BlockInvokeEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    if (desc.size())
        outfile <<"\tdesc = " << desc;

	if (begin)
		outfile << "\tbegin";
	else
		outfile << "\tend";

    EventBase *peer = get_event_peer();
    if (peer) {
        if (begin) 
            outfile << "\t(Enqueued)0x" << std::hex << peer->get_tid()\
                << " at " << std::fixed << std::setprecision(1) << peer->get_abstime();
        else
            outfile << "\t(Invoked)0x" << std::hex << peer->get_tid()\
				<<" at" << std::fixed << std::setprecision(1) << peer->get_abstime();
    }
    outfile << std::endl;
}
