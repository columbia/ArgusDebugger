#include "msg_pattern.hpp"
#include "parse_helper.hpp"
#include <unordered_map>

#define MSG_PATTERN_DEBUG 0

MsgPattern::MsgPattern(std::list<EventBase*> &list)
:ev_list(list)
{
}

void MsgPattern::read_ipc_patterns(std::string msgInfo)
{
	std::ifstream pFile(msgInfo);
	std::string line;
	size_t pos = std::string::npos;

	std::unordered_map<uint64_t, std::unordered_map<double, EventBase *>> map;
	for (auto event: ev_list) {
		if (map.find(event->get_tid()) == map.end()) {
			std::unordered_map<double, EventBase *> empty_map;
			map[event->get_tid()] = empty_map;
		}
		map[event->get_tid()][event->get_abstime()] = event;
	}
	
	while (getline(pFile, line)) {
		std::vector<MsgEvent *> pattern;
		int size = 0;

		if ((pos = line.find("pattern_size =")) != std::string::npos) {
			size = std::stoi(line.substr(pos + sizeof("pattern_size =")));
			assert(size >= 2 && size <= 4);
			pattern.clear();

			int index = 0;
			uint64_t not_used, tid, pid;
			double timestamp;
			std::string procname, op;

			while (index < size && getline(pFile, line)) {
				std::istringstream  iss(line);
				if (!(iss >> std::hex >> not_used >> timestamp\
					>> std::hex >> tid >> std::dec >> pid\
					>> procname >> op)) {
					LOG_S(INFO) << "fail to read from msg pattern";
					continue;
				}
				
				assert(map.find(tid) != map.end());
				assert((map[tid]).find(timestamp) != map[tid].end());

				EventBase *msg = map[tid][timestamp];
				assert(msg->get_event_type() == MSG_EVENT);
				pattern.push_back(dynamic_cast<MsgEvent*>(msg));
				index++;
			}
			assert(pattern.size() == size);
			//update pattern events
			if (size == 2)
				update_msg_pattern(pattern[0], pattern[1], NULL, NULL);
			if (size == 3)
				update_msg_pattern(pattern[0], pattern[1], pattern[2], NULL);
			if (size == 4)
				update_msg_pattern(pattern[0], pattern[1], pattern[2], pattern[3]);
		}
	}
}

void MsgPattern::collect_patterned_ipcs()
{
	std::string msgInfo("msg_pattern_info");
	std::ifstream pFile(msgInfo);
	
	if (pFile) {
		bool read_from_file = false;
		if (pFile.peek() == std::ifstream::traits_type::eof())
			LOG_S(INFO) << "file " << msgInfo << "is empty";
		else
			read_from_file = true;
		pFile.close();

		if (read_from_file) {
			read_ipc_patterns(msgInfo);
    		std::string output("output/msg_pattern_for_check.log");
    		save_patterned_ipcs(output);
			return;
		}
	}

#ifdef MSG_PATTERN_DEBUG
	LOG_S(INFO) << "begin matching msg... ";
#endif
    patterned_ipcs.clear();
    mig_list.clear();
    msg_list.clear();
	
    MsgEvent *cur_msg;
	for (auto it: ev_list) {
        cur_msg = dynamic_cast<MsgEvent *> (it);
        assert(cur_msg);
        if (cur_msg->is_mig() == true)
            mig_list.push_back(cur_msg);
        else {
            msg_list.push_back(cur_msg);
			MsgHeader *header = cur_msg->get_header();
			assert(header);
			key_t key = {.remote_port = header->get_remote_port(),
					.local_port = header->get_local_port(),
					.is_recv = header->check_recv()};
			msg_map.insert(std::pair<key_t, MsgEvent*>(key, cur_msg));
		}
	}

    std::list<MsgEvent *>::iterator it;
	for (it = msg_list.begin(); it != msg_list.end(); it++) {
		itermap[*it] = it;
	}
	visit_map.clear();
    collect_msg_pattern();

#ifdef MSG_PATTERN_DEBUG
	LOG_S(INFO) << "finish matching msg.";
#endif
}


MsgPattern::~MsgPattern(void)
{
    patterned_ipcs.clear();
}

void MsgPattern::update_msg_pattern(MsgEvent * event0, MsgEvent * event1,
                                    MsgEvent * event2, MsgEvent* event3)
{
    assert(event0 != nullptr && event1 != nullptr);
    event0->set_peer(event1);
    event1->set_peer(event0);
	visit_map[event0] = true;
	visit_map[event1] = true;
	
    if (event2 != nullptr) {
        event1->set_next(event2);
	}

    if (event3 != nullptr) {
        assert(event2 != nullptr);
        event2->set_peer(event3);
        event3->set_peer(event2);
		visit_map[event2] = true;
		visit_map[event3] = true;
    }
    
    msg_episode s;
    s.insert(event0);
    s.insert(event1);
    if (event2 != nullptr) {
        s.insert(event2);
    }
    if (event3 != nullptr) {
        s.insert(event3);
    } 
    patterned_ipcs.push_back(s);
}

void MsgPattern::collect_mig_pattern(void)
{
    std::list<MsgEvent *>::iterator it;
    MsgEvent *cur_msg, *mig_req;
    MsgHeader *curr_header, *mig_req_h;
    
    if (mig_list.front()->get_header()->check_recv() == true)
        mig_list.pop_front();
    if (mig_list.back()->get_header()->check_recv() == false)
        mig_list.pop_back();

    while (!mig_list.empty()) {
        mig_req = mig_list.front();
        mig_list.pop_front();

        if (mig_req->is_freed_before_deliver() == true)
            continue;
       
        mig_req_h = mig_req->get_header();
        for (it = mig_list.begin(); it != mig_list.end(); it++) {
            cur_msg  = *it;
            curr_header = cur_msg->get_header();
            if (curr_header->get_remote_port() ==  mig_req_h->get_local_port()
                && curr_header->get_rport_name() == mig_req_h->get_lport_name()
                && mig_req->get_tid() == cur_msg->get_tid()) {
                update_msg_pattern(mig_req, cur_msg, nullptr, nullptr);
                break;
             }
        }
        it = mig_list.erase(it);
    }
}

void MsgPattern::collect_msg_pattern(void)
{
    MsgEvent *cur_msg;
    MsgHeader *cur_header;
	for (auto it = msg_list.begin(); it != msg_list.end(); it++) {
        cur_msg = *(it);
        assert(cur_msg);
		if (visit_map.find(cur_msg) != visit_map.end())
			continue;

        cur_header = cur_msg->get_header();
        assert(cur_header);

        if (cur_msg->is_freed_before_deliver() == true
            || cur_header->check_recv() == true) {
            continue;
        }

        if (cur_header->get_remote_port() != 0 && cur_header->get_local_port() != 0) {
            std::list<MsgEvent *>::iterator req_send_offset = it;
            std::list<MsgEvent *>::iterator req_recv_offset;
            std::list<MsgEvent *>::iterator reply_send_offset;
            std::list<MsgEvent *>::iterator reply_recv_offset;
            
            uint32_t reply_sender_port_name = 0;
            uint32_t reply_recver_port_name = cur_header->get_lport_name();
            pid_t request_pid = cur_msg->get_pid();
            pid_t reply_pid = -1;
        
            req_recv_offset = search_ipc_msg(
                                &reply_sender_port_name,
                                &reply_pid,
                                cur_header->get_remote_port(),
                                cur_header->get_local_port(),
                                true,
                                req_send_offset,
                                reply_recver_port_name);

            if (req_recv_offset == msg_list.end()) {
				visit_map[*it] = true;
                continue;
            }

            reply_send_offset = search_ipc_msg(
                                &reply_sender_port_name,
                                &reply_pid,
                                cur_header->get_local_port(),
                                0,
                                false,
                                req_recv_offset,
                                reply_recver_port_name);
            if (reply_send_offset == msg_list.end()) {
                update_msg_pattern(*it, *req_recv_offset, nullptr, nullptr);
                assert(req_recv_offset != msg_list.end() && *req_recv_offset);
                continue;
            }

            reply_recv_offset = search_ipc_msg(
                                &reply_recver_port_name,
                                &request_pid,
                                cur_header->get_local_port(),
                                0,
                                true,
                                reply_send_offset,
                                reply_recver_port_name);
            if (reply_recv_offset == msg_list.end()) { 
                update_msg_pattern(*it, *req_recv_offset, nullptr, nullptr);
                assert(req_recv_offset != msg_list.end() && *req_recv_offset);
                continue;
            }

            update_msg_pattern(*it, *req_recv_offset, *reply_send_offset, *reply_recv_offset);
        }

        else if (cur_header->get_remote_port() != 0 && cur_header->get_local_port() == 0) {
            std::list<MsgEvent *>::iterator req_send_offset = it;
            std::list<MsgEvent *>::iterator req_recv_offset;
            uint32_t reply_sender_port_name = 0;
            pid_t reply_pid = -1;

            req_recv_offset = search_ipc_msg(
                                &reply_sender_port_name,
                                &reply_pid,
                                cur_header->get_remote_port(),
                                cur_header->get_local_port(),
                                true,
                                req_send_offset,
                                0);
            if (req_recv_offset == msg_list.end()) {
                assert(*it);
				visit_map[*it] = true;
                continue;
            }
            update_msg_pattern(*it, *req_recv_offset, nullptr, nullptr);
            assert(*req_recv_offset);
        }
        else {
            assert(*it);
			visit_map[*it] = true;
        }
    }
    std::string output("output/msg_pattern.log");
    save_patterned_ipcs(output);
}


/*
 * parameters for function search_ipc_msg
 * RECV1
 * (out)port_name : get reply port name in receiver side(local port from sender)
 * (in)remote_port : remote port kaddr
 * (in)local_port : local port kaddr
 * (in)begin_it : search begins from the next iterator
 * (out)i : return the result ipc msg_event pos in the list
 * REPLY (RECV/SEND):
 * (in)port_name : reply port name in reply-sender or reply-receiver(remote port in the reply)
 */
std::list<MsgEvent *>::iterator MsgPattern::search_ipc_msg(
            uint32_t *port_name, 
            pid_t *pid,
            uint64_t remote_port,
			uint64_t local_port,
            bool is_recv,
            std::list<MsgEvent *>::iterator begin_it,
            uint32_t reply_recver_port_name)
{

	key_t key = {.remote_port = remote_port,
					.local_port = local_port,
					.is_recv = is_recv};

	std::list<MsgEvent*> matched_list;
	matched_list.clear();
	auto range = msg_map.equal_range(key);
	for (auto it = range.first; it != range.second;) {
		MsgEvent *msg = (*it).second;
		if (visit_map.find(msg) != visit_map.end()) {
			it = msg_map.erase(it);
		} else {
			matched_list.push_back(msg);
			it++;
		}
	}

    matched_list.sort(Parse::EventComparator::compare_time);
	
	for (auto cur_ipc: matched_list) {
        assert(cur_ipc);
        MsgHeader *header = cur_ipc->get_header();
        assert(header);
		/*checking processes*/
		if (*pid == -1) {
			*pid = cur_ipc->get_pid();
		} else {
			if (*pid != cur_ipc->get_pid()) {
				continue;
			}
		}
		/*checking port names*/
		if (*port_name == 0) {
			assert(is_recv == true);
			*port_name = header->get_lport_name();
		} else {
			if (is_recv == false && header->is_from_kernel()) {
				if (header->get_rport_name() != reply_recver_port_name) {
					continue;
				}
			} else { //recv == true || not from kernel
				if (*port_name != header->get_rport_name()) {
					continue;
				}
			}
		}
		/*check the timing information*/
		if (cur_ipc->get_abstime() < (*begin_it)->get_abstime())
			continue; 

		/*meet requirment*/
		return itermap[cur_ipc];
    }
    return msg_list.end();
}

std::list<msg_episode> &MsgPattern::get_patterned_ipcs(void)
{
    return patterned_ipcs;
}

std::list<msg_episode>::iterator MsgPattern::episode_of(MsgEvent *msg_event)
{
    if (msg_event == nullptr)
        return patterned_ipcs.end();

    std::list<msg_episode>::iterator it;
    for (it = patterned_ipcs.begin(); it != patterned_ipcs.end(); it++) {
        if ((*it).find(msg_event) != (*it).end())
            return it;
    }
    return it;
}

std::vector<MsgEvent *> MsgPattern::sort_msg_episode(msg_episode & s)
{
    std::vector<MsgEvent*> episode;
    episode.clear();
    std::set<MsgEvent*>::iterator s_it;
    for (s_it = s.begin(); s_it != s.end(); s_it++) {
        episode.push_back(*s_it);
    }
    sort(episode.begin(), episode.end(), Parse::EventComparator::compare_time);
    return episode;
}

void MsgPattern::save_patterned_ipcs(std::string &output_path)
{
    std::ofstream output(output_path, std::ofstream::out);
    std::list<msg_episode>::iterator it;
    int i = 0;
    for (it = patterned_ipcs.begin(); it != patterned_ipcs.end(); it++, i++) {
        output << i <<" pattern_size = " << (*it).size() << std::endl;
        std::vector<MsgEvent *> cur_pattern = sort_msg_episode(*it);
		for (auto event: cur_pattern) {
			event->streamout_event(output);
		}
    }
    output << "mach msg store done" << std::endl;
    output.close();
}
////////////////
void MsgPattern::decode_patterned_ipcs(std::string &output_path)
{
    std::ofstream output(output_path, std::ofstream::out);
    std::list<msg_episode>::iterator it;
    int i = 0;
    output << "checking mach msg pattern ... " << std::endl;
    output << "number of patterns " << patterned_ipcs.size() << std::endl;
    for (it = patterned_ipcs.begin(); it != patterned_ipcs.end(); it++, i++) {
        output << "## " << i <<" pattern_size = " << (*it).size() << std::endl;
        std::vector<MsgEvent *> cur = sort_msg_episode(*it);
        std::vector<MsgEvent *>::iterator s_it;
        for (s_it = cur.begin(); s_it != cur.end(); s_it++) {
            (*s_it)->decode_event(0, output);
        }
    }
    output << "mach msg checking done" << std::endl;
    output.close();
}

void MsgPattern::verify_msg_pattern()
{
    
}
