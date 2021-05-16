#include "voucher.hpp"
VoucherConnectEvent::VoucherConnectEvent(double timestamp, std::string op, uint64_t tid, uint64_t _voucher_ori, uint64_t _voucher_new, uint32_t coreid, std::string procname)
:EventBase(timestamp, VOUCHER_CONN_EVENT, op, tid, coreid, procname)
{
    voucher_ori = _voucher_ori;
    voucher_new = _voucher_new;
}

void VoucherConnectEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    outfile << "\n\tfrom_" << std::hex << voucher_ori;
    outfile << "\tto_" << std::hex << voucher_new << std::endl; 
    
}
void VoucherConnectEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\tfrom_" << std::hex << voucher_ori << "_to_" << std::hex << voucher_new << std::endl; 
}
