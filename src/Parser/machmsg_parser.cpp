#include "parser.hpp"
#include "mach_msg.hpp"

namespace Parse
{
    #define ARGUS_IPC_kmsg_free            0
    #define ARGUS_IPC_msg_trap            1
    #define ARGUS_IPC_msg_link            2
    #define ARGUS_IPC_msg_send            3
    #define ARGUS_IPC_msg_recv            4
    //#define ARGUS_IPC_msg_boarder        5
    //#define ARGUS_IPC_msg_thread_voucher 6
    //#define ARGUS_IPC_msg_dump            7
    //#define ARGUS_IPC_msg_dsc            8
    //#define ARGUS_IPC_msg_idsc            9
    
    MachmsgParser::MachmsgParser(std::string filename)
    :Parser(filename)
    {
        msg_events.clear();
        collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_kmsg_free",
             ARGUS_IPC_kmsg_free));
        collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_msg_trap",
            ARGUS_IPC_msg_trap));
        collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_msg_link",
            ARGUS_IPC_msg_link));
        collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_msg_send",
            ARGUS_IPC_msg_send));
        collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_msg_recv",
            ARGUS_IPC_msg_recv));
        collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_msg_recv_voucher_r",
            ARGUS_IPC_msg_recv));
        //collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_msg_boarder",
        //    ARGUS_IPC_msg_boarder));
        //collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_msg_thread_voucher",
        //    ARGUS_IPC_msg_thread_voucher));
        //collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_msg_dump",
        //    ARGUS_IPC_msg_dump));
        //collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_msg_dsc",
        //    ARGUS_IPC_msg_dsc));
        //collector.insert(std::pair<std::string, uint64_t>("ARGUS_IPC_msg_idsc",
        //    ARGUS_IPC_msg_idsc));
    }

    /* Check if current msg is kern_service recv,
     * if yes, match corresponding send to recv
     */
    MsgEvent *MachmsgParser::check_mig_recv_by_send(MsgEvent *cur_msg,
        uint64_t msgh_id)
    {
        if (cur_msg->get_header()->check_recv() == false
            || (LoadData::mig_dictionary.find(msgh_id - 100) == LoadData::mig_dictionary.end()))
            return nullptr;


        std::list<EventBase *>::reverse_iterator rit = local_event_list.rbegin();
        while(rit != local_event_list.rend() && *rit != cur_msg)
            rit++;
        if (rit == local_event_list.rend())
            return nullptr;
        
        MsgEvent *msg;
        for (++rit; rit != local_event_list.rend(); rit++ ) {
            msg = dynamic_cast<MsgEvent *>(*rit);
            /*
            outfile << "Checking msg " << std::fixed << std::setprecision(1) \
                    <<  msg->get_abstime() << " for " \
                    << std::fixed << std::setprecision(1) << cur_msg->get_abstime() \
                    << std::endl;
            if (msg->is_freed_before_deliver())
                outfile << "is free before_delivery" << std::endl;
            outfile << "msg tid " << std::hex << msg->get_tid() \
                    << "\t cur_msg tid " << std::hex << cur_msg->get_tid() << std::endl;
            */

            if (msg->is_freed_before_deliver() == false 
                && msg->get_header()->get_local_port() \
                == cur_msg->get_header()->get_remote_port()
                && msg->get_tid() == cur_msg->get_tid()) {
                break;
            }
        }

        if (rit == local_event_list.rend()) {
            outfile << "Check mach_msg: No send for recv [list end] "; 
            outfile << std::fixed << std::setprecision(1) << cur_msg->get_abstime();
            outfile << std::endl;
            return nullptr;
        }

        /* this is a recv msg with validate mig number and find the nearest prev msg */
        if (msg->get_header()->is_mig() == false) {
            outfile << "Check mach_msg: No mig send for recv [is_mig] ";
            outfile << "current message is " << std::dec << msgh_id - 100 << "/" << std::hex << msgh_id;
            outfile << "\t" << LoadData::mig_dictionary[msgh_id - 100] << std::endl;;
            outfile << std::fixed << std::setprecision(1) << cur_msg->get_abstime();
            outfile << std::endl;
            return nullptr;
        }

        if (msg->get_header()->check_recv()) {
            outfile << "Check mach_msg : No send for mig recv [recv] ";
            outfile << std::fixed << std::setprecision(1) << cur_msg->get_abstime();
            outfile << std::endl;
            return nullptr;
        }

        return msg;
    }

    bool MachmsgParser::process_kmsg_end(std::string opname, double abstime,
        std::istringstream &iss)
    {
        uint64_t kmsg_addr, user_addr, rcv_or_send, unused, tid, coreid;
        std::string procname;

        if (!(iss >> std::hex >> kmsg_addr >> user_addr >> rcv_or_send >> unused \
            >> tid >> coreid))
            return false;

        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = "";
        
        if (msg_events.find(tid) != msg_events.end()) {
            MsgEvent *cur_msg = msg_events[tid];
            assert(cur_msg);
            MsgHeader *header = cur_msg->get_header();
            assert(header);
            if (header->check_recv() == bool(rcv_or_send - 1)) {
                MsgEvent *mig_send = nullptr;
                /* Check if current msg is kern_service recv,
                 * if yes, match corresponding send to recv
                 */
                uint64_t msgh_id = header->get_msgh_id();
                mig_send = check_mig_recv_by_send(cur_msg, msgh_id);

                if (header->set_msgh_id(msgh_id, mig_send) == false && mig_send) {
                    outfile << "Error: Incorrect mig number, not a recv ";
                    outfile << std::fixed << std::setprecision(1) << abstime << std::endl;
                }
                
                cur_msg->set_user_addr(user_addr);
                cur_msg->set_complete();
                msg_events.erase(tid);
                return true;
            } else {
                outfile << "Error: msg type (send/rcv) does not match" << std::endl;
                return false;
            }
        }
        outfile << "Error: No msg found to collect info from MACH_IPC_trap" << std::endl;
        return false;
    }

    bool MachmsgParser::process_kmsg_set_info(std::string opname, double abstime,
        std::istringstream &iss)
    {
        uint64_t kmsg_addr, portnames, remote_port, local_port, tid, coreid;
        std::string procname;

        if (!(iss >> std::hex >> kmsg_addr >> portnames >> remote_port \
            >> local_port >> tid >> coreid))
            return false;

        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = "";

        if (msg_events.find(tid) != msg_events.end()) {
            msg_events[tid]->get_header()->set_port_names(Parse::hi(portnames),
                Parse::lo(portnames));

            msg_events[tid]->get_header()->set_remote_local_ports(remote_port,
                local_port);
            return true;
        }
        outfile << "Error: No msg found to collect info from MACH_IPC_trap" << std::endl;
        return false;
    }

    MsgEvent *MachmsgParser::new_msg(std::string msg_op_type, uint64_t tag,
        double abstime, uint64_t tid, uint64_t coreid, std::string procname)
    {
        MsgHeader *msgh_info = new MsgHeader();

        if (!msgh_info)
            return nullptr;

        if (msg_op_type.find("recv") != std::string::npos)
            msgh_info->set_recv();
        
        assert(msgh_info);
        MsgEvent *new_msg = new MsgEvent(abstime, msg_op_type, tid, msgh_info,
            (uint32_t)coreid, procname);

        if (!new_msg) {
            delete msgh_info;
            return nullptr;
        }
        new_msg->set_tag(tag);
        msg_events[tid] = new_msg;
        local_event_list.push_back(new_msg);
        return new_msg;
    }

    bool MachmsgParser::process_kmsg_begin(std::string opname, double abstime,
        std::istringstream &iss)
    {
        uint64_t kmsg_addr, msgh_bits, msgh_id, msg_voucher, tid, coreid;
        std::string procname;
        MsgEvent *new_msg_event = nullptr;
        MsgHeader *header = nullptr;
        bool ret = true;

        if (!(iss >> std::hex >> kmsg_addr >> msgh_bits >> msgh_id >> msg_voucher \
            >> tid >> coreid))
            return false;

        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = "";
        
        if (msg_events.find(tid) != msg_events.end()) {
            if (msg_events[tid]->get_tag() == kmsg_addr)
                outfile << "Error: multiple msg operation on one kmsg_addr " \
                        << std::hex << kmsg_addr << std::endl;
            else
                outfile << "Check mach_msg: one thread sends multiple kmsgs concurrently "\
                        << std::hex << msg_events[tid]->get_tag() << "\t"\
                        << std::fixed << std::setprecision(1) << msg_events[tid]->get_abstime() << std::endl;
            ret = false;
        }

        //if (msg_events.find(tid) == msg_events.end()) {
        {
			new_msg_event = new_msg(opname, kmsg_addr, abstime, tid, coreid, procname);

			if (new_msg_event == nullptr) {
                std::cerr << "OOM " << __func__ << std::endl;
                exit(EXIT_FAILURE);
            }

            header = new_msg_event->get_header();
            assert(header != nullptr);
            header->set_msgh_id(msgh_id, nullptr);
            header->set_msgh_bits(msgh_bits);
            header->set_carried_vport(msg_voucher);
        } 

        return ret;
    }
    
    bool MachmsgParser::process_kmsg_free(std::string opname, double abstime,
        std::istringstream &iss)
    {
        uint64_t kmsg_addr, unused, tid, coreid;
        std::string procname;
        MsgEvent *msg_event = nullptr;

        if (!(iss >> std::hex >> kmsg_addr >> unused >> unused >> unused \
            >> tid >> coreid))
            return false;

        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = "";
        
        std::list<EventBase*>::reverse_iterator rit;
        for (rit = local_event_list.rbegin(); rit != local_event_list.rend();
            rit++) {
            msg_event = dynamic_cast<MsgEvent *>(*rit);
            if (msg_event->get_tag() == kmsg_addr)
                break;
        }
        
        if (rit != local_event_list.rend()) {
            if (msg_events.find(msg_event->get_tid()) != msg_events.end()
                && msg_events[msg_event->get_tid()] == msg_event) {
                outfile << "Check mach_msg: msg interrupted " \
                        << std::fixed << std::setprecision(1) \
                        << msg_event->get_abstime() << std::endl;

                msg_events.erase(msg_event->get_tid());
                local_event_list.remove(msg_event);
                delete(msg_event);
                return false;
            } 
            /* if send -> freed then, the msg is freed before delivered
             * TODO : check if msg freed in the middle of recv,
             *        then sender is not able to find peer
             *        or receiver will retry
             * observed case : a send and wake up b, b wake up c, c free msg
             */
            if ((msg_event->get_header()->check_recv() == false )
                && (msg_event->get_header()->is_mig() == false)) {
                outfile << "Check mach_msg: msg free after send " \
                        << std::fixed << std::setprecision(1) \
                        << msg_event->get_abstime() << std::endl;
                msg_event->set_freed();
                return true;
            }
        }

        return true;
    }

    void MachmsgParser::process()
    {
        std::string line, deltatime, opname;
        double abstime;
        bool ret = false;

		LOG_S(INFO) << "Parse mach msg event...";
        while(getline(infile, line)) {
            std::istringstream iss(line);

            if (!(iss >> abstime >> deltatime >> opname)) {
                outfile << line << std::endl;
                continue;
            }

            switch  (collector[opname]) {
                case ARGUS_IPC_kmsg_free:
                    ret = process_kmsg_free(opname, abstime, iss);
                    break;
                case ARGUS_IPC_msg_recv:
                    opname = "ARGUS_IPC_msg_recv";
                case ARGUS_IPC_msg_send:
                    ret = process_kmsg_begin(opname, abstime, iss);
                    break;
                case ARGUS_IPC_msg_trap:
                    ret = process_kmsg_set_info(opname, abstime, iss);
                    break;
                case ARGUS_IPC_msg_link:
                    ret = process_kmsg_end(opname, abstime, iss);
                    break;
                default:
                    outfile << "Error: unknown opname " << std::endl;
                    ret = false;
            }

            if (ret == false) {
                outfile << line << std::endl;
            }
        }
		LOG_S(INFO) << "Finish parsing mach msg event";
        //TODO:checking remainding eventstd::map
    }
}
