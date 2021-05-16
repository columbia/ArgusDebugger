#include "interrupt.hpp"
std::string SchedInfo::ast_desc(uint32_t ast)
{
    std::string desc;
    if (ast & AST_PREEMPT)
        desc.append("preempt ");
    if (ast & AST_QUANTUM)
        desc.append("quantum ");
    if (ast & AST_URGENT)
        desc.append("urget ");
    if (ast & AST_HANDOFF)
        desc.append("handoff ");
    if (ast & AST_YIELD)
        desc.append("yield ");
    if (ast & AST_APC)
        desc.append("apc ");
    if (ast & AST_LEDGER)
        desc.append("ledger ");
    if (ast & AST_BSD)
        desc.append("bsd ");
    if (ast & AST_KPERF)
        desc.append("kperf ");
    if (ast & AST_MACF)
        desc.append("macf ");
    if (ast & AST_CHUD)
        desc.append("chud ");
    if (ast & AST_CHUD_URGENT)
        desc.append("ast_chud_urgent ");
    if (ast & AST_GUARD)
        desc.append("ast_guard ");
    if (ast & AST_TELEMETRY_USER)
        desc.append("ast_telmetry_user ");
    if (ast & AST_TELEMETRY_KERNEL)
        desc.append("ast_telemetry_kern ");
    if (ast & AST_TELEMETRY_WINDOWED)
        desc.append("ast_telemetry_windowed ");
    if (ast & AST_SFI)
        desc.append("ast_sfi ");
    if (!desc.size())
        desc = "ast_none";
    return desc;
}

std::string SchedInfo::decode_ast(uint64_t ast) {
    uint32_t thr_ast = uint32_t(ast >> 32);
    uint32_t thr_reason = uint32_t(ast);
    return "thr_ast: " + ast_desc(thr_ast) + "\tblock_reason: " + ast_desc(thr_reason);
}

IntrEvent::IntrEvent(double timestamp, std::string op, uint64_t _tid, uint64_t intr_no, uint64_t _rip, uint64_t _user_mode, uint32_t _core_id, std::string procname)
:EventBase(timestamp, INTR_EVENT, op, _tid, _core_id, procname)
{
    dup = nullptr;
    rip_context.first = _rip;
    rip_context.second.clear();
    user_mode = _user_mode;
    interrupt_num = (uint32_t)intr_no;
    sched_priority_pre = (int16_t)(intr_no >> 32);
}

IntrEvent::IntrEvent(IntrEvent * intr)
:EventBase(intr)
{
    *this = *intr;
    dup = nullptr;
}

IntrEvent::~IntrEvent()
{
    if (!dup)
        delete(dup);
    dup = nullptr;
}

//const char * IntrEvent::decode_interrupt_num(uint64_t interrupt_num)
const char *IntrEvent::decode_interrupt_num()
{
    int n = interrupt_num - LAPIC_DEFAULT_Interrupt_BASE;
    switch(n) {
        case LAPIC_PERFCNT_Interrupt:
            return "PERFCNT";
        case LAPIC_INTERPROCESSOR_Interrupt:
            return "IPI";
        case LAPIC_TIMER_Interrupt:
            return "TIMER";
        case LAPIC_THERMAL_Interrupt:
            return "THERMAL";
        case LAPIC_ERROR_Interrupt:
            return "ERROR";
        case LAPIC_SPURIOUS_Interrupt:
            return "SPURIOUS";
        case LAPIC_CMCI_Interrupt:
            return "CMCI";
        case LAPIC_PMC_SW_Interrupt:
            return "PMC_SW";
        case LAPIC_PM_Interrupt:
            return "PM";
        case LAPIC_KICK_Interrupt:
            return "KICK";        
        default:
            return "unknown";
    }
}

void IntrEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\n\t - " << std::fixed << std::setprecision(1) << get_finish_time();

    const char * intr_desc = decode_interrupt_num();
    if (strcmp(intr_desc, "unknown"))
        outfile << "\n\t" << intr_desc;
    else
        outfile << "\n\t" << interrupt_num;

    outfile << "\n\t" << "priority: " << std::hex << sched_priority_pre << " -> " << std::hex << sched_priority_post;
    //outfile << "\n\t" << "thread_qos: " << std::hex << get_thep_qos();
    //outfile << "\n\t" << "thread_ast: " << ast_desc(ast >> 32);
    //outfile << "\n\t" << "block_reason: " << ast_desc((uint32_t)ast);

    outfile << "\n\t" << std::hex << rip_context.first << std::endl;
    if (rip_context.second.size())
        outfile << "\n\t" << rip_context.second;
    outfile << std::endl;
}

void IntrEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    const char * intr_desc = decode_interrupt_num();
    if (strcmp(intr_desc, "unknown"))
        outfile << "\t" << intr_desc << "_INTR";
    else
        outfile << "\t" << interrupt_num << "_INTR";

    outfile << "\t - " << std::fixed << std::setprecision(1) << get_finish_time();

    if (rip_context.second.size())
        outfile << "\t" << rip_context.second;
    outfile << std::endl;
}
