#include "parser.hpp"
#include "backtraceinfo.hpp"

#define DEBUG_PARSER 0

Parse::BacktraceParser::BacktraceParser(std::string filename, std::string _path_log)
:Parser(filename), path_log(_path_log)
{
    backtrace_events.clear();
    images_events.clear();
    image_subpaths.clear();
    proc_images_map.clear();
    proc_backtraces_map.clear();
    path_to_vmsym_map.clear();
}

Parse::BacktraceParser::~BacktraceParser()
{
    for (auto &elem: proc_images_map)
        delete(elem.second);
    for (auto &elem: proc_backtraces_map)
        elem.second.clear();
    for (auto &elem: path_to_vmsym_map)
        elem.second.clear();

    path_to_vmsym_map.clear();
    proc_images_map.clear();
    proc_backtraces_map.clear();

    for (auto &elem: backtrace_events)
        delete(elem.second);
    for (auto &elem: images_events)
        delete(elem.second);
    for (auto &elem: image_subpaths)
        delete(elem.second);
    backtrace_events.clear();
    images_events.clear();
    image_subpaths.clear();
}

void Parse::BacktraceParser::collect_backtrace_for_proc(
        BacktraceEvent *backtrace_event, std::string procname)
{
    uint64_t tid = backtrace_event->get_tid();
	proc_key_t key;
	key.second = procname;

    if (LoadData::tpc_maps.find(tid) == LoadData::tpc_maps.end()) {
        outfile << "unknown pid for tid " << std::hex << tid << " at " << __func__ << std::endl;
		key.first = -1;
    } else {
    	ProcessInfo *p = LoadData::tpc_maps[tid];
		key.first = p->get_pid();
	}
	

	if (proc_backtraces_map.find(key) == proc_backtraces_map.end()) {
		std::list<BacktraceEvent *> l;
		l.clear();
		proc_backtraces_map[key] = l;
	}

	//outfile << std::hex << key.first << " before size " << proc_backtraces_map[key].size() << std::endl;
	proc_backtraces_map[key].push_back(backtrace_event);
	//outfile << std::hex << key.first << " after size " << proc_backtraces_map[key].size() << std::endl;
}

void Parse::BacktraceParser::gather_frames(BacktraceEvent *backtrace_event,
        double abstime, uint64_t *frames, uint64_t size, bool is_end)
{
    int i = size - 1;
    while(frames[i] == 0 && i >= 0) {
		is_end = true;
        i--;
	}

    if (i >= 0) {
        size = frames[i] == 0 ? 0 : i + 1;
        backtrace_event->add_frames(frames, size);
        backtrace_event->override_timestamp(abstime - 0.5);
    }
    if (is_end == true) {
        backtrace_events.erase(backtrace_event->get_tid());
        backtrace_event->set_complete();
    }
}

bool Parse::BacktraceParser::process_frame(std::string opname, double abstime,
        std::istringstream &iss, bool is_end)
{
    uint64_t host_tag, frame[3], tid, coreid;
    std::string procname = "";

    if (!(iss >> std::hex >> host_tag >> frame[0] >> frame[1] >> frame[2] \
                >> tid >> coreid))
        return false;

    if (!getline(iss >> std::ws, procname) || !procname.size()) {
        outfile << "Error: procname for frame should not be empty at";
        outfile << std::fixed << std::setprecision(1) << abstime << std::endl;
        procname = "";
    }

retry:
    if (backtrace_events.find(tid) == backtrace_events.end()) {
		/*
        if (frame[0] > TRACE_MAX) {
            outfile << "Error: not a new trace event" << std::endl;
            return false;
        }
		*/

        BacktraceEvent *backtrace_event = new BacktraceEvent(abstime,
                opname, tid, host_tag, frame[0], coreid, procname);
        if (!backtrace_event) {
            std::cerr << "OOM " << __func__ << std::endl;
            exit(EXIT_FAILURE);
        }

        collect_backtrace_for_proc(backtrace_event, procname);
        backtrace_events[tid] = backtrace_event;
        local_event_list.push_back(backtrace_event);
        gather_frames(backtrace_event, abstime, frame + 1, 2, is_end);
    } else {
        BacktraceEvent *backtrace_event = backtrace_events[tid];
        if (backtrace_event->get_tag() != host_tag) {
            backtrace_events.erase(tid);    
            backtrace_event->set_complete();
            goto retry;
        }
        gather_frames(backtrace_event, abstime, frame, 3, is_end);
    }
    return true;
}

//////////////////////////////////
//process paths from tracing data
//////////////////////////////////
bool Parse::BacktraceParser::collect_image_for_proc(images_t *cur_image)
{
	proc_key_t key = {.first = cur_image->get_pid(), .second = cur_image->get_procname()};

	/*
	for (auto element : proc_images_map) {
		if (element.first.first == pid && element.first.second == procname) {
			outfile << "Check reload image for " << procname \
				<< "\tpid=" << std::hex << pid << std::endl;
			return false;
		}
	}
	*/
	
	if (proc_images_map.find(key) != proc_images_map.end()) {
		outfile << "Reload image for " << key.second\
				<< "\tpid = " << std::dec << key.first << std::endl;
		return false;
	}
   	proc_images_map[key] = cur_image;
   	return true;
}

bool Parse::BacktraceParser::complete_path(tid_t tid)
{
    assert(images_events.find(tid) != images_events.end());
    assert(image_subpaths.find(tid) != image_subpaths.end());

    images_t *cur_image = images_events[tid];
    raw_path_t *cur_path = image_subpaths[tid];
    std::string new_path = QueueOps::queue_to_string(cur_path->path);
    addr_t vm_offset = cur_path->vm_offset;
    bool ret;

    if (new_path.length()) {
        if (cur_image->get_main_proc_path().length() == 0)
            cur_image->set_main_proc(vm_offset, new_path);
        cur_image->add_module(vm_offset, new_path);
        ret = true;       
    } else {
        std::cerr << "Invalid path in " << __func__ << std::endl;
        ret = false;
    }

    if (!collect_image_for_proc(cur_image))
        delete(cur_image);

    return ret;
}       

bool Parse::BacktraceParser::gather_path(tid_t tid, addr_t vm_offset, addr_t *addrs)
{
    assert(image_subpaths.find(tid) != image_subpaths.end());
    raw_path_t *cur_path = image_subpaths[tid];
    bool ret;

    if (vm_offset != cur_path->vm_offset &&
            cur_path->vm_offset != uint64_t(-1)) {
        ret = complete_path(tid);
    }

    cur_path->vm_offset = vm_offset;
    QueueOps::triple_enqueue(cur_path->path, addrs[0], addrs[1], addrs[2]);

    return true;
}

bool Parse::BacktraceParser::create_image(tid_t tid, std::string procname)
{
    images_t *new_image = new images_t(tid, procname);
    if (!new_image) {
        std::cerr << "OOM " << __func__ << std::endl;
        return false;
    }
    if (LoadData::tpc_maps.find(tid) != LoadData::tpc_maps.end()) {
        new_image->set_pid(LoadData::tpc_maps[tid]->get_pid());
    } else {
        std::cerr << "Missing pid info " << __func__ << std::endl;
        delete (new_image);
        return false;
    }
    images_events[tid] = new_image;

    raw_path_t *cur_path = new raw_path_t;
    if (!cur_path) {
        std::cerr << "OOM " << __func__ << std::endl;
        return false;
    }
    cur_path->init();
    assert(image_subpaths.find(tid) == image_subpaths.end());
    image_subpaths[tid] = cur_path;
    return true;
}

bool Parse::BacktraceParser::process_path(std::string opname, double abstime, std::istringstream &iss)
{
    uint64_t vm_offset, addr[3], tid, coreid;
    std::string procname = "";
    bool ret = true;

    if (!(iss >> std::hex >> vm_offset >> addr[0] >> addr[1] >> addr[2]\
                >> tid >> coreid))
        return false;

    if (!getline(iss >> std::ws, procname) || !procname.size()) {
        outfile << "Error: missing procname for lib info" << std::endl;
        return false;
    }

    if (images_events.find(tid) == images_events.end()) {
        assert(vm_offset == 0);
        return create_image(tid, procname);
    } else {
        assert(image_subpaths.find(tid) != image_subpaths.end());
        assert(vm_offset != 0);
        
        if (vm_offset == 1) {
            ret = complete_path(tid);
            if (ret == false)
                delete(images_events[tid]);
            delete(image_subpaths[tid]);
            image_subpaths.erase(tid);
            images_events.erase(tid);
            return ret;
        }
        return gather_path(tid, vm_offset, addr);
    }
}

bool Parse::BacktraceParser::process_path_from_log(void)
{
    if(!path_log.size()) {
        std::cerr << "No libinfo file for parsing" << std::endl;
        return false;
    }
    std::ifstream path_in(path_log);
    if (path_in.fail()) {
        outfile << "No log file " << std::endl;
        return false;
    }
#if DEBUG_PARSER
    std::cerr << "Process backtrace log " << path_log  << std::endl;
#endif

    std::string line, procname, path, arch, unused;
    images_t *cur_image = nullptr;
    bool seek_main = false;
    addr_t vm_offset;
    pid_t pid;

    while(getline(path_in, line)) {
        std::istringstream iss(line);
        if (line.find("DumpProcess") != std::string::npos) {
            if (cur_image && !collect_image_for_proc(cur_image))
                delete(cur_image);

            cur_image = nullptr;
            if (!(iss >> unused >> std::hex >> pid >> arch)
                    || !getline(iss >> std::ws, procname)
                    || !procname.size()) {
                outfile  << "Error from path log : " << line << std::endl;
                continue;
            }

            std::cerr << "create image for proc " << procname << std::endl;

            if (arch.find("i386") != std::string::npos)
                host_arch = "i386";
            else
                host_arch = "x86_64";

            cur_image = new images_t(-1, procname);
            if (!cur_image) {
                std::cerr << "OOM " << __func__ << std::endl;
                exit(EXIT_FAILURE);
            }
            cur_image->set_pid(pid);
            seek_main = true;
        } else {
            if (!cur_image) {
                outfile << "Error no image " << line << std::endl;
                continue;
            }

            if (!(iss >> std::hex >> vm_offset)
                    || !getline(iss >> std::ws, path)
                    || !path.size()) {
                outfile  << "Error from path log : " << line << std::endl;
                continue;
            }

            cur_image->add_module(vm_offset, path);
            if (seek_main) {
                cur_image->set_main_proc(vm_offset, path);
                seek_main = false;
            }
        }
    }

    if (cur_image && !collect_image_for_proc(cur_image))
        delete(cur_image);
    path_in.close();
    return true;
}

std::list<BacktraceEvent *> Parse::BacktraceParser::get_event_list(
	proc_key_t key)
{
	if (proc_backtraces_map.find(key) == proc_backtraces_map.end()) {
		backtrace_list_t event_list;
		event_list.clear();
		return event_list;
	}
	return proc_backtraces_map[key];
}

void Parse::BacktraceParser::parse_frame(std::string line, BacktraceEvent *cur_event)
{
	if (cur_event == nullptr) {
		LOG_S(INFO) << "unable to attach symbol to event: " << line; 
		return;
	}

	//matching frames in the event
	size_t pos_0 = line.find("Frame [") + sizeof("Frame [") - 1;
	size_t pos_1 = line.find("]");

	if (pos_0 == std::string::npos || pos_1 == std::string::npos)
		return;

	int index = std::stoi(line.substr(pos_0, pos_1 - pos_0), nullptr, 16);

	line = line.substr(pos_1);
	pos_0 = line.find("0x") + 2;
	pos_1 = line.find(" :");
	if (pos_0 == std::string::npos || pos_1 == std::string::npos)
		return;
	uint64_t addr = std::stoull(line.substr(pos_0, pos_1 - pos_0), nullptr, 16);

	pos_0 = pos_1 + sizeof(" :") - 1;
	pos_1 = line.find("\t");
	if (pos_0 == std::string::npos || pos_1 == std::string::npos)
		return;
	std::string symbol = line.substr(pos_0, pos_1 - pos_0);

	line = line.substr(pos_1 + sizeof("\t") - 1);
	pos_1 = line.find("\t");
	if (pos_1 == std::string::npos)
		return;
	std::string path = line.substr(0, pos_1);

	pos_0 = pos_1 + sizeof("\t") - 1;
	if (pos_1 == std::string::npos)
		return;
	uint64_t freq = std::stoull(line.substr(pos_0), nullptr, 10);

	cur_event->add_frame(index, addr, symbol, path, freq);
}

void Parse::BacktraceParser::read_symbol_from_file()
{
	//convert file to event backtrace symbols 
    std::string btInfo("backtrace_bt_info");
	std::ifstream pFile(btInfo);
	if (!pFile) {
		LOG_S(INFO) << "unable to open file: " << btInfo; 
		return; 
	}
	if (pFile.peek() == std::ifstream::traits_type::eof()) {
		LOG_S(INFO) << "file " << btInfo << " is empty";
		pFile.close();
		return;
	}
	std::string line;
	backtrace_list_t event_list;
	size_t pos = std::string::npos;
	key_t key;
	size_t size;

	while (getline(pFile, line)) {
		if ((pos = line.find("Decode backtrace for ")) != std::string::npos) {
			key.second = line.substr(pos + sizeof("Decode backtrace for ") - 1);
#if DEBUG_PARSER
			LOG_S(INFO) << "Read backtrace for " << key.second;
#endif
		}

		if ((pos = line.find("Pid = ")) != std::string::npos) {
#if DEBUG_PARSER
			LOG_S(INFO) << "Pid = " << line.substr(pos + sizeof("Pid = ") - 1);
#endif
			key.first = std::stoi(line.substr(pos + sizeof("Pid = ") - 1));
		}

		if ((pos = line.find("List size = ")) != std::string::npos) {

			if (key.second.size() == 0 || key.first == -1)
				continue;

			event_list = get_event_list(key);
			if (event_list.size() == 0)
				continue;

			size = std::stoi(line.substr(pos + sizeof("List size = ") - 1));
#if DEBUG_PARSER
			LOG_S(INFO) << "Size = " << std::dec << event_list.size();
#endif
			assert(event_list.size() == size);

			auto it = event_list.begin();
			bool begin_event = false;
			BacktraceEvent *cur_event = nullptr;
			while (it != event_list.end() && getline(pFile, line)) {
				if (line.find("ARGUS_DBG_BT") != std::string::npos) {
					if (begin_event == false) {
						begin_event = true;
					} else {
						//matching next event in the list
						(*it)->count_symbols();
						++it;
					}
					cur_event = *it;
					continue;
				}

				if ((pos = line.find("Decode backtrace for ")) != std::string::npos) {
					key.second = line.substr(pos + sizeof("Decode backtrace for ") - 1);
					key.first = -1;
#if DEBUG_PARSER
					LOG_S(INFO) << "Read backtrace for " << key.second;
#endif
					break;
				}

				if (line.find("Frame") == std::string::npos) {
					cur_event = nullptr;
					continue;
				}
				parse_frame(line, cur_event);
				if (cur_event->is_symbolized() == false)
					cur_event->set_symbolized();
				if (line.find("ConnectionSetSpinning") != std::string::npos) {
					LOG_S(INFO) << line;
				}
			}
			key.first = -1;
			key.second ="";
		}
	}
	pFile.close();
}


void Parse::BacktraceParser::symbolize_backtraces()
{
	//check if file exists
    std::string btInfo("backtrace_bt_info");
	std::ifstream pFile(btInfo);
	bool read_from_file = false;
	if (pFile && pFile.peek() != std::ifstream::traits_type::eof()) {
		read_from_file = true;
	}
	if (pFile)
		pFile.close();

	if (read_from_file) {
		read_symbol_from_file();
	} else {
#if defined(__APPLE__)
		symbolize_backtraces_with_lldb();
#endif
	}
}

#if defined(__APPLE__)
void Parse::BacktraceParser::symbolize_backtraces_with_lldb()
{

	std::map<proc_key_t, images_t *>::iterator image_it;
	debug_data_t cur_debugger;
	backtrace_list_t event_list;
	backtrace_list_t::iterator event_it;

	for (image_it = proc_images_map.begin(); image_it != proc_images_map.end(); image_it++) {
		event_list = get_event_list(image_it->first);
		if (event_list.size() == 0 
				||!setup_lldb(&cur_debugger, image_it->second))
            goto clear_debugger;

        for (event_it = event_list.begin(); event_it != event_list.end(); event_it++) {
            (*event_it)->set_image(image_it->second);
            (*event_it)->symbolication(&cur_debugger, path_to_vmsym_map);
			(*event_it)->count_symbols();
			(*event_it)->set_symbolized();
			if ((*event_it)->contains_symbol("NSEventThread")
        		&& (*event_it)->contains_symbol(LoadData::meta_data.suspicious_api))
				LOG_S(INFO) << "NSEventThread spinning is detected in thread 0x" << std::hex\
					<< (*event_it)->get_tid()\
					<< " pid = " << std::dec << (*event_it)->get_pid()\
					<< " at " << std::fixed << std::setprecision(1)\
					<< (*event_it)->get_abstime();
        }

clear_debugger:    
        if (cur_debugger.debugger.IsValid()) {
            if (cur_debugger.cur_target.IsValid())
                cur_debugger.debugger.DeleteTarget(cur_debugger.cur_target);
			cur_debugger.debugger.Clear();
            lldb::SBDebugger::Destroy(cur_debugger.debugger);
        } 
    }

	//save backtrace to a json file
    std::string outfile("backtrace_bt.log");
    std::ofstream out_bt(outfile, std::ofstream::out);//| std::ofstream::app);
    for (image_it = proc_images_map.begin(); image_it != proc_images_map.end(); image_it++) {
		event_list = get_event_list(image_it->first);
		if (event_list.size() == 0)
			continue;

        out_bt << "Decode backtrace for "\
			 << (image_it->first).second << std::endl;
        out_bt << "Pid = " << std::dec << (image_it->first).first << std::endl;
        out_bt << "List size = " << std::dec <<  event_list.size() << std::endl;
        for (event_it = event_list.begin(); event_it != event_list.end(); event_it++) {
			(*event_it)->sort_symbols();
            (*event_it)->streamout_event(out_bt);
		}
	}
    out_bt.close();
}
#endif


images_t *Parse::BacktraceParser::proc_to_image(pid_t pid, std::string procname)
{
	proc_key_t key = {.first = pid, .second = procname};
	if (proc_images_map.find(key) != proc_images_map.end())
		return proc_images_map[key];

    return nullptr;
}

#if defined(__APPLE__)
bool Parse::BacktraceParser::setup_lldb(debug_data_t *debugger_data, images_t *images)
{
    lldb::SBError err;
    if (images == nullptr) {
        std::cerr << "Error: fail to get image" << std::endl;
        return false;
    }

    debugger_data->debugger = lldb::SBDebugger::Create(true, nullptr, nullptr);
    if (!debugger_data->debugger.IsValid()) {
        std::cerr << "Error: fail to create a debugger object" << std::endl;
        return false;
    }

    debugger_data->cur_target = debugger_data->debugger.CreateTarget(
            images->get_main_proc_path().c_str(),
            host_arch.c_str(), /*"x86_64"*/
            nullptr, true, err);
    if (!err.Success() || !debugger_data->cur_target.IsValid()) {
        std::cerr << "Error: fail to create target" << std::endl;
        std::cerr << "\t" << err.GetCString() << std::endl;
        return false;
    }

    std::string platform = host_arch.find("i386") != std::string::npos\
                           ? "i386-apple-macosx" : "x86_64-apple-macosx";
    std::cerr << "platform for lldb " << platform << std::endl;

    std::map<std::string, uint64_t> &modules = images->get_modules();
    std::map<std::string, uint64_t>::iterator it;

    for (it = modules.begin(); it != modules.end(); it++) {
        lldb::SBModule cur_module = debugger_data->cur_target.AddModule(
                (it->first).c_str(), platform.c_str(), nullptr);
        if (!cur_module.IsValid()) {
            std::cerr << "Check: invalid module path " << it->first << std::endl;
            continue;
        }
        err = debugger_data->cur_target.SetSectionLoadAddress(
                cur_module.FindSection("__TEXT") , lldb::addr_t(it->second));    
        if (!err.Success()) {
            std::cerr << "Check: fail to load section for " << it->first << std::endl;
            std::cerr << "Check: " << err.GetCString() << std::endl;
            continue;
        }
        /* TODO : add DATA section to symbolicate watched variables
           err = debugger_data->cur_target.SetSectionLoadAddress(
           cur_module.FindSection("__DATA") , lldb::addr_t(it->second));    

           if (!err.Success()) {
           std::cerr << "Check: fail to load section for " << it->first << std::endl;
           std::cerr << "Check: " << err.GetCString() << std::endl;
           continue;
           }
         */
    }
    return true;
}
#endif

void Parse::BacktraceParser::process()
{
    std::string line, deltatime, opname;
    double abstime;
    bool ret = false;

	LOG_S(INFO) << "Parse backtrace...";
    process_path_from_log();

    while (getline(infile, line)) {
        std::istringstream iss(line);
        if (!(iss >> abstime >> deltatime >> opname)) {
            outfile << line << std::endl;
            continue;
        }
        if (opname == "ARGUS_Path_info") {
            ret = process_path(opname, abstime, iss);
        } else if (opname == "ARGUS_DBG_BT") {
            bool is_end = false;
            if (deltatime.find("(") != std::string::npos)
                is_end = true;
            ret = process_frame(opname, abstime, iss, is_end);
        } else {
            ret = false;
            outfile << "Error: unknown operation" << std::endl;
        }

        if (ret == false)
            outfile << line << std::endl;
    }
	LOG_S(INFO) << "Finished parsing backtrace, symbolize...";
    // symbolicated backtrace events
    symbolize_backtraces();
	LOG_S(INFO) << "Finished symbolication";
}
