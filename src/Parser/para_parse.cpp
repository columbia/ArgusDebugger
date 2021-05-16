#include "parser.hpp"
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>
#include <boost/filesystem.hpp>

typedef std::unique_ptr<boost::asio::io_service::work> asio_worker;

#if defined(__APPLE__)
void Parse::extra_symbolization(std::map<uint64_t, Parser *> &parsers)
{
    std::cerr << "Symbolize additional events with backtrace parser ... " << std::endl;
    if (parsers.find(BACKTRACE) == parsers.end()) {
        std::cerr << "No backtrace infomation found for " << __func__ << std::endl;
        return;
    }

    BacktraceParser *backtraceparser =
        dynamic_cast<BacktraceParser*>(parsers[BACKTRACE]);
    if (!backtraceparser) {
        std::cerr << "No backtrace parser found for " << __func__ << std::endl;
        return;
    }

    if (parsers.find(INTR) != parsers.end()) {
        IntrParser *intrparser =
            dynamic_cast<IntrParser*>(parsers[INTR]);
        std::map<Parse::key_t, event_list_t> &proc_event_list_map
			= intrparser->get_event_list_map();
		for (auto element : proc_event_list_map)
			intrparser->symbolize_intr_for_proc(backtraceparser, element.first);
	
    }

    if (parsers.find(DISP_EXE) != parsers.end()) {
        DispatchParser *dispparser =
            dynamic_cast<DispatchParser *>(parsers[DISP_EXE]);
        std::map<Parse::key_t, event_list_t> &proc_event_list_map
			= dispparser->get_event_list_map();
	
		for (auto element : proc_event_list_map)
        	dispparser->symbolize_block_for_proc(backtraceparser, element.first);
    }


    if (parsers.find(RL_BOUNDARY) != parsers.end()) {
        RLBoundaryParser *rlboundaryparser =
            dynamic_cast<RLBoundaryParser *>(parsers[RL_BOUNDARY]);
        std::map<Parse::key_t, event_list_t> &proc_event_list_map
			= rlboundaryparser->get_event_list_map();

		for (auto element : proc_event_list_map)
        	rlboundaryparser->symbolize_rlblock_for_proc(backtraceparser, element.first);
    }

    if (parsers.find(BREAKPOINT_TRAP) != parsers.end()) {
        BreakpointTrapParser *hwbrparser =
            dynamic_cast<BreakpointTrapParser *>(parsers[BREAKPOINT_TRAP]);
        hwbrparser->symbolize_hwbrtrap(backtraceparser);
    }

    if (parsers.find(MACH_CALLCREATE) != parsers.end()) {
        TimercallParser *timerparser = 
            dynamic_cast<TimercallParser *>(parsers[MACH_CALLCREATE]);
        std::map<Parse::key_t, event_list_t> &proc_event_list_map
			= timerparser->get_event_list_map();
		for (auto element : proc_event_list_map)
        	timerparser->symbolize_func_ptr_for_proc(backtraceparser, element.first);
    }
}
#endif

std::map<uint64_t, std::list<EventBase *> > Parse::parse(std::map<uint64_t, std::string>& files)
{

    LOG_S(INFO) << "Begin parsing events...";
    boost::asio::io_service ioService;
    boost::thread_group threadpool;
    asio_worker work(new asio_worker::element_type(ioService));

    for (int i = 0 ; i < LoadData::meta_data.nthreads; i++)
        threadpool.create_thread( boost::bind(&boost::asio::io_service::run,
                    &ioService));

    std::map<uint64_t, Parser *> parsers;
    Parser *parser;

	for (auto file : files) {
        parser = nullptr;
        switch(file.first) {
            case MACH_IPC_MSG: 
                parser = new MachmsgParser(file.second);
                break;
            case MACH_IPC_VOUCHER_INFO: 
                parser = new VoucherParser(file.second);
                break;
            case MACH_MK_RUN:
                parser = new MkrunParser(file.second);
                break;
            case INTR:
                parser = new IntrParser(file.second);
                break;
            case MACH_TS:
                parser = new TsmaintainParser(file.second);
                break;
            case MACH_WAIT:
                parser = new WaitParser(file.second);
                break;
            case DISP_ENQ:
            case DISP_EXE:
            case DISP_MIG:
                parser = new DispatchParser(file.second);
                break;
            case MACH_CALLCREATE:
            case MACH_CALLOUT:
            case MACH_CALLCANCEL:
                parser = new TimercallParser(file.second);
                break;
#if defined(__APPLE__)
            case BACKTRACE:
                parser = new BacktraceParser(file.second,
                        LoadData::meta_data.libinfo_file);
                break;
#endif
            case MACH_SYS:
            case BSD_SYS:
                parser = new SyscallParser(file.second);
                break;
            case CA_SET:
            case CA_DISPLAY:
                parser = new CAParser(file.second);
                break;
            case BREAKPOINT_TRAP:
                parser = new BreakpointTrapParser(file.second);
                break;
            case NSAPPEVENT:
                parser = new NSAppEventParser(file.second);
                break;
            case RL_BOUNDARY:
                parser = new RLBoundaryParser(file.second);
                break;
            default:
                break;
        }        

        if (parser) {
            parsers[file.first] = parser;
            ioService.post(boost::bind(&Parser::process, parser));
        } else {
            std::cerr << "Check : fail to begin parser " << file.first << std::endl;
        }
    }
    work.reset();
    threadpool.join_all();
    //ioService.stop();

    extra_symbolization(parsers);

    std::map<uint64_t, std::list<EventBase *>> ret_lists;
    std::list<EventBase *> result;
	ret_lists.clear();
	result.clear();

	for (auto element: parsers) {
		ret_lists[element.first] = element.second->collect();
		ret_lists[0].insert(ret_lists[0].end(), ret_lists[element.first].begin(), ret_lists[element.first].end());
	}
    parsers.clear();

	LOG_S(INFO) << "Finished parsing. Parsed " << std::dec << ret_lists[0].size() << " events in total";
    return ret_lists;
}

std::string Parse::get_prefix(std::string &input_path)
{
    std::string filename;
    size_t dir_pos = input_path.find_last_of("/");
    if (dir_pos != std::string::npos)
        filename = input_path.substr(dir_pos + 1);
    else
        filename = input_path;

    boost::filesystem::path dir("temp");
    if (!(boost::filesystem::exists(dir)))  
        boost::filesystem::create_directory(dir);

    if ((boost::filesystem::exists(dir))) {
        std::string prefix("temp/");
        filename = prefix + filename;
    }

    size_t pos = filename.find(".");
    if (pos != std::string::npos)
        return filename.substr(0, pos);
    else
        return filename;
}

std::map<uint64_t, std::string> Parse::divide_files(std::string filename,
        uint64_t (*hash_func)(uint64_t, std::string))
{
    std::map<uint64_t, std::string> div_files;
    std::ifstream infile(filename);

    if (infile.fail()) {
        std::cerr << "Error: faild to open file: " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    std::map<uint64_t, std::ofstream*> filep;
    std::string line, abstime, deltatime, opname;
    uint64_t arg1, arg2, arg3, arg4, tid, hashval; 

    while (getline(infile, line)) {
        std::istringstream iss(line);
        if (!(iss >> abstime >> deltatime >> opname >> std::hex >> arg1 \
                    >> arg2 >> arg3 >> arg4 >> tid)) {
            break;
        }

        hashval = hash_func(tid, opname);
        // 0 is reserved for irrelated tracing points
        if (hashval == 0) {
            continue;
        }

        if (filep.find(hashval) == filep.end()) {
            filep[hashval] = new std::ofstream(get_prefix(filename)
                    + "." + std::to_string(hashval) + ".tmp");

            if (!filep[hashval] || !*filep[hashval]) {
                std::cerr << "Error: unable to create std::ofstream for ";
                if (filep[hashval])
                    std::cerr << "file ";
                std::cerr << hashval << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        *(filep[hashval]) << line.c_str() << std::endl;        
    }
    infile.close();

    std::map<uint64_t, std::ofstream*>::iterator it;
    for (it = filep.begin(); it != filep.end(); ++it) {
        div_files[it->first] = get_prefix(filename)
            + "." + std::to_string(it->first) + ".tmp";
        (it->second)->close();
        delete it->second;
    }
    return div_files;
}

std::map<uint64_t, std::list<EventBase *> > Parse::divide_and_parse()
{
    std::map<uint64_t, std::string> files = divide_files(LoadData::meta_data.data,
            LoadData::map_op_code);
    std::map<uint64_t, std::list<EventBase*> > lists = parse(files);
	for (auto event: lists[0]) {
		if (event->get_event_type() < 0)
			std::cout << event->get_event_type() << std::endl;
		assert(event->get_event_type() > 0);
	}
    return lists;
}

std::list<EventBase *> Parse::parse_backtrace()
{
    std::map<uint64_t, std::string> files = divide_files(LoadData::meta_data.data, LoadData::map_op_code);
    BacktraceParser parser(files[BACKTRACE], LoadData::meta_data.libinfo_file);
    parser.process();
    return parser.collect();
}
