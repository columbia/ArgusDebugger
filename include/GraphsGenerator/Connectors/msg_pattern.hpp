#ifndef MSG_PATTERN_HPP
#define MSG_PATTERN_HPP

#include "mach_msg.hpp"

#include <functional>
#include <map>

typedef std::set<MsgEvent *> msg_episode;
typedef std::vector<MsgEvent *> msg_episode_v;

class MsgPattern {
    std::list<EventBase *> &ev_list;
    std::list<MsgEvent *> mig_list;
    std::list<MsgEvent *> msg_list;

	std::unordered_map<MsgEvent *, std::list<MsgEvent *>::iterator> itermap;
	std::unordered_map<MsgEvent *, bool> visit_map;

	struct key_t{
		uint64_t remote_port;
		uint64_t local_port;
		bool is_recv;

		bool operator == (const key_t &k) const
		{
			return remote_port == k.remote_port
				&& local_port == k.local_port
				&& is_recv == k.is_recv;
		}
		
		bool operator < (const key_t &k) const
		{
			if (*this  == k) return false;

			if (remote_port != k.remote_port)
				return remote_port < k.remote_port;

			if (local_port != k.local_port)
				return local_port < k.local_port;
						
			return is_recv < k.is_recv;
		}
	
		bool operator > (const key_t &k) const
		{
			return !(*this == k) && !(*this < k);
		}
	};

	struct KeyHasher
	{
		std::size_t operator()(const key_t& k) const
		{
			return ((std::hash<uint64_t>{}(k.remote_port)
						^ (std::hash<uint64_t>{}(k.local_port) << 1)) >> 1)
				^ (std::hash<int>{}(k.is_recv) << 1);
		}
	};

	std::unordered_multimap<key_t, MsgEvent *, KeyHasher> msg_map;

    std::list<msg_episode> patterned_ipcs;
    std::list<MsgEvent*>::iterator search_ipc_msg(
                uint32_t *, pid_t *, uint64_t,
                uint64_t, bool, 
                std::list<MsgEvent *>::iterator, uint32_t);

    void update_msg_pattern(MsgEvent *, MsgEvent *, MsgEvent *, MsgEvent *);
    std::list<msg_episode>::iterator episode_of(MsgEvent*);
    std::vector<MsgEvent *> sort_msg_episode(msg_episode & s);
	void read_ipc_patterns(std::string file);
    
public:
    MsgPattern(std::list<EventBase *> &event_list);
    ~MsgPattern(void);
    void collect_mig_pattern();
    void collect_msg_pattern();
    void collect_patterned_ipcs(void);
    std::list<msg_episode> &get_patterned_ipcs(void);
    void save_patterned_ipcs(std::string &output_path);

    void verify_msg_pattern(void);
    void decode_patterned_ipcs(std::string &output_path);
};

typedef MsgPattern msgpattern_t;
#endif
