#ifndef LOADER_HPP
#define LOADER_HPP
#include "base.hpp"
#include "syscall.hpp"
/* loading necessary system information for systemcall parsing,
 * backtrace parsing and thread matching
 */

/* define codes for parser to divide large log file
 * 0 is reserved
 */
#define MACH_IPC_MSG                1   //MSG_EVENT
#define MACH_IPC_VOUCHER_INFO        2  //VOUCHER_EVENT
#define MACH_MK_RUN                    8 //MR_EVENT
#define INTR                        9   //INTR_EVENT
#define MACH_TS                        11 //TSM_EVENT
#define MACH_WAIT                    12  //WAIT_EVENT
#define DISP_ENQ                    13 //DISP_ENQ_EVENT
#define DISP_EXE                    15 //DISP_INV_EVENT
#define MACH_CALLCREATE                16 //TMCALL_CREATE_EVENT
#define MACH_CALLOUT                17  //TMCALL_CANCEL_EVENT
#define MACH_CALLCANCEL                18 //TMCALL_CALLOUT_EVENT
#define BACKTRACE                    19 //BACKTRACE_EVENT
#define MACH_SYS                    20 //SYSCALL_EVENT
#define BSD_SYS                        21 //SYSCALL_EVENT
#define CA_SET                        22 //CA_SET_EVENT
#define CA_DISPLAY                    23 //CA_DISPLAY_EVENT
#define BREAKPOINT_TRAP                24 //BREAKPOINT_TRAP_EVENT
#define NSAPPEVENT                    27 //NSAPPEVENT_EVENT
#define DISP_MIG                    28 //DISP_MIG_EVENT
#define RL_BOUNDARY                    29 //RL_BOUNDARY_EVENT

#define MAX_ARGC 12

struct syscall_entry {
    int64_t syscall_number;
    const char *syscall_name;
    const char *args[MAX_ARGC];
};

struct mig_service {
    int mig_num;
    const char *mig_name;
};

namespace LoadData
{
    typedef uint64_t tid_t;
    typedef struct Meta{
        std::string data;
        std::string host;
        pid_t pid;
        uint64_t nthreads;

        std::string tpc_maps_file;
        std::string libs_dir;
        std::string procs_file;
        std::string symbolicate_procs_file;
        std::string hwbr_script_file;
        std::string libinfo_file;

        std::string suspicious_api;
        double spin_timestamp;
		double last_event_timestamp;
    } meta_data_t;

    extern meta_data_t meta_data;

    extern std::map<tid_t, ProcessInfo*> tpc_maps;
    extern std::map<int, std::string> mig_dictionary;
    extern const struct mig_service mig_table[];
    extern const uint64_t mig_size;

    extern std::map<std::string, uint64_t> msc_name_index_map;
    extern const struct syscall_entry mach_syscall_table[];
    extern const uint64_t msc_size;

    extern std::map<std::string, uint64_t> bsc_name_index_map;
    extern const struct syscall_entry bsd_syscall_table[];
    extern const uint64_t bsc_size;
    extern const std::map<std::string, uint64_t> op_code_map;
    extern std::map<std::string, int> symbolic_procs; 
    uint64_t map_op_code(uint64_t unused, std::string opname);

    std::string pid2comm(pid_t pid);
	pid_t tid2pid(tid_t tid);
    bool load_lib(std::string procname, std::string lib_info_file);
    bool load_all_libs(std::string lib_info_file);
    void preload();
}
#endif
