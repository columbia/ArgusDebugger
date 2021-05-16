#include "mach_msg.hpp"

MsgEvent::MsgEvent(double _timestamp, std::string _op, tid_t _tid, MsgHeader *_header, uint32_t _core_id, std::string _procname)
: EventBase(_timestamp, MSG_EVENT, _op, _tid, _core_id, _procname)
{
    assert(_header);
    header = _header;
    prev = peer = next = nullptr;
    bt = nullptr;
    voucher = nullptr;
    free_before_deliver = false;
}

MsgEvent::~MsgEvent()
{
    if (header != nullptr)
        delete(header);
    header = nullptr;
}

void MsgEvent::decode_event(bool is_verbose, std::ofstream &outfile)
{
    EventBase::decode_event(is_verbose, outfile);
    //mach_msg_bits_t other = MACH_MSGH_BITS_OTHER(header->get_msgh_bits()) & MACH_MSGH_BITS_USED;
    outfile << "\t" << std::hex << tag;
    header->decode_header(outfile);
    if (peer)
        outfile << "\n\tpeer " << std::hex << peer->get_tid() << "\t" << std::fixed << std::setprecision(1) << peer->get_abstime();
    if (next)
        outfile << "\n\tnext " << std::hex << next->get_tid() << "\t" <<  std::fixed << std::setprecision(1) << next->get_abstime();

    if (is_freed_before_deliver())
        outfile << "\n\tfreed before deliver";
    

#if defined(__APPLE__)
    if (bt)
        outfile  << "\n\tbacktrace at: " << std::fixed << std::setprecision(2) << bt->get_abstime() << std::endl;
#endif
    if (voucher)
        outfile << "\n\tvoucher at: " << std::fixed << std::setprecision(2) << voucher->get_abstime() << std::endl;

    outfile << "\n\tmsgh_id: " << std::hex << header->get_msgh_id() << std::endl;
    outfile << std::endl;
}

void MsgEvent::streamout_event(std::ofstream &outfile)
{
    EventBase::streamout_event(outfile);
    outfile << "\t" << std::hex << tag;
    if (header->is_mig()) {
        assert(LoadData::mig_dictionary.find(header->get_msgh_id()) != 
                LoadData::mig_dictionary.end());
        outfile << "\tmig_service = " <<  LoadData::mig_dictionary[header->get_msgh_id()].c_str();
    }
    else
        outfile <<"\tremote_port = 0x" << std::hex << (uint32_t)(header->get_remote_port());
    
    outfile << "\tuser_addr 0x" << std::hex << user_addr;

    if (is_freed_before_deliver())
        outfile << "\tfreed before deliver";
    outfile << "\tmsgh_id = " << std::dec << header->get_msgh_id() << std::endl;
}
