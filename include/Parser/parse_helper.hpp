#ifndef PARSE_HELPER_HPP
#define PARSE_HELPER_HPP

#include "base.hpp"

namespace Parse
{
    inline uint32_t lo(uint64_t arg) {return (uint32_t)arg;}
    inline uint32_t hi(uint64_t arg) {return (uint32_t)(arg >> 32);}
    
    namespace EventComparator
    {
        bool compare_time(EventBase * event_1, EventBase * event_2);
    }

    namespace LittleEndDecoder
    {
        void decode32(uint32_t word, char *info, int* index, char pad = '.'); 
        void decode64(uint64_t word, char *info, int *index, char pad = '.');
    }

    namespace QueueOps
    {
        uint64_t dequeue64(std::queue<uint64_t> &my_queue);
        void clear_queue(std::queue<uint64_t> &my_queue);
        void triple_enqueue(std::queue<uint64_t> &my_queue, uint64_t arg1, uint64_t arg2, uint64_t arg3);
        void queue2buffer(void *my_buffer, std::queue<uint64_t> &my_queue);
        std::string queue_to_string(std::queue<uint64_t> &my_queue);
        std::string array_to_string(uint64_t *array, int size);
    }
    
    std::map<uint64_t, std::list<EventBase *>> parse(std::map<uint64_t, std::string> &files);
    std::string get_prefix(std::string &input_path);
    std::map<uint64_t, std::string> divide_files(std::string filename,
        uint64_t (*hash_func)(uint64_t, std::string));
    std::map<uint64_t, std::list<EventBase *>> divide_and_parse();
    std::list<EventBase *> parse_backtrace();
}
#endif
