#include "parser.hpp"
#include "voucher.hpp"

namespace Parse
{
    VoucherParser::VoucherParser(std::string filename)
    :Parser(filename)
    {}
    
    bool VoucherParser::process_voucher_info(std::string opname, double abstime,
        std::istringstream &iss)
    {
        uint64_t kmsg_addr, _voucher_addr, _bank_attr_type, pids, tid, coreid;
        pid_t holder_pid, merchant_pid;
        std::string procname = "";

        if (!(iss >> std::hex >> kmsg_addr >> _voucher_addr >> _bank_attr_type \
            >> pids >> tid >> coreid))
            return false;

        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = "";

        VoucherEvent *new_voucher_info = new VoucherEvent(abstime - 0.05,
            opname, tid, kmsg_addr, _voucher_addr,
            _bank_attr_type, coreid, procname);

        if (!new_voucher_info) {
            std::cerr << "OOM " << __func__ << std::endl;
            exit(EXIT_FAILURE);    
        }

        holder_pid = (pid_t)(Parse::hi(pids));
        merchant_pid = (pid_t)(Parse::lo(pids));

        if (holder_pid != 0)
            new_voucher_info->set_bank_holder(holder_pid);

        if (merchant_pid != 0)
            new_voucher_info->set_bank_merchant(merchant_pid);
        
        local_event_list.push_back(new_voucher_info);
        new_voucher_info->set_complete();
        return true;
    }
/*  
    bool VoucherParser::process_voucher_conn(std::string opname, double abstime,
        std::istringstream &iss)
    {
        uint64_t voucher_ori, voucher_new, unused, tid, coreid;
        std::string procname = "";
        if (!(iss >> std::hex >> voucher_ori >> voucher_new >> unused >> unused \
            >> tid >> coreid)) {
            return false;
        }

        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = "";

        VoucherConnectEvent  *new_voucher_conn = new VoucherConnectEvent(abstime,
            opname, tid, voucher_ori, voucher_new, coreid, procname);
        if (!new_voucher_conn) {
            std::cerr << "OOM " << __func__ << std::endl;
            exit(EXIT_FAILURE);
        }

        local_event_list.push_back(new_voucher_conn);
        new_voucher_conn->set_complete();
        return true;
    }

    bool VoucherParser::process_voucher_transit(std::string opname, double abstime,
        std::istringstream &iss)
    {
        uint64_t voucher_dst, voucher_src, unused, tid, coreid;
        std::string procname = "";

        if (!(iss >> std::hex >> voucher_dst >> voucher_src >> unused >> unused \
            >> tid >> coreid)) {
            return false;
        }

        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = "";

        VoucherTransitEvent *new_voucher_transit = new VoucherTransitEvent(
            abstime, opname, tid, voucher_dst, voucher_src, coreid, procname);
        if (!new_voucher_transit) {
            std::cerr << "OOM " << __func__ << std::endl;
            exit(EXIT_FAILURE);
        }

        local_event_list.push_back(new_voucher_transit);
        new_voucher_transit->set_complete();
        return true;
    }

    bool VoucherParser::process_voucher_deallocate(std::string opname,
        double abstime, std::istringstream &iss)
    {
        uint64_t voucher, unused, tid, coreid;
        std::string procname = "";
        if (!(iss >> std::hex >> voucher >> unused >> unused >> unused \
            >> tid >> coreid)) {
            return false;
        }

        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = "";

        VoucherDeallocEvent *dealloc_voucher_event = new VoucherDeallocEvent(
            abstime, opname, tid, voucher, coreid, procname);
        if (!dealloc_voucher_event) {
            std::cerr << "OOM " << __func__ << std::endl;
            exit(EXIT_FAILURE);
        }

        local_event_list.push_back(dealloc_voucher_event);
        dealloc_voucher_event->set_complete();
        return true;
    }

    bool VoucherParser::process_bank_account(std::string opname, double abstime,
        std::istringstream &iss)
    {
        uint64_t merchant, holder, unused, tid, coreid;
        std::string procname = "";
        if (!(iss >> std::hex >> merchant >> holder >> unused >> unused \
            >> tid >> coreid)) {
            return false;
        }

        if (!getline(iss >> std::ws, procname) || !procname.size())
            procname = "";

        BankEvent *new_bank_event = new BankEvent(abstime, opname, tid,
            merchant, holder, coreid, procname);
        if (!new_bank_event) {
            std::cerr << "OOM " << __func__ << std::endl;
            exit(EXIT_FAILURE);
        }

        local_event_list.push_back(new_bank_event);
        new_bank_event->set_complete();
        return true;
    }
*/

    /* MACH_IPC_MSG carries voucher(VoucherEvent)
     * Processing Command on voucher(VoucherConnectEvent)
     * Resule of Command is to create/reuse voucher (VoucherTransitEvent)
     * End of a voucher (voucher_free_ev_t)
     */
    void VoucherParser::process(void)
    {
        std::string line, deltatime, opname;
        double abstime;
        bool ret = false;

        while(getline(infile, line)) {
            std::istringstream iss(line);
            if (!(iss >> abstime >> deltatime >> opname)){
                outfile << line << std::endl;
                continue;
            }
            switch (LoadData::map_op_code(0, opname)) {
                case MACH_IPC_VOUCHER_INFO:
                    ret = process_voucher_info(opname, abstime, iss);
                    break;
				/*
                case MACH_IPC_VOUCHER_CONN:
                    ret = process_voucher_conn(opname, abstime, iss);
                    break;
                case MACH_IPC_VOUCHER_TRANSIT:
                    ret = process_voucher_transit(opname, abstime, iss);
                    break;
                case MACH_IPC_VOUCHER_DEALLOC:
                    ret = process_voucher_deallocate(opname, abstime, iss);
                    break;
                case MACH_BANK_ACCOUNT:
                    ret = process_bank_account(opname, abstime, iss);
                    break;
				*/
                default:
                    ret = false;
                    std::cerr << "unknown op code" << std::endl;
            }
            if (ret == false)
                outfile << line << std::endl;            
        }
    }
}
