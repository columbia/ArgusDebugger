#ifndef BACKTRACEINFO_HPP
#define BACKTRACEINFO_HPP

#include "base.hpp"
#include "loader.hpp"


#if defined(__APPLE__)
#include "lldb/API/SBDebugger.h"
#include "lldb/API/SBTarget.h"
#include "lldb/API/SBModule.h"
#include "lldb/API/SBFunction.h"
#include "lldb/API/SBSymbol.h"
#include "lldb/API/SBSymbolContext.h"
#include "lldb/API/SBStream.h"
extern "C" {
#include "symbolicator.h"
}
#endif

class Images;
class Frames;

typedef uint64_t addr_t;
typedef Images images_t;
typedef Frames frames_t;

#if defined(__APPLE__)
typedef struct DebugData {
    lldb::SBDebugger debugger;
    lldb::SBTarget cur_target;
}debug_data_t;
#endif

class Images {
    std::string procname;
    pid_t pid;
    tid_t tid;
    struct exec_path {
        uint64_t vm_offset;
        std::string path;
    } main_proc;
    std::map<std::string, addr_t> modules_loaded_map;
public:
    Images(uint64_t tid, std::string procname);
    ~Images(){modules_loaded_map.clear();}

    tid_t get_tid() {return tid;}
    void set_pid(pid_t _pid) {pid = _pid;}
    pid_t get_pid(void) {return pid;}
    std::string &get_procname(void) {return procname;}
    std::map<std::string, uint64_t> &get_modules() {return modules_loaded_map;}

    void set_main_proc(addr_t vm_offset, std::string path) {main_proc.vm_offset = vm_offset; main_proc.path = path;}
    std::string get_main_proc_path(void) {return main_proc.path;}
    void add_module(uint64_t vm_offset, std::string path);
    std::string search_path(uint64_t vm_addr);
    void decode_images(std::ofstream &outfile);
};


class Frames {
public:
    typedef struct {
        addr_t addr;
        std::string symbol;
        std::string filepath;
		uint64_t freq;
    } frame_info_t;

	static bool compare_freq(frame_info_t elem1, frame_info_t elem2) {
		return elem1.freq > elem2.freq;
	}

    typedef std::map<addr_t, std::string> symmap_t;
    typedef std::map<std::string, symmap_t> path_to_symmap_t;

protected:
    uint64_t host_event_tag;
    uint64_t tid;
    uint64_t max_frames;
    Images *image;
    std::vector<frame_info_t> frames_info;
    bool is_spinning;

#if DEBUG_SYM
    static int32_t check_symtable_once;
#endif
private:
    std::string addr_to_path(addr_t addr);
    std::string addr_to_sym(addr_t vm_offset, symmap_t &vm_sym_map);
    bool need_correct(int index);

public:
    Frames(uint64_t _tag, uint64_t _max_frame, uint64_t _tid);
    ~Frames();
    void set_image(Images *img) {image = img;} 
    bool add_frame(addr_t addr);
    bool add_frame(addr_t addr, std::string path);
	bool add_frame(int index, addr_t addr, std::string sym, std::string path, uint64_t freq);
    uint64_t get_tag(void) {return host_event_tag;}
    uint64_t get_size(void) {return frames_info.size();}
	std::vector<frame_info_t> get_frames_info() {return frames_info;}
    addr_t get_addr(int index);
    std::string get_filepath(int index);
    std::string get_sym(int index);
    uint64_t get_freq(int index);
    bool update_frame_path(int index, std::string filepath);
    bool update_frame_sym(int index, std::string symbol);
    bool contains_symbol(std::string func);

#if defined(__APPLE__)
    bool symbolize_with_lldb(int index, debug_data_t *debugger_data);
    bool correct_symbol(int index, path_to_symmap_t &path_to_vmsym_map);
    void symbolication(debug_data_t *debugger, path_to_symmap_t &path_to_vmsym_map);
#endif

    void decode_frames(std::ofstream &outfile);
    void decode_frames(std::ostream &out);
};


class BacktraceEvent: public EventBase, public Frames {
	bool symbolized;
    EventBase *host_event;
public:
    BacktraceEvent(double abstime, std::string op, uint64_t tid, uint64_t _tag, uint64_t max_frames, uint32_t core_id, std::string procname="");
    ~BacktraceEvent(void);
	bool is_symbolized() {return symbolized;}
	void set_symbolized() {symbolized = true;}
    void add_frames(uint64_t *frames, int size);
    bool hook_to_event(EventBase *event, event_type_t event_type);
    EventBase *get_hooked_event(void) {return host_event;}
    bool spinning() {return is_spinning;}
	void count_symbols();
	std::list<Frames::frame_info_t> sort_symbols();
	std::list<uint64_t> get_frames();
    void decode_event(bool is_verbose, std::ofstream &outfile);
    void streamout_event(std::ofstream &outfile);
    void streamout_event(std::ostream &out);
	void log_event();
};
#endif
