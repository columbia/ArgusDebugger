#include "voucher.hpp"
VoucherTransitEvent::VoucherTransitEvent(double timestamp, std::string op, uint64_t tid, uint64_t _voucher_dst, uint64_t _voucher_src, uint32_t coreid, std::string procname)
:EventBase(timestamp, VOUCHER_TRANS_EVENT, op, tid, coreid, procname)
{
    voucher_dst = _voucher_dst;
    if (op.find("reuse") != std::string::npos) {
        voucher_src = _voucher_src;
        type = REUSE;
    } else {
        voucher_src = 0;
        type = CREATE;
    }
}

void VoucherTransitEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\tvoucher dst " << std::hex << voucher_dst << std::endl;
}

void VoucherTransitEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\tvoucher dst " << std::hex << voucher_dst << std::endl;
}
