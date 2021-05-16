#include <iostream>
#include <iomanip>
#include <sstream>
#include <dirent.h>

#include "argus_loop.h"

#if defined(__APPLE__)
#include <Python.h>
#endif

ArgusLoop::~ArgusLoop()
{

    if (trans_graph != nullptr) {
        delete trans_graph;
        trans_graph = nullptr;
    }

    if (event_graph != nullptr) {
        delete event_graph;
        event_graph = nullptr;
    }

    if (bsearch != nullptr) {
        delete bsearch;
        bsearch = nullptr;
    }
}

void ArgusLoop::register_debugger_commands()
{
	ArgusLoop &commands = *this;
	CompositeCommand *debugger = new CompositeCommand("debugger", "interactive debugging");
	this->add(debugger);

    debugger->add("beam-diagnose", [&](Arguments args) {
        Graph *graph = nullptr;
        args.shouldHave(3);
        switch (args.get(0).c_str()[0]) {
            case '0' : graph = commands.event_graph;
                break;
            case '1' : graph = commands.trans_graph;
                break;
            default: return;
        }
		int beam_width = args.get_num(1, "dec");
		int lookback = args.get_num(2, "dec");
        if (graph == nullptr) {
            std::cout << "No graph avialable" << std::endl;
            return;
        }
		if (commands.bsearch != nullptr) {
			delete commands.bsearch;
			commands.bsearch = nullptr;
		}
		LOG_S(INFO) << "Beamwidth " << std::dec << beam_width\
			<< " lookbacksteps" << std::dec << lookback;
		commands.bsearch = new BeamSearcher(graph, beam_width, lookback);
		commands.bsearch->init_diagnose();
    }, "Automatic diagnosis with beam search");
}

void ArgusLoop::register_query_commands()
{
	ArgusLoop &commands = *this;
	CompositeCommand *view = new CompositeCommand("view", "check current diagnositics");
	this->add(view);
    view->add("graphs", [&](Arguments args) {
			if (commands.event_graph)
				std::cout << "0\tevent_graph exists\n";
			if (commands.trans_graph)
				std::cout << "1\ttrans_graph exists\n";
        }, "list in-memory graphs");

    view->add("vertex", [&](Arguments args) {
            args.shouldHave(1); 
            if (commands.event_graph == nullptr)
				return;
			uint64_t gid = args.get_num(0, "hex");
            std::cout << "Vertex 0x" << std::hex << gid << ":" << std::endl;
            Node *node = commands.event_graph->id_to_node(gid);
            if (node != nullptr) {
                node->get_group()->pic_group(std::cout);
                node->get_group()->streamout_group(std::cout);
            }
        }, "Rendering vertex information");
	
	view->add("event-at", [&](Arguments args) {
			if (commands.event_graph == nullptr)
				return;
			args.shouldHave(2);
			uint64_t gid = args.get_num(0, "hex");
			uint64_t index = args.get_num(1, "hex");
			std::cout << "show-event 0x"\
				<< std::hex << gid\
				<< " at ["\
				<< std::hex << index << "]" << std::endl;
			assert(commands.event_graph);
            Node *node = commands.event_graph->id_to_node(gid);
            if (node == nullptr) {
				std::cout << "Error: no gid found" << std::endl;
				return;
            }
			node->show_event_detail(index, std::cout);
		}, "Rendering even information in (gid, index_to_event)");
}

void ArgusLoop::register_graph_commands()
{
	ArgusLoop &commands = *this;
	CompositeCommand *config = new CompositeCommand("set", "config with system info");
	this->add(config);
	CompositeCommand *construct = new CompositeCommand("construct", "construct graphs");
	this->add(construct);
	CompositeCommand *save = new CompositeCommand("save", "save result to file");
	this->add(save);

    config->add("host-proc", [&](Arguments args) {
            args.shouldHave(1);
            std::string appname = args.get(0);
            LoadData::meta_data.host = appname;
            LoadData::symbolic_procs[appname] = 1;
        }, "Inspect the performance issue for the process");

    config->add("live-pid", [&](Arguments args) {
            uint64_t set_pid;
            args.shouldHave(1);
            if (args.asDec(0, &set_pid))
                LoadData::meta_data.pid = (pid_t) set_pid;
        }, "Set any live GUI app pid for correcting lldb-symbolication for CoreGraphics");

    config->add("symbolize-proc", [&](Arguments args) {
            args.shouldHave(1);
            std::string procname = args.get(0);
            LoadData::symbolic_procs[procname] = 1;
        }, "Request callstack event symbolication for a process");

    construct->add("event-graph", [&](Arguments args) {
            args.shouldHave(1);
            std::string logfile = args.get(0);
            LoadData::meta_data.data = logfile;
			
			LOG_S(INFO) << "host : " << LoadData::meta_data.host;
			if (access(logfile.c_str(), R_OK) != 0)
            	return;
            if (commands.event_graph != nullptr) {
                delete(commands.event_graph);
                commands.event_graph = nullptr;
            }
            commands.event_graph = new EventGraph();
       }, "construct event graph with log file");

    construct->add("trans-graph", [&](Arguments args) {
            args.shouldHave(1);
			uint64_t gid = args.get_num(0, "hex");

            if (!commands.event_graph 
                || commands.event_graph->id_to_node(gid) == nullptr) {
            	std::cout << "no event graph" << std::endl;
            	return;
            }
            if (commands.trans_graph) {
                delete commands.trans_graph;
                commands.trans_graph = nullptr;
            }
            commands.trans_graph = new TransactionGraph(commands.event_graph->get_groups_ptr(), gid);
        }, "construct transaction graph with root node id");

	save->add("graph", [&](Arguments args) {
            args.shouldHave(2);
            std::string logpath = args.get(1);
            Graph *graph = nullptr;
            switch (args.get(0).c_str()[0]) {
                case '0' : graph = commands.event_graph;
                            break;
                case '1' : graph = commands.trans_graph;
                            break;
                default: return;
            }
            if (graph == nullptr)
                return;
            graph->streamout_nodes_and_edges(logpath);
		}, "save graph to file [graph-type] [log-path]");
    
}

std::string ArgusLoop::readline(const std::string &prompt)
{
    std::string line;
    std::cout << prompt;
    std::cout.flush();
    std::getline(std::cin, line);
    return line;
}

static void printUsageHelper(Command *command, int level);

static void printUsage(ArgusLoop *command) {
    std::cout << "usage:\n";
    for(auto c : command->getMap()) {
        printUsageHelper(c.second, 1);
    }
}

static void printUsageHelper(Command *command, int level) {
    for(int i = 0; i < level; i ++) std::cout << "    ";

    std::cout << std::left << std::setw(30) << command->getName()
        << " " << command->getDescription() << std::endl;

    if(auto composite = dynamic_cast<CompositeCommand *>(command)) {
        for(auto c : composite->getMap()) {
            printUsageHelper(c.second, level + 1);
        }
    }
}

#if defined(__APPLE__)
void ArgusLoop:: invoke_lldb_script()
{
	Py_Initialize();
	Py_Finalize();
}
#endif

void mainloop(ArgusLoop &commands)
{
	bool running = true;
	//ArgusLoop commands;
	commands.add("exit", [&](Arguments) {running = false;}, "exit");
	commands.add("help", [&](Arguments) {printUsage(&commands); },"print help info");
	commands.add("ls", [&](Arguments args) {
		args.shouldHave(1); 
		std::string dir_path = args.get(0);
		DIR *dir;
		struct dirent *ent;
		if ((dir = opendir (dir_path.c_str())) != NULL) {
			while ((ent = readdir (dir)) != NULL)
				std::cout << ent->d_name <<"\t";
			std::cout << std::endl;
			closedir (dir);
		} else {
			std::cout << dir_path << " : path doesn't exist" << std::endl;
		}
	}, "show files in directory");

	commands.register_graph_commands();
	commands.register_debugger_commands();
	commands.register_query_commands();

    while (running) {
        std::string line = commands.readline("argus> ");
        std::istringstream sstream(line);
        Arguments args;
        std::string arg;
        std::string comp_arg;
        while (sstream >> arg) {
            if (arg.back() != '\\') {
                comp_arg += arg;
                args.add(comp_arg);
                comp_arg.clear();
            } else {
                arg.back() = ' ';
                comp_arg += arg;
            }
        }
        if (comp_arg.size() > 0)
            args.add(comp_arg);

        if(args.size() > 0) {
            try {
                commands(args);
            }
            catch(const char *s) {
                std::cout << "error: " << s << std::endl;
            }
            catch(const std::string &s) {
                std::cout << "error: " << s << std::endl;
            }
        }
    }
	std::cout << "Exit argus_shell" << std::endl;
}

