#include "backtraceinfo.hpp"
#define DEBUG_SYM 0

Frames::Frames(uint64_t _tag, uint64_t _max_frames, uint64_t _tid)
{
    host_event_tag = _tag;
    max_frames = _max_frames;
    tid = _tid;
    frames_info.clear();
	is_spinning = false;
}

#if DEBUG_SYM
int32_t Frames::check_symtable_once = 0;
#endif

Frames::~Frames()
{
    image = nullptr;
    frames_info.clear();
}

addr_t Frames::get_addr(int index)
{
    if (frames_info.size() > max_frames || frames_info.size() <= index)
        return 0;
    return frames_info[index].addr;
}

std::string Frames::get_filepath(int index)
{
    if (frames_info.size() > max_frames || frames_info.size() <= index)
        return std::string("path_null_in_") + __func__;
    return frames_info[index].filepath;
}

std::string Frames::get_sym(int index)
{
    if (frames_info.size() > max_frames || frames_info.size() <= index)
        return "";
    return frames_info[index].symbol;
}

uint64_t Frames::get_freq(int index)
{
    if (frames_info.size() > max_frames || frames_info.size() <= index)
        return 0;
    return frames_info[index].freq;
}

bool Frames::update_frame_path(int index, std::string filepath)
{
    if (frames_info.size() > max_frames || frames_info.size() <= index)
        return false;
    frames_info[index].filepath = filepath;
    return true;
}

bool Frames::update_frame_sym(int index, std::string symbol)
{
    if (frames_info.size() > max_frames || frames_info.size() <= index)
        return false;
    frames_info[index].symbol = symbol;

    if (frames_info.size() == index + 1
        && contains_symbol("NSEventThread")
        && contains_symbol(LoadData::meta_data.suspicious_api)) {
        is_spinning = true;
	}

    return true;
}

bool Frames::add_frame(addr_t addr)
{
    if (frames_info.size() > max_frames)
        return false;

    frame_info_t new_frame;
    new_frame.addr = addr;
    new_frame.symbol.clear();
    new_frame.filepath.clear();
	new_frame.freq = 0;
    frames_info.push_back(new_frame);
    return true;
}

bool Frames::add_frame(addr_t addr, std::string path)
{
    if (frames_info.size() > max_frames)
        return false;

    frame_info_t new_frame;
    new_frame.addr = addr;
    new_frame.filepath = path;
    new_frame.symbol.clear();
	new_frame.freq = 0;
    frames_info.push_back(new_frame);
    return true;
}

bool Frames::add_frame(int index, addr_t addr, std::string symbol, std::string path, uint64_t freq)
{
	if (frames_info.size() < index) {
		LOG_S(INFO) << "Error: add frame to backtrace at index (!= "\
			<< std::dec << frames_info.size() << ") "\
			<< std::dec << index;
		return false;
	}
	if (frames_info[index].addr != addr) {
		LOG_S(INFO) << "Error: frame addr = " \
			<< std::hex << frames_info[index].addr\
			<< " != " << std::hex << addr;
	}
    assert(frames_info[index].addr == addr);
    update_frame_path(index, path);
    update_frame_sym(index, symbol);          
	frames_info[index].freq = freq;
    
	return true;
}

bool Frames::contains_symbol(std::string func)
{
    std::vector<frame_info_t>::iterator it;
    
    for (it = frames_info.begin(); it != frames_info.end(); it++) {
        if ((*it).symbol.find(func) != std::string::npos)
            return true;
    }
    return false;
}

std::string Frames::addr_to_path(addr_t addr)
{
    if (!image)
        return std::string("no_image_for_search_in_") + __func__;

    std::map<std::string, uint64_t> &modules = image->get_modules();
    std::map<std::string, uint64_t>::iterator it, ret;
    uint64_t load_addr = addr + 8;
    
    for (it = modules.begin(); it != modules.end(); it++)
        if (it->second <= addr && it->second < load_addr) {
            load_addr = it->second;
            ret = it;
        }
    if (load_addr <= addr)
        return ret->first;
    return std::string("path_null_in_") + __func__;
}

std::string Frames::addr_to_sym(uint64_t vm_offset, symmap_t &vm_sym_map)
{
    if (vm_sym_map.size() == 0)
        return "empty_sym";
#if DEBUG_SYM
    LOG_S(INFO) << "Check symbols";
    if (Frames::check_symtable_once == 0) {
        Frames::check_symtable_once = 1;
        std::map<uint64_t, std::string>::iterator it;
        LOG_S(INFO) << "symbol table size = " << vm_sym_map.size();
        for (it = vm_sym_map.begin(); it != vm_sym_map.end(); it++) {
            LOG_S(INFO) << "[" << std::hex << it->first << "]\t" << it->second;
        }
    }
#endif
    std::map<uint64_t, std::string>::iterator next = vm_sym_map.upper_bound(vm_offset);
    if (next != vm_sym_map.begin())
        next--;
    return next->second;
}

bool Frames::need_correct(int index)
{
    if (frames_info.size() > max_frames
        || frames_info.size() <= index
        || LoadData::meta_data.pid == 0)
        return false;

    if (get_filepath(index).find("CoreGraphics") != std::string::npos
		|| get_filepath(index).find("SkyLight") != std::string::npos)
        return true;

    return false;
}

#if defined(__APPLE__)
bool Frames::symbolize_with_lldb(int index, debug_data_t *debugger_data)
{
    //if (frames_info.size() > max_frames || frames_info.size() <= index)
	if (index >= frames_info.size())
        return false;

    lldb::SBAddress sb_addr = debugger_data->cur_target.ResolveLoadAddress(frames_info[index].addr); 
    bool success = sb_addr.IsValid() && sb_addr.GetSection().IsValid();
    if (success) {
        lldb::SBSymbolContext sc = debugger_data->cur_target.ResolveSymbolContextForAddress(sb_addr, lldb::eSymbolContextEverything);
        lldb::SBStream strm;
        sc.GetDescription(strm);
        std::string desc(strm.GetData());
        desc.erase(std::remove(desc.begin(), desc.end(), '\n'), desc.end());

        std::string filepath = "lldb_unkown_path";
        std::string symbol = "lldb_unknown_sym";
        
        size_t pos = desc.find("file =");
        size_t pos_1 = pos != std::string::npos ? desc.find("\"", pos) : std::string::npos;
        size_t pos_2 = pos_1 != std::string::npos ? desc.find("\"", pos_1 + 1) : std::string::npos;
        
        if (pos_2 != std::string::npos) {
            pos_1++;
            filepath = desc.substr(pos_1, pos_2 - pos_1);
        }

        pos = desc.find("name=");
        pos_1 = pos != std::string::npos ? desc.find("\"", pos) : std::string::npos;    
        pos_2 = pos_1 != std::string::npos ? desc.find("\"", pos_1 + 1) : std::string::npos;
        
        if (pos_2 != std::string::npos) {
            pos_1++;
            symbol = desc.substr(pos_1, pos_2 - pos_1);
        } else {
            pos = desc.find("mangled=");
            pos_1 = pos != std::string::npos ? desc.find("\"", pos) : std::string::npos;
            pos_2 = pos_1 != std::string::npos ? desc.find("\"", pos_1 + 1) : std::string::npos;
            if (pos_2 != std::string::npos) {
                pos_1++;
                symbol = desc.substr(pos_1, pos_2 - pos_1);
            }
        }
        update_frame_path(index, filepath);
        update_frame_sym(index, symbol);          
    }
    return success;
}

bool Frames::correct_symbol(int index, path_to_symmap_t &path_to_vmsym_map)
{
    if (LoadData::meta_data.pid == 0 || frames_info.size() <= index)
        return false;

    if (get_filepath(index).size() == 0)
        update_frame_path(index, addr_to_path(get_addr(index)));

	if (image->get_modules().find(get_filepath(index)) == image->get_modules().end())
		return false;
        
    addr_t vm_offset = get_addr(index) - image->get_modules()[get_filepath(index)];

    if (path_to_vmsym_map.find(get_filepath(index)) == path_to_vmsym_map.end()) {
        struct mach_o_handler mh;
        memset(&mh, 0, sizeof(struct mach_o_handler));
        if (get_syms_for_libpath(LoadData::meta_data.pid, get_filepath(index).c_str(), &mh)) {
            symmap_t vm_syms_map;
            uint64_t vm_offset = 0;
            uint64_t  str_index = 0;
            for (int i = 0; i < mh.nsyms; i++) {
                vm_offset = mh.symbol_arrays[i].vm_offset;
                if (vm_offset == mh.vm_slide - (uint64_t)mh.mach_address)
                    break; //dysymbol stubs
                str_index = mh.symbol_arrays[i].str_index;
                vm_syms_map[vm_offset] = std::string(mh.strings + str_index + 1);
            }

            if (vm_syms_map.size() > 0)
                path_to_vmsym_map[get_filepath(index)] = vm_syms_map;
        }
        if (mh.strings)
            free((void *)mh.strings);
        if (mh.symbol_arrays)
            free((void *)mh.symbol_arrays);
    }
    
    if (path_to_vmsym_map.find(get_filepath(index)) != path_to_vmsym_map.end()) {
        update_frame_sym(index, addr_to_sym(vm_offset, path_to_vmsym_map[get_filepath(index)]));
        return true;
    }
#if DEBUG_SYM
    else
        LOG_S(INFO) << "Unable to get syms";
#endif
    return false;
}

void Frames::symbolication(debug_data_t *debugger_data, path_to_symmap_t &path_to_vmsym_map)
{
    for (int index = 0; index < frames_info.size(); index++) {
        if (get_addr(index) < 0xfff) {
            std::string desc("leaked_frame");
            update_frame_sym(index, desc);
            continue;
        }   
        
        bool success = symbolize_with_lldb(index, debugger_data);
		

        if (success == false || 
			need_correct(index) == true)
            success = correct_symbol(index, path_to_vmsym_map);

#if DEBUG_SYM
		if (success == false) {
			LOG_S(INFO) << "Fail to symbolize symbol for tid = " << std::hex << tid << " ";
			if (LoadData::tpc_maps.find(tid) != LoadData::tpc_maps.end()) {
				ProcessInfo *info = LoadData::tpc_maps[tid];
				LOG_S(INFO) << " in proc " << info->get_procname() << " ";
			}
			LOG_S(INFO) << "at 0x" << std::hex << get_addr(index);
		}
#endif

        if (image->get_modules().find(get_filepath(index)) != image->get_modules().end() ) {
			if (get_sym(index).find("__lldb_unnamed_function") != std::string::npos
                || get_sym(index).size() == 0) {
            	addr_t offset = get_addr(index) - image->get_modules()[get_filepath(index)];
            	std::stringstream hex_offset_stream;
            	hex_offset_stream << "_offset_0x_" << std::hex << offset;
            	update_frame_sym(index, get_sym(index) + hex_offset_stream.str());
			}
        } else {
           	std::stringstream desc_stream;
			desc_stream << "__lldb_unfound_path_va_0x" << std::hex << get_addr(index);
			update_frame_sym(index, desc_stream.str());
		}
    }
}
#endif

void Frames::decode_frames(std::ofstream &outfile)
{
    for (int index = 0; index < frames_info.size(); index++)
        outfile << "Frame [" << std::setfill('0') << std::setw(2) << std::hex << index <<"]: "\
            << "0x" << std::setfill('0') << std::setw(16) << std::hex << get_addr(index) << " :"\
            << get_sym(index) << "\t" << get_filepath(index)\
			<< "\t" << std::dec << frames_info[index].freq << std::endl;
    outfile << std::endl;
}

void Frames::decode_frames(std::ostream &out)
{
    for (int index = 0; index < frames_info.size(); index++)
        out << "[" << std::dec << index << "] : "<< get_sym(index) << "\t" << get_filepath(index)\
		<< "\t" << frames_info[index].freq << std::endl;
}
