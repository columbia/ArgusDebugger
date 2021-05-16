#include "voucher.hpp"
VoucherDeallocEvent::VoucherDeallocEvent(double timestamp, std::string op, uint64_t tid, uint64_t _voucher, uint32_t coreid, std::string procname)
:EventBase(timestamp, VOUCHER_DEALLOC_EVENT, op, tid, coreid, procname)
{
    voucher = _voucher;
}

void VoucherDeallocEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\n\tvoucher " << std::hex << voucher << std::endl;
}

void VoucherDeallocEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\t" << std::hex << voucher << std::endl;
}
