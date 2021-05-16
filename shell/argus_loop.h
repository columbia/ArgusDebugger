#ifndef ARGUS_LOOP_HPP
#define ARGUS_LOOP_HPP
#include <iostream>
#include <unistd.h>
#include <boost/filesystem.hpp>

//#include "parser.hpp"
//#include "group.hpp"
//#include "canonization.hpp"

#include "command.h"
#include "graph.hpp"
#include "beam_search.hpp"

class ArgusLoop : public CompositeCommand {
public:
    EventGraph *event_graph;
    TransactionGraph *trans_graph;
    BeamSearcher *bsearch;

    ArgusLoop() : CompositeCommand("", ""),
                event_graph(nullptr),
                trans_graph(nullptr),
                bsearch(nullptr){}
	void register_debugger_commands();
	void register_query_commands();
	void register_graph_commands();
    ~ArgusLoop();
	virtual void invokeNull(Arguments args) {}
	void invokeDefault(Arguments args) {
		std::cout << "unknown command, try \"help\"\n";
	}
	void invoke_lldb_script();
	std::string readline(const std::string &prompt);
};

void mainloop(ArgusLoop &commands);

#endif
