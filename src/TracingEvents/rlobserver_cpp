#include "rlobserver.hpp"
RunLoopObserverEvent::RunLoopObserverEvent(double timestamp, std::string op, uint64_t tid, uint64_t _rl, uint64_t _mode, uint64_t _stage, uint32_t event_core, std::string procname)
:EventBase(timestamp, RL_OBSERVER_EVENT, op, tid, event_core, procname)
{
    rl = _rl;
    mode = _mode;
    stage = (rl_stage_t)_stage;
}

const char *RunLoopObserverEvent::decode_activity_stage(uint64_t stage)
{
    switch (stage) {
        case kCFRunLoopEntry:
            return "RunLoopEntry";
        case kCFRunLoopExtraEntry:
            return "RunLoopExtraEntry";
        case kCFRunLoopBeforeTimers:
            return "RunLoopBeforeTimers";
        case kCFRunLoopBeforeSources:
            return "RunLoopBeforeSources";
        case kCFRunLoopBeforeWaiting:
            return "RunLoopBeforeWaiting";
        case kCFRunLoopAfterWaiting:
            return "RunLoopAfterWaiting";
        case kCFRunLoopExit:
            return "RunLoopExit";
        case kCFRunLoopAllActivities:
            return "AllActivities_Error";
        default:
            return "Error";
    }
}

void RunLoopObserverEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\t" << decode_activity_stage(stage) << std::endl;
}

void RunLoopObserverEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\t" << decode_activity_stage(stage) << std::endl;
}
