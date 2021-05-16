#include "backtraceinfo.hpp"

Images::Images(uint64_t _tid, std::string _procname)
{
    procname = _procname;
    pid = -1;
    tid = _tid;
    main_proc = {.path = "", .vm_offset = uint64_t(-1)};
    modules_loaded_map.clear();
}

void Images::add_module(uint64_t vm_offset, std::string path)
{
    if (path != main_proc.path
        && modules_loaded_map.find(path) != modules_loaded_map.end()) {
        std::cerr << "Warn: reload identical libraries path:";
        std::cerr << "\n\tlibpath: " << path;
        std::cerr << "\n\tprocess: " << procname << std::endl;
    }
    modules_loaded_map[path] = vm_offset;
}

std::string Images::search_path(uint64_t vm_addr)
{
    //find path offset that is most near and less than vm_addr
    std::map<std::string, uint64_t>::iterator it;
    uint64_t max_less = 0;
    std::string path = "";
    for (it = modules_loaded_map.begin(); it != modules_loaded_map.end(); it++) {
        if (it->second <= vm_addr && it->second > max_less) {
            max_less = it->second;
            path = it->first;
        }
    }
    return path;
}


void Images::decode_images(std::ofstream &outfile)
{
    outfile << procname << "\n"; 
    outfile << "\nTarget Path: " << main_proc.path << " load_vm: " << std::hex << main_proc.vm_offset << std::endl;
    std::map<std::string, uint64_t>::iterator it;
    if (modules_loaded_map.size())
        outfile << "Libs : \n";
    for (it = modules_loaded_map.begin(); it != modules_loaded_map.end(); it++) {
        outfile << "\t" << it->first << " load_vm: " << it->second << std::endl;
    }
}
