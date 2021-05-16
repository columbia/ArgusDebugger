#ifndef Interrupt_HPP
#define Interrupt_HPP

#include "base.hpp"

#define LAPIC_DEFAULT_Interrupt_BASE 0xD0
#define LAPIC_REDUCED_Interrupt_BASE 0x50

#define LAPIC_PERFCNT_Interrupt        0xF
#define LAPIC_INTERPROCESSOR_Interrupt    0xE
#define LAPIC_TIMER_Interrupt        0xD
#define LAPIC_THERMAL_Interrupt        0xC
#define LAPIC_ERROR_Interrupt        0xB
#define LAPIC_SPURIOUS_Interrupt    0xA
#define LAPIC_CMCI_Interrupt        0x9
#define LAPIC_PMC_SW_Interrupt        0x8
#define LAPIC_PM_Interrupt            0x7
#define LAPIC_KICK_Interrupt        0x6
#define LAPIC_NMI_Interrupt            0x2

#define AST_PREEMPT     0x01
#define AST_QUANTUM     0x02
#define AST_URGENT      0x04
#define AST_HANDOFF     0x08
#define AST_YIELD       0x10
#define AST_APC         0x20    /* migration APC hook */
#define AST_LEDGER      0x40
#define AST_BSD         0x80
#define AST_KPERF       0x100   /* kernel profiling */
#define AST_MACF        0x200   /* MACF user ret pending */
#define AST_CHUD        0x400 
#define AST_CHUD_URGENT     0x800
#define AST_GUARD       0x1000
#define AST_TELEMETRY_USER  0x2000  /* telemetry sample requested on interrupt from userspace */
#define AST_TELEMETRY_KERNEL    0x4000  /* telemetry sample requested on interrupt from kernel */
#define AST_TELEMETRY_WINDOWED  0x8000  /* telemetry sample meant for the window buffer */
#define AST_SFI         0x10000 /* Evaluate if SFI wait is needed before return to userspace */ 

class SchedInfo {
protected:
    int16_t sched_priority_pre;
    int16_t sched_priority_post;
    uint64_t ast;
    std::vector<tid_t> invoke_threads;

public:
    SchedInfo(){invoke_threads.clear();}

    void add_invoke_thread(tid_t tid) {invoke_threads.push_back(tid);}
    void set_sched_priority_post(int16_t prio) {sched_priority_post = prio;}
    void set_ast(uint64_t _ast) {ast = _ast;}
    int16_t get_sched_priority_pre(void) {return sched_priority_pre;}
    int16_t get_sched_priority_post(void) {return sched_priority_post;}
    std::string ast_desc(uint32_t ast);
    std::string decode_ast(uint64_t ast);
};

class IntrEvent : public EventBase, public SchedInfo {
    uint32_t interrupt_num;
    std::pair<uint64_t, std::string> rip_context;
    uint64_t user_mode;
    IntrEvent *dup;

public:
    IntrEvent(double timestamp, std::string op, uint64_t _tid, uint64_t intr_no, uint64_t rip, uint64_t user_mode,
        uint32_t _coreid, std::string procname = "");
    IntrEvent(IntrEvent * intr);
    ~IntrEvent();

    void add_dup(IntrEvent *_dup) {dup = _dup;}
    void set_context(std::string symbol) {rip_context.second = symbol;}
    uint64_t get_rip(void) {return rip_context.first;}
    uint32_t get_interrupt_num(void) {return interrupt_num;}
    uint64_t get_user_mode(void) {return user_mode;}

    const char *decode_interrupt_num();
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
	std::string get_context() {return rip_context.second;}
};

#endif
