#include "mach_msg.hpp"
#include "dispatch.hpp"
#include "backtraceinfo.hpp"

#define DEBUG_BT 0

static std::map<std::string, uint64_t> symbol_freq;

BacktraceEvent::BacktraceEvent(double abstime, std::string _op, uint64_t _tid, 
    uint64_t _tag, uint64_t _max_frames, uint32_t _core_id, std::string _procname)
:EventBase(abstime, BACKTRACE_EVENT, _op, _tid, _core_id, _procname),
Frames(_tag, _max_frames, _tid)
{
	symbolized = false;
    host_event = nullptr;
}

BacktraceEvent::~BacktraceEvent(void)
{
}

void BacktraceEvent::add_frames(uint64_t *frames, int size)
{
    for (int i = 0; i < size; i++)
        add_frame(frames[i]);
	assert(frames_info[frames_info.size()-1].addr != 0);
}

bool BacktraceEvent::hook_to_event(EventBase *event, event_type_t event_type) 
{
    bool ret = false;
    switch (event_type) {
        case MSG_EVENT: {
            MsgEvent *msg_event = dynamic_cast<MsgEvent*>(event);
            if (msg_event && host_event_tag == msg_event->get_user_addr()) {
                host_event = event;
                msg_event->set_bt(this);
#if DEBUG_BT
                LOG_S(INFO) << std::fixed << std::setprecision(1) << get_abstime() << " hooked to msg "\
                    std::fixed << std::setprecision(1) << host_event->get_abstime();
#endif
                ret = true;
            }
            break;
        }
        case DISP_INV_EVENT: {
            BlockInvokeEvent *blockinvoke_event = dynamic_cast<BlockInvokeEvent *>(event);
            if (blockinvoke_event && host_event_tag == blockinvoke_event->get_func()) {
                host_event = event;
                blockinvoke_event->set_bt(this);
#if DEBUG_BT
                LOG_S(INFO) << std::fixed << std::setprecision(1) << get_abstime() << " hooked to dispatch_block "\
                    << std::fixed << std::setprecision(1) << host_event->get_abstime();
#endif
                ret = true;
            }
            break;
        }
        default:
            ret = false;
    }

    return ret;
}

//count symbol when parsing
void BacktraceEvent::count_symbols()
{
	for(auto frame: frames_info) {
		if (symbol_freq.find(frame.symbol) == symbol_freq.end())
			symbol_freq[frame.symbol] = 1;
		else
			symbol_freq[frame.symbol]++;
	}
}

//update the freq of sysmbols based on the whole log
std::list<Frames::frame_info_t> BacktraceEvent::sort_symbols()
{
	std::list<Frames::frame_info_t> frames;
	frames.clear();

	for (int i = 0; i < frames_info.size(); i++) {
		if (symbol_freq.find(frames_info[i].symbol) != symbol_freq.end()) {
			frames_info[i].freq = symbol_freq[frames_info[i].symbol];
		}
		frames.push_back(frames_info[i]);
	}
	frames.sort(Frames::compare_freq);
	return frames;
}

std::list<uint64_t> BacktraceEvent::get_frames()
{
	std::list<uint64_t> ret;
	ret.clear();
	for (auto frame: frames_info)
		ret.push_back(frame.addr);
	return ret;
}

void BacktraceEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);

#if DEBUG_BT
    if (host_event)
        outfile << "\thooked to " << std::fixed << std::setprecision(1) << host_event->get_abstime() << std::endl;
    else
        outfile << "\tnot hooked" << std::endl;
#endif

    outfile << "#frames = " << std::dec << max_frames << std::endl;
	if (symbolized)
    	decode_frames(outfile);
}

void BacktraceEvent::streamout_event(std::ostream &out) 
{
    EventBase::streamout_event(out);
	out<<std::endl;
	if (symbolized)
    	decode_frames(out);
}

void BacktraceEvent::streamout_event(std::ofstream &outfile) 
{
    EventBase::streamout_event(outfile);
#if DEBUG_BT
    if (host_event)
        outfile << "\\n\thooked to " << std::fixed << std::setprecision(1) << host_event->get_abstime() << std::endl;
    else
        outfile << "\\n\tno hooked" << std::endl;
#endif
	if (symbolized)
    	decode_frames(outfile);
}

void BacktraceEvent::log_event()
{
    if (frames_info.size() > 4)
	    LOG_S(INFO) << "Callstack:";
	for (int index = 0; index < frames_info.size(); index++) {
		if (frames_info[index].symbol.size() > 0)
			LOG_S(INFO) << "[" << index << "]\t" <<  frames_info[index].symbol;
	}
}
