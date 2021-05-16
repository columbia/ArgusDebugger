#include "voucher.hpp"
#include "mach_msg.hpp"

VoucherEvent::VoucherEvent(double timestamp, std::string op, uint64_t tid, uint64_t kmsg_addr, uint64_t _voucher_addr, uint64_t _bank_attr_type, uint32_t coreid, std::string procname)
:EventBase(timestamp, VOUCHER_EVENT, op, tid, coreid, procname)
{
    if (_bank_attr_type == BANK_TASK) {
        bank_attr_type = (uint32_t)_bank_attr_type;
        bank_orig = bank_prox = -1;
    } else {
        bank_orig = pid_t(_bank_attr_type >> 32);
        bank_orig = bank_orig == 0 ? -1 : bank_orig;
        bank_prox = pid_t(_bank_attr_type);
        bank_prox = bank_prox == 0 ? -1 : bank_prox;
        bank_attr_type = BANK_ACCT;
    }
    bank_holder = bank_merchant = -1;

    carrier = nullptr;
    carrier_addr = kmsg_addr;
    voucher_addr = _voucher_addr;

    bank_holder_name = "-";
    bank_merchant_name = "-";
    bank_orig_name = "-";
    bank_prox_name = "-";
}

bool VoucherEvent::hook_msg(MsgEvent * msg_event)
{
    if (msg_event->get_header()->get_carried_vport() == voucher_addr
        && msg_event->get_tag() == carrier_addr) {
        carrier = msg_event;
        msg_event->set_voucher(this);
        return true;
    }
    return false;
}

void VoucherEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    if (bank_attr_type == BANK_TASK)
        outfile << "\n\tbank_context";
    else if (bank_attr_type == BANK_ACCT)
        outfile << "\n\tbank_account";
    else
        outfile << "\n\tbank_unknown";

    if (bank_holder != -1) 
        outfile << "\n\tbank_holder : " << bank_holder_name;
    if (bank_merchant != -1)
        outfile << "\n\tbank_merchant : " << bank_merchant_name;
    if (bank_orig != -1)
        outfile << "\n\tbank_originator : " << bank_orig_name;
    if (bank_prox != -1)
        outfile << "\n\tbank_proximate : " << bank_prox_name;

    outfile << std::endl;
}

void VoucherEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    /*
    if (bank_attr_type == BANK_TASK)
        outfile << "\tbank_context(";
    else if (bank_attr_type == BANK_ACCT)
        outfile << "\tbank_account(";
    */

    outfile << "\t(voucher_" << std::hex << voucher_addr << "_msg_" << std::hex << carrier_addr << ")";
    if (bank_holder != -1) 
        outfile << "\t" << bank_holder_name;
    if (bank_merchant != -1)
        outfile << "\t" << bank_merchant_name;
    if (bank_orig != -1)
        outfile << "\t" << bank_orig_name;
    if (bank_prox != -1)
        outfile << "\t" << bank_prox_name;
    outfile << std::endl;
}
