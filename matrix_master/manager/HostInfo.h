#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_HOST_INFO_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_HOST_INFO_H__

#include <string>
#include <stdexcept>

#include <sys/types.h>

#include "op/oped/noah/sailor/utility/thread/Mutex.h"
#include "inf/computing/matrix/matrix-agent/json-cpp/include/json/json.h"

namespace matrix {

class ResourceTuple
{
public:
    ResourceTuple();
    ResourceTuple(int64_t cpu_num, int64_t memory_mb, int64_t disk_total_mb);
public:
    bool is_valid() const;
    bool is_empty() const;
    bool from_json(const Json::Value& t);
    const Json::Value to_json() const;
public:
    ResourceTuple operator+(const ResourceTuple& another) const;
    ResourceTuple operator-(const ResourceTuple& another) const;
    bool operator>(const ResourceTuple& another) const;
    bool operator>=(const ResourceTuple& another) const;
    bool operator==(const ResourceTuple& another) const;
public:
    int64_t _cpu_num;
    int64_t _memory_mb;
    int64_t _disk_total_mb;
};
    
class HostInfo
{
public:
    struct HostState
    {
        enum type {
            INVALID = 0,       // not inited
            AVAILABLE = 10,    // 
            FAILED = 20        // host has some error
        };

        static std::string to_string(type t) {
            switch(t) {
            case   INVALID: return "INVALID";
            case AVAILABLE: return "AVAILABLE";
            case    FAILED: return "FAILED";
            default: throw std::runtime_error("Invalid HostState");
            }
        }
    };
	
public:

    HostInfo();
    HostInfo(const HostInfo& other);
    ~HostInfo();
    
    HostInfo& operator=(const HostInfo& from);
    bool is_ready() const;
    Json::Value to_json() const;

public:
    bool has_enough_resource(const ResourceTuple& resource) const;
    bool use_resource(const ResourceTuple& resource);
    bool release_resource(const ResourceTuple& resource);
    bool init_used_resource();
    
    HostState::type get_host_state() const;
    const ResourceTuple& get_total_resource() const;
    
    void set_host_state(HostState::type state);
    void set_total_resource(const ResourceTuple& resource);
    void set_reserved_resource(const ResourceTuple& resource);

private:
    HostState::type _state;                // 主机状态
    ResourceTuple _total_resource;         // 总资源
	ResourceTuple _reserved_resource;      // 保留资源
	ResourceTuple _used_resource;          // 已使用资源
    
    mutable ::sailor::Mutex _mutex;
};

}

#endif


