#ifndef MACHMSG_HPP
#define MACHMSG_HPP

#include "base.hpp"
#include "backtraceinfo.hpp"
#include "voucher.hpp"
#include "loader.hpp"

#define MACH_MSG_KALLOC_COPY_T    4
#define MACH_MSGH_BITS_ZERO        0x00000000
#define MACH_MSGH_BITS_REMOTE_MASK 0x0000001f
#define MACH_MSGH_BITS_LOCAL_MASK  0x00001f00
#define MACH_MSGH_BITS_VOUCHER_MASK     0x001f0000
#define    MACH_MSGH_BITS_PORTS_MASK        \
        (MACH_MSGH_BITS_REMOTE_MASK |    \
         MACH_MSGH_BITS_LOCAL_MASK |    \
         MACH_MSGH_BITS_VOUCHER_MASK)
#define MACH_MSGH_BITS_COMPLEX        0x80000000U    /* message is complex */
#define MACH_MSGH_BITS_USER             0x801f1f1fU    /* allowed bits user->kernel */
#define    MACH_MSGH_BITS_RAISEIMP        0x20000000U    /* importance raised due to msg */
#define MACH_MSGH_BITS_DENAP        MACH_MSGH_BITS_RAISEIMP
#define    MACH_MSGH_BITS_IMPHOLDASRT    0x10000000U    /* assertion help, userland private */
#define MACH_MSGH_BITS_DENAPHOLDASRT    MACH_MSGH_BITS_IMPHOLDASRT
#define    MACH_MSGH_BITS_CIRCULAR        0x10000000U    /* message circular, kernel private */
#define    MACH_MSGH_BITS_USED        0xb01f1f1fU


#define MACH_MSGH_BITS_REMOTE(bits) \
    ((bits) & MACH_MSGH_BITS_REMOTE_MASK)

#define MACH_MSGH_BITS_LOCAL(bits) \
    (((bits) & MACH_MSGH_BITS_LOCAL_MASK) >> 8)

#define MACH_SEND_MSG    0x00000001
#define MACH_RCV_MSG     0x00000002

#define NAME_IDX(name) name // ((name) >> 8)

#if !defined(__APPLE__)
#define	MACH_MSGH_BITS_OTHER(bits)				\
		((bits) &~ MACH_MSGH_BITS_PORTS_MASK)
#define MACH_MSG_TYPE_MOVE_RECEIVE	16	/* Must hold receive rights */
#define MACH_MSG_TYPE_MOVE_SEND		17	/* Must hold send rights */
#define MACH_MSG_TYPE_MOVE_SEND_ONCE	18	/* Must hold sendonce rights */
#define MACH_MSG_TYPE_COPY_SEND		19	/* Must hold send rights */
#define MACH_MSG_TYPE_MAKE_SEND		20	/* Must hold receive rights */
#define MACH_MSG_TYPE_MAKE_SEND_ONCE	21	/* Must hold receive rights */
#define MACH_MSG_TYPE_COPY_RECEIVE	22	/* Must hold receive rights */
#define MACH_MSG_TYPE_PORT_NAME		15
typedef unsigned int  mach_msg_bits_t;
typedef unsigned int mach_msg_copy_options_t;
typedef unsigned int mach_msg_type_name_t;
#endif

class MsgHeader {
private:
    uint64_t remote_port;
    uint64_t local_port;
    uint32_t rport_name;
    uint32_t lport_name;
    uint64_t carried_voucher_port;
    uint64_t thread_voucher_port;
    uint64_t msgh_id;
    uint64_t msgh_bits;
    bool recv;
    bool mig;
    bool from_kernel;
private:
    const char *msgh_bit_decode64(mach_msg_bits_t bit);
    const char *ipc_type_name64(mach_msg_bits_t bit);
    void decode_remote_port(std::ofstream &outfile);
    void decode_local_port(std::ofstream &outfile);
    void decode_voucher_port(std::ofstream &outfile);
public:
    MsgHeader();
    void set_port_names(uint32_t _rport_name, uint32_t _lport_name) {
        rport_name = _rport_name;
        lport_name = _lport_name;
    }

    void set_remote_local_ports(uint64_t _remote_port, uint64_t _local_port) {
        remote_port = _remote_port;
        local_port = _local_port;
        if (recv == false && local_port == 0 && mig == true) {
            mig = false;
        }
    }

    void set_carried_vport(uint64_t _carried_voucher_port) {
        carried_voucher_port = _carried_voucher_port;
    }
    void set_thread_vport(uint64_t _thread_voucher_port) {
        thread_voucher_port = _thread_voucher_port;
    }
    bool set_msgh_id(uint64_t _msgh_id, MsgEvent * prev_msg);
    void set_msgh_bits(uint64_t _msgh_bits) {msgh_bits = _msgh_bits;}
    void set_recv(void) {recv = true;}
    uint64_t get_remote_port(void) {return remote_port;}
    uint64_t get_local_port(void) {return local_port;}
    uint32_t get_rport_name(void) {return rport_name;}
    uint32_t get_lport_name(void) {return lport_name;}
    uint64_t get_carried_vport(void) {return carried_voucher_port;}
    uint64_t get_thread_vport(void) {return thread_voucher_port;}
    uint64_t get_msgh_id(void) {return msgh_id;}
    uint64_t get_msgh_bits(void) {return msgh_bits;}
    bool check_recv(void) {return recv;}
    bool is_mig(void) {return mig;}
    bool is_from_kernel(void) {return from_kernel;}
    void decode_header(std::ofstream &outfile);
};

class MsgEvent : public EventBase {
private:
    MsgHeader *header;
    MsgEvent *peer;
    MsgEvent *next;
    MsgEvent *prev;
    BacktraceEvent *bt;
    VoucherEvent *voucher;
    uint64_t tag;
    uint64_t user_addr;
    bool free_before_deliver;

private:
    const char* mm_copy_options_string64(mach_msg_copy_options_t option);

public:
    MsgEvent(double _timestamp, std::string _op, tid_t _tid, MsgHeader *_header, uint32_t _core_id, std::string _procname = "");
    ~MsgEvent(void);
    MsgHeader *get_header(void) {return header;}
    void set_freed(void) {free_before_deliver = true;}
    bool is_freed_before_deliver(void) {return free_before_deliver;}
    void set_tag(uint64_t _tag) {tag = _tag;}
    void set_user_addr(uint64_t _user_addr) {user_addr = _user_addr;}
    void set_peer(MsgEvent *_peer) {peer = _peer; set_event_peer(peer);}
    void set_next(MsgEvent *_next) {next = _next; _next->set_prev(this);}
    void set_prev(MsgEvent *_prev) {prev = _prev;}
    void set_bt(BacktraceEvent *_bt) {bt = _bt;}
    void set_voucher(VoucherEvent * _voucher) {voucher = _voucher;}
    MsgEvent* get_peer(void) {return peer;}
    MsgEvent* get_next(void) {return next;}
    MsgEvent* get_prev(void) {return prev;}
    BacktraceEvent* get_bt(void) {return bt;}
    VoucherEvent* get_voucher(void) {return voucher;}
    uint64_t get_tag(void) {return tag;}
    uint64_t get_user_addr(void) {return user_addr;}
    bool is_mig(void) {assert(header); return header->is_mig();}
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
};

#endif
