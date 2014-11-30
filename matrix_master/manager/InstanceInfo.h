#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_INSTANCE_INFO_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_INSTANCE_INFO_H__

#include <string>
#include <map>
#include "inf/computing/matrix/matrix-agent/manager/HostInfo.h"

namespace matrix
{
class InstanceMeta
{
public:
    struct PackageType {
        enum type {
            ARCHER = 0,
            IMCI   = 1,
        };
        static std::string to_string(type t) {
            switch(t) {
                case ARCHER : return "ARCHER";
                case IMCI   : return "IMCI";
                default     : throw std::runtime_error("Invalid PackageType.");
            }
        }
    };
public:
    InstanceMeta();
    virtual ~InstanceMeta();

public:
    bool is_valid() const;
    
    void set_empty();
    
    bool from_json(const Json::Value& t);
    Json::Value to_json() const;
    
public:
    std::string _meta_version;
    std::string _package_source;
    std::string _package_version;
    PackageType::type _package_type; 
    std::string _deploy_dir;
    std::map<std::string, int32_t> _port;
    int32_t _max_port_include;
    int32_t _min_port_include;
    std::map<std::string, std::string> _process_name;
    std::map<std::string, std::string> _tag;
    ResourceTuple _resource;
    int32_t _deploy_timeout_sec;
    int32_t _health_check_timeout_sec;
    std::string _group;
    
private:
    inline static bool map_from_json(const Json::Value& t, std::map<std::string, int32_t>* map);
    inline static bool map_from_json(const Json::Value& t, std::map<std::string, std::string>* map);
    
private:
    static const size_t MAX_META_VERSION_LENGTH = 256;
    static const size_t MAX_PACKAGE_SOURCE_LENGTH = 4096;
    static const size_t MAX_PACKAGE_VERSION_LENGTH = 4096;
    static const size_t MAX_DEPLOY_DIR_LENGTH = 4096;
    static const size_t MAX_PORT_K_LENGTH = 4096;
    static const size_t MAX_PROCESS_K_LENGTH = 4096;
    static const size_t MAX_PROCESS_V_LENGTH = 4096;
    static const size_t MAX_GROUP_LENGTH = 4096;
    static const size_t MAX_TAG_K_LENGTH = 4096;
    static const size_t MAX_TAG_V_LENGTH = 4096;
};   // class InstanceMeta
    
class InstanceInfo
{
public:
    struct InstanceState {
        enum type{
            INVALID     =  0,
            DEPLOYING   = 10,       //
            STARTING    = 20,
            RUNNING     = 30,       //
            ERROR       = 40,       //
            REMOVING    = 50,       //
        };
        static const std::string to_string(const type t) {
            switch (t) {
                case INVALID  : return "INVALID";
                case DEPLOYING: return "DEPLOYING";
                case STARTING : return "STARTING";
                case RUNNING  : return "RUNNING";
                case ERROR    : return "ERROR";
                case REMOVING : return "REMOVING";
                default      : throw std::runtime_error("Invalid InstanceState");
            }
        }
    };
    
    struct ErrorCode {
        enum type{
            SUCCESS             =  0,
            DEPLOY_TIMEOUT      = 10,  //
            DEPLOY_FAIL         = 20,  //
            HEALCHECK_TIMEOUT   = 30,
            HEALCHECK_FAIL      = 40,  //
            REMOVE_TIMEOUT      = 50,  //
            REMOVE_FAIL         = 60,  //
        };
        static const std::string to_string(const type t) {
            switch (t) {
                case SUCCESS          : return "SUCCESS";
                case DEPLOY_TIMEOUT   : return "DEPLOY_TIMEOUT";
                case DEPLOY_FAIL      : return "DEPLOY_FAIL";
                case HEALCHECK_TIMEOUT: return "HEALCHECK_TIMEOUT";
                case HEALCHECK_FAIL   : return "HEALCHECK_FAIL";
                case REMOVE_TIMEOUT   : return "REMOVE_TIMEOUT";
                case REMOVE_FAIL      : return "REMOVE_FAIL";
                default              : throw std::runtime_error("Invalid ErrorCode");
            }
        }
    };

    struct HealthCheckStatus {
        enum type {
            RUNNING,
            STARTING,
            ERROR,
            TIMEOUT,
        };
        static const std::string to_string(const type t) {
            switch (t) {
                case RUNNING : return "RUNNING";
                case STARTING: return "STARTING";
                case ERROR   : return "ERROR";
                case TIMEOUT : return "TIMEOUT";
                default     : throw std::runtime_error("Invalid HealthCheckStatus");
            }
        }
    };
    
public:
    InstanceInfo();
    InstanceInfo(std::string service_name, int32_t offset);
    ~InstanceInfo();
    //
    bool from_json(const Json::Value& t);
    Json::Value to_json() const;

    bool is_valid() const;

public:
    std::string get_instance_name() const;
    std::string get_meta_version() const;
    std::string get_service_name() const;
    int32_t get_offset() const;
    InstanceState::type get_instance_state() const;
    ErrorCode::type get_error_code() const;
    const InstanceMeta& get_instance_meta() const;
    
    void set_instance_state(const InstanceState::type state);
    void set_error_code(const ErrorCode::type error_code);
    void set_instance_meta(const InstanceMeta & meta);
private:
    std::string _service_name;
    int32_t _offset;
    InstanceState::type _instance_state;
    ErrorCode::type _error_code;
    InstanceMeta _instance_meta;
    
};  // class InstanceInfo
}   // namespace matrix

#endif // __INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_INSTANCE_INFO_H__
