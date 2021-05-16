#ifndef TSMAINTENANCE_HPP
#define TSMAINTENANCE_HPP
 class TimeshareMaintenanceEvent : public EventBase {
     void *preempted_group_ptr;
 public:
     TimeshareMaintenanceEvent(double abstime, std::string op, uint64_t _tid, uint32_t event_core, std::string proc = "");
     void save_gptr(void * g_ptr) {preempted_group_ptr = g_ptr;}
     void *load_gptr(void); 
     void decode_event(bool is_verbose, std::ofstream &outfile);
     void streamout_event(std::ofstream &outfile);
}; 
#endif
