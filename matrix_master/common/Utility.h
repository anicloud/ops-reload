#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_COMMON_UTILITY_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_COMMON_UTILITY_H__

#include <list>
#include <map>
#include <stdint.h>
#include <sys/types.h>

namespace matrix {

extern int get_process_name_from_pid(const pid_t pid, std::string* process_name);

extern void get_process_names(const std::list<pid_t>& pids, 
                              std::list<std::string>* processes);

extern void get_port_and_fd_list(std::map<int32_t, uint64_t>* port_fd_list);

extern void get_socket_fd(const std::list<pid_t>& pids, std::list<uint64_t>* fds);

extern bool get_uid_gid_of_specified_user(const std::string& user, 
                                          uid_t* const uid, 
                                          gid_t* const gid);

extern bool string_check_base(const std::string& str);
extern bool string_check_url(const std::string& str);
extern bool string_check_path(const std::string& str);
extern bool string_check_abspath(const std::string& str);
extern bool string_check_version(const std::string& str);
extern bool string_check_name(const std::string& str);

extern std::string time_to_str(struct timeval& val);

/*
 * get time escaped in ms since 1970.1.1
 */
extern int64_t get_current_timestamp_ms();

} /// namespace matrix

#endif ///__INF_COMPUTING_MATRIX_MATRIX_AGENT_COMMON_UTILITY_H__
