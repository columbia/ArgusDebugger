#include "parser.hpp"
#include "mkrunnable.hpp"

Parse::MkrunParser::MkrunParser(std::string filename)
:Parser(filename)
{
	mkrun_events.clear();
}

bool Parse::MkrunParser::set_info(MakeRunEvent *mr_event, uint64_t peer_prio,
		uint64_t wait_result, uint64_t run_info)
{
	/*
	   mr_event->set_peer_prio(peer_prio);
	   mr_event->set_peer_wait_result(wait_result);
	   mr_event->set_peer_run_count(lo(run_info));
	   mr_event->set_peer_ready_for_runq(hi(run_info));
	 */
	return true;
}

bool Parse::MkrunParser::process_finish_wakeup(double abstime, std::istringstream &iss)
{
	std::string procname;
	uint64_t peer_tid, peer_prio, wait_result, run_info, tid, coreid;
	if (!(iss >> std::hex >> peer_tid >> peer_prio >> wait_result >> run_info \
				>> tid >> coreid))
		return false;

	if (!getline(iss >> std::ws, procname) || !procname.size())
		procname = "";

	if (mkrun_events.find(tid) == mkrun_events.end()) {
		outfile << "Error: fail to matching wake up at " << std::fixed << std::setprecision(1) << abstime << std::endl;
		return false;
	}
	MakeRunEvent * mr_event = mkrun_events[tid];

	if (peer_tid != mr_event->get_peer_tid()) {
		outfile << "Error : try to maching wake up that begins at ";
		outfile << std::fixed << std::setprecision(1) << mr_event->get_abstime();
		outfile << " in different threads ";
		outfile << std::hex << peer_tid << " " << std::hex << mr_event->get_peer_tid() << std::endl;
		return false;
	}

	set_info(mr_event, peer_prio, wait_result, run_info);
	mr_event->override_timestamp(abstime);
	mr_event->set_complete();
	mkrun_events.erase(tid);
	return true;
}

bool Parse::MkrunParser::process_new_wakeup(double abstime, std::istringstream &iss)
{
	std::string procname;
	uint64_t peer_tid, wake_event, wakeup_type, pids, tid, coreid;
	if (!(iss >> std::hex >> peer_tid >> wake_event >> wakeup_type >> pids >> tid >> coreid))
		return false;    

	if (!getline(iss >> std::ws, procname) || !procname.size())
		procname = "";

	MakeRunEvent *new_mr = new MakeRunEvent(abstime, "Wakeup", tid, peer_tid,
			wake_event, wakeup_type, lo(pids), hi(pids), coreid, procname);

	if (!new_mr) {
		LOG_S(INFO) << "OOM " << __func__ << std::endl;
		exit(EXIT_FAILURE);
	}
	mkrun_events[tid] = new_mr;
	local_event_list.push_back(new_mr);
	return true;
}

void Parse::MkrunParser::process()
{
	std::string line, deltatime, opname;
	double abstime;
	//uint64_t peer_tid;
	bool ret = false;

	while(getline(infile, line)) {
		std::istringstream iss(line);
		ret = false;
		if (!(iss >> abstime >> deltatime >> opname)) {
			outfile << line << std::endl;
			continue;
		}

		if (opname == "ARGUS_WAKEUP_REASON")
			ret = process_new_wakeup(abstime, iss);
		else
			ret = process_finish_wakeup(abstime, iss);

		if (ret == false)
			outfile << line << std::endl;
	}
	// check mkrun_events
}
