#include "mach_msg.hpp"
MsgHeader::MsgHeader()
{
    remote_port = 0;
    local_port = 0;
    rport_name = 0;
    lport_name = 0;
    carried_voucher_port = 0;
    thread_voucher_port = 0;
    msgh_id = 0;
    msgh_bits = 0;
    recv = false;
    mig = false;
    from_kernel = false;
}

bool MsgHeader::set_msgh_id(uint64_t _msgh_id, MsgEvent * prev_msg)
{
    bool is_mig = _msgh_id >> 60;
    _msgh_id  = (_msgh_id << 4) >> 4;
    bool is_from_kernel = _msgh_id >> 58; 
    from_kernel = is_from_kernel;
    _msgh_id  = (_msgh_id << 6) >> 6;

    if (recv) {
        if (prev_msg && prev_msg->get_header()->get_msgh_id() == _msgh_id - 100) {
            msgh_id = _msgh_id - 100;
            mig = true;
        } else 
            msgh_id = _msgh_id;
    }
    else {
        msgh_id = _msgh_id;
        if (is_mig && LoadData::mig_dictionary.find(msgh_id) != LoadData::mig_dictionary.end())
            mig = true;
        /* may not a mig
         * leave the verification to the sender
         * by checking local port
          * if local_port == 0 then is not mig
          */
    }
    return mig;
}

const char * MsgHeader::msgh_bit_decode64(mach_msg_bits_t bit)
{
    switch (bit) {
        case MACH_MSGH_BITS_COMPLEX: 
            return "complex";
        case MACH_MSGH_BITS_CIRCULAR:
            return "circular";
        case MACH_MSGH_BITS_RAISEIMP:
            return "raise_imp";        
        default:
            return "unknown";
    }
}

const char * MsgHeader::ipc_type_name64(mach_msg_bits_t type_name)
{
    switch (type_name) {
        case MACH_MSG_TYPE_PORT_NAME:
            return "port_name";
        
        case MACH_MSG_TYPE_MOVE_RECEIVE:
        if (recv) {
            return "port_receive";
        } else {
            return "move_receive";
        }
        
        case MACH_MSG_TYPE_MOVE_SEND:
        if (recv) {
            return "port_send";
        } else {
            return "move_send";
        }
        
        case MACH_MSG_TYPE_MOVE_SEND_ONCE:
        if (recv) {
            return "port_send_once";
        } else {
            return "move_send_once";
        }
        
        case MACH_MSG_TYPE_COPY_SEND:
            return "copy_send";
        
        case MACH_MSG_TYPE_MAKE_SEND:
            return "make_send";
        
        case MACH_MSG_TYPE_MAKE_SEND_ONCE:
            return "make_send_once";
        
        default:
            return "unknown";
    }
}

void MsgHeader::decode_remote_port(std::ofstream &outfile)
{
    if (remote_port) {
        mach_msg_bits_t rbit = MACH_MSGH_BITS_REMOTE(msgh_bits);
        const char * port_name = ipc_type_name64(rbit);
        outfile << "remote = " << std::hex << rport_name;
        outfile << "(0x" << std::hex << remote_port << ")";
        if (port_name)
            outfile << " (" << port_name << ")\n";
        else 
            outfile << " (type 0x" << std::hex << rbit << ")\n";
    } else {
        outfile << "remote = null\n";
    }
}

void MsgHeader::decode_local_port(std::ofstream &outfile)
{
    if (local_port) {
        mach_msg_bits_t lbit = MACH_MSGH_BITS_LOCAL(msgh_bits);
        const char * port_name = ipc_type_name64(lbit);
        outfile << "\t      \t\tlocal = " << std::hex << lport_name;
        outfile << "(0x" << std::hex << local_port << ")";
        if (port_name)
            outfile << " (" << port_name << ")\n";
        else
            outfile << " (type 0x" << std::hex << lbit << ")\n";
    } else {
        outfile << "\t      \t\tlocal = null\n";
    }
}

void MsgHeader::decode_voucher_port(std::ofstream &outfile)
{
    if (carried_voucher_port) {
        outfile << "\t      \t\tvoucher = " << std::hex << carried_voucher_port << std::endl;
    } else {
        outfile << "\t      \t\tvoucher = null" << std::endl;
    }
}

void MsgHeader::decode_header(std::ofstream &outfile)
{
    std::string recv_send = recv ? "recv" : "send";
    outfile << "\t" << recv_send << "\t\n";
    outfile <<"\tPORTS:\t\t";
    decode_remote_port(outfile);
    decode_local_port(outfile);
    decode_voucher_port(outfile);

    outfile << "\tOTHER_BITS:\t\t";
    int need_comma = 0;
    mach_msg_bits_t other = MACH_MSGH_BITS_OTHER(msgh_bits) & MACH_MSGH_BITS_USED;
    int i = 0;
    unsigned int bit = 1; 
    for (i = 0, bit = 1; i < sizeof(other) * 8; ++i, bit <<= 1) {
        if ((other & bit) == 0)
            continue;
        const char * bit_name = msgh_bit_decode64((mach_msg_bits_t) bit);
        if (bit_name) {
            if (need_comma)
                outfile << ", " << bit_name;
            else
                outfile << "\t" << bit_name;
        }
        else{
            if (need_comma)
                outfile << ", unknownbit (0x" << std::hex << bit <<")";
            else
                outfile << "\tunknownbit (0x" << std::hex << bit << ")";
        }
        ++need_comma;
    }
    if (msgh_bits & ~MACH_MSGH_BITS_USED) {
        if (need_comma)
            outfile << ", unused = 0x" << std::hex << (msgh_bits & ~MACH_MSGH_BITS_USED) << std::endl;
        else
            outfile << "\tunused = 0x" << std::hex << (msgh_bits & ~MACH_MSGH_BITS_USED) << std::endl;
    }
    if (from_kernel == true)
        outfile << "send from kernel" << std::endl;
    if (mig == true) 
        outfile << "\n\tmig[" << std::dec << msgh_id << "]: " <<::LoadData::mig_dictionary[msgh_id].c_str() << std::endl;
    outfile << std::endl;
}
