#ifndef DISPATCH_HPP
#define DISPATCH_HPP
#include "base.hpp"

#define ROOTQ_DRAIN     3
#define ROOTQ_INVOKE    4
#define Q_DRAIN         5
#define MAINQ_CB       21
#define RLQ_PF         23

class BlockInfo;
class FuncInfo;
class BlockEnqueueEvent;
class BlockInvokeEvent;
class DispatchQueueMIGServiceEvent;

class BlockInfo {
protected:
    uint64_t q_id;
    uint64_t item;
	uint64_t dc_flags;
    uint64_t nested_level;
public:
    BlockInfo(uint64_t _q_id, uint64_t _item, uint64_t _dc_flags)
        :q_id(_q_id), item(_item), dc_flags(_dc_flags), nested_level(-1){}
    uint64_t get_qid(void) {return q_id;}
    uint64_t get_item(void) {return item;}
    uint64_t get_flags(void) {return dc_flags;}
    void set_nested_level(uint64_t level) {nested_level = level;}
	uint64_t get_nested_level() {return nested_level;}
};

class FuncInfo {
protected:
    uint64_t ctxt;
    uint64_t func_ptr;
    uint64_t invoke_ptr;
    uint64_t vtable_ptr;
public:
	FuncInfo(uint64_t _ctxt, uint64_t _func_ptr) :ctxt(_ctxt), func_ptr(_func_ptr){invoke_ptr = vtable_ptr = _func_ptr;}
    FuncInfo(uint64_t _ctxt, uint64_t _func_ptr, uint64_t _invoke_ptr, uint64_t _vtable_ptr)
        :ctxt(_ctxt), func_ptr(_func_ptr), invoke_ptr(_invoke_ptr), vtable_ptr(_vtable_ptr){}
    uint64_t get_ctxt(void) {return ctxt;}
    uint64_t get_func(void) {return func_ptr;}
    uint64_t get_func_ptr(void) {return func_ptr;}
    uint64_t get_invoke_ptr(void) {return invoke_ptr;}
    uint64_t get_vtable_ptr(void) {return vtable_ptr;}
    void set_ptrs(uint64_t _func_ptr, uint64_t _invoke_ptr, uint64_t _vtable_ptr) {
        func_ptr = _func_ptr;
        invoke_ptr = _invoke_ptr;
        vtable_ptr = _vtable_ptr;
    }
};

class BlockEnqueueEvent: public EventBase, public BlockInfo, public FuncInfo {
    bool consumed;
public:
	BlockEnqueueEvent(double abstime, std::string op, uint64_t _tid, uint64_t _q_id,
		uint64_t _ctxt, uint64_t func, uint64_t _dc_flags, uint32_t _core, std::string procname = "");
    bool is_consumed(void) {return consumed;}
    void set_consumer(BlockInvokeEvent *_consumer) {
        consumed = true;
        set_event_peer((EventBase *)_consumer);}
    EventBase *get_consumer() {return get_event_peer();}
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};

class BlockInvokeEvent: public EventBase, public BlockInfo, public FuncInfo {
    std::string desc;
    bool begin;
    BacktraceEvent *bt;
public:
    BlockInvokeEvent(double abstime, std::string op, uint64_t _tid, uint64_t _func,
            uint64_t _ctxt, bool _begin, uint32_t _core_id, std::string proc="");
    void set_desc(std::string &_desc) {desc = _desc;}
    std::string get_desc(void) {return desc;}

    bool is_rooted(void) {return get_event_peer() != nullptr;}
    bool is_begin(void) {return begin;}
    void set_root(EventBase *peer) {set_event_peer(peer);}
    EventBase *get_root(void) {return get_event_peer();}
    void set_bt (BacktraceEvent *_bt) {bt = _bt;}
    BacktraceEvent *get_bt(void) {return bt;}
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};

class DispatchQueueMIGServiceEvent: public EventBase {
    BlockInvokeEvent *invoker;
    void *owner;
public:
    DispatchQueueMIGServiceEvent(double abstime, std::string op, uint64_t tid, uint32_t core_id, std::string procname);
    void save_owner(void *_owner);
    void set_mig_invoker(BlockInvokeEvent *_invoker) {invoker = _invoker;}
    BlockInvokeEvent *get_mig_invoker() {return invoker;}
    void *restore_owner(void);
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};
#endif
