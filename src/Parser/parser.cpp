#include "parser.hpp"

Parse::Parser::Parser(std::string _filename)
{
    filename = _filename;
    infile.open(filename);
    if (infile.fail()) {
        std::cerr << "Error: fail to read file " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    outfile.open(filename+".remain");
    if (outfile.fail()) {
        std::cerr << "Error: fail to write file " << filename + ".remain" << std::endl;
        infile.close();
        exit(EXIT_FAILURE);
    }
}

Parse::Parser::~Parser()
{
    long pos = outfile.tellp();
    if (infile.is_open())
        infile.close();
    if (outfile.is_open())
        outfile.close();
    local_event_list.clear();
    if (pos == 0) {
        remove(filename.c_str());
        remove((filename + ".remain").c_str());
    } else {
        std::cerr << "Warning : file " << filename << " is not parsed completely" << std::endl;
    }
}

void Parse::Parser::add_to_proc_map(Parse::key_t proc, EventBase *event)
{
	if (proc.first == -1 && proc.second == "")
		return;
	
	if (proc_event_list_map.find(proc) == proc_event_list_map.end()) {
		event_list_t empty;
		empty.clear();
		proc_event_list_map[proc] = empty;
	}
	proc_event_list_map[proc].push_back(event);
}

Parse::event_list_t Parse::Parser::get_events_for_proc(Parse::key_t proc)
{
    return proc_event_list_map[proc];
}
