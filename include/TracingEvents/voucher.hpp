#ifndef VOUCHER_HPP
#define VOUCHER_HPP
#include "base.hpp"

#define BANK_TASK    0
#define BANK_ACCT    1

class BankEvent: public EventBase {
    pid_t bank_holder;
    pid_t bank_merchant;
    std::string bank_holder_name;
    std::string bank_merchant_name;
public:
    BankEvent(double timestamp, std::string op, uint64_t tid, uint64_t merchant, uint64_t holder, uint32_t coreid, std::string procname = "");
    pid_t get_bank_holder(void) { return bank_holder;}
    void set_bank_holder_name(std::string name) { bank_holder_name = name;}
    pid_t get_bank_merchant(void) { return bank_merchant;}
    void set_bank_merchant_name(std::string name) {bank_merchant_name = name;}
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};

class VoucherEvent: public EventBase {
    MsgEvent* carrier;
    uint32_t bank_attr_type;
    uint64_t carrier_addr;
    uint64_t voucher_addr;
    pid_t bank_holder;
    pid_t bank_merchant;
    pid_t bank_orig;
    pid_t bank_prox;
    std::string bank_holder_name;
    std::string bank_merchant_name;
    std::string bank_orig_name;
    std::string bank_prox_name;
public:
    VoucherEvent(double timestamp, std::string op, uint64_t tid, uint64_t kmsg_addr, uint64_t _voucher_addr, uint64_t _bank_attr_type, uint32_t coreid, std::string procname ="");
    void set_carrier(MsgEvent* cur_msg) {carrier = cur_msg;}
    MsgEvent * get_carrier(void) {return carrier;}
    uint64_t get_carrier_addr(void) {return carrier_addr;}
    uint64_t get_voucher_addr(void) {return voucher_addr;}

    void set_bank_holder(pid_t pid) {bank_holder = pid;}
    pid_t get_bank_holder(void) { return bank_holder;}
    void set_bank_holder_name(std::string name) {bank_holder_name = name;}
    std::string get_bank_holder_name(void) {return bank_holder_name;}

    void set_bank_merchant(pid_t pid) {bank_merchant = pid;}
    pid_t get_bank_merchant(void) { return bank_merchant;}
    void set_bank_merchant_name(std::string name) {bank_merchant_name = name;}
    std::string get_bank_merchant_name(void) {return bank_merchant_name;}

    pid_t get_bank_orig(void) {return bank_orig;}
    void set_bank_orig_name(std::string name) {bank_orig_name = name;}
    std::string get_bank_orig_name(void) {return bank_orig_name;}

    pid_t get_bank_prox(void) {return bank_prox;}
    void set_bank_prox_name(std::string name) {bank_prox_name = name;}
    std::string get_bank_prox_name(void) {return bank_prox_name;}

    bool hook_msg(MsgEvent * msg_event);
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};
#endif
