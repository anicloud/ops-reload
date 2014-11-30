#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_INSTANCE_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_INSTANCE_H__

#include <stdint.h>
#include <sys/types.h>

#include "op/oped/noah/sailor/utility/thread/Mutex.h"

#include "inf/computing/matrix/matrix-agent/manager/InstanceInfo.h"

namespace matrix {

class Instance
{
public:
    struct Events {
        enum type {
            HEALTH_OK     ,
            HEALTH_START  ,
            HEALTH_FAIL   ,
            HEALTH_TIMEO  ,
            INSTALL_OK    ,
            INSTALL_FAIL  ,
            INSTALL_TIMEO ,
            REMOVE_OK     ,
            REMOVE_FAIL   ,
            REMOVE_TIMEO  ,
            INSTALL       ,
            REMOVE        ,
        };
        static const std::string to_string(const type t) {
            switch(t) {
                case HEALTH_OK    : return "HEALTH_OK";
                case HEALTH_START : return "HEALTH_START";
                case HEALTH_FAIL  : return "HEALTH_FAIL";
                case HEALTH_TIMEO : return "HEALTH_TIMEO";
                case INSTALL_OK   : return "INSTALL_OK";
                case INSTALL_FAIL : return "INSTALL_FAIL";
                case INSTALL_TIMEO: return "INSTALL_TIMEO";
                case REMOVE_OK    : return "REMOVE_OK";
                case REMOVE_FAIL  : return "REMOVE_FAIL";
                case REMOVE_TIMEO : return "REMOVE_TIMEO";
                case INSTALL      : return "INSTALL";
                case REMOVE       : return "REMOVE";
                default          : throw std::runtime_error("Invalid Events");
            }
        }
    };

public:
    Instance(uint64_t generation, std::string service_name, int32_t offset);
    explicit Instance(std::string meta_file);
    virtual ~Instance();
    
    bool do_install(const InstanceMeta& instance_meta);
    bool do_update(const InstanceMeta& instance_meta);
    bool do_remove();
    
    void on_health_check(InstanceInfo::HealthCheckStatus::type type);
    void on_install(InstanceInfo::ErrorCode::type type);
    void on_remove(InstanceInfo::ErrorCode::type type);
    
    bool check_timeo();
    bool reset_to_running();
    
    bool from_string(const std::string& content);
    std::string to_string() const;

public:
    const InstanceInfo& get_instance_info() const;
    uint64_t get_generation() const;

private:
    bool do_event(Events::type ev);
    
    inline void set_timeout();
    inline void clear_timeout();
    
    bool from_json(const Json::Value& root);
    const Json::Value to_json() const;
    bool save_to_file() const;
    bool remove_meta_file() const;
    
private:
    struct Translation
    {
        const InstanceInfo::InstanceState::type  from;
        const Events::type                       ev;
        const InstanceInfo::InstanceState::type  to;
    };
    static const Translation _state_trans_table[];
    
    uint64_t _generation;
    InstanceInfo _instance_info;
    
    int32_t _event_start_timestamp;
    int32_t _event_timeout_sec;
    
    ::sailor::Mutex _mutex;
};  // class Instance

}   // namespace matrix

#endif   // __INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_INSTANCE_H__
