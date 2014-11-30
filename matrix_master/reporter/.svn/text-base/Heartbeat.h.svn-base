#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_REPORTER_HEARTBEAT_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_REPORTER_HEARTBEAT_H__

#include <vector>
#include <stdexcept>

#include "op/oped/noah/sailor/utility/Logger.h"

#include "inf/computing/matrix/matrix-agent/json-cpp/include/json/json.h"

#include "inf/computing/matrix/matrix-agent/manager/HostInfo.h"
#include "inf/computing/matrix/matrix-agent/manager/InstanceInfo.h"

namespace matrix {

class MatrixJsonException : public std::runtime_error
{
public:
    MatrixJsonException(const std::string& what) : runtime_error(what)
    {
    }
};

/**
 * Conf got from master when heartbeat.
 */
class AgentConf
{
public:
    static const int32_t MIN_HEART_BEATINTERVAL_SEC = 1; 
    static const int32_t MAX_HEART_BEATINTERVAL_SEC = 3600; // 1 hour
    static const int32_t MAX_HOST_TAG_LENGTH = 4096;
    static const int32_t MIN_PORT_RANGE = 1;
    static const int32_t MAX_PORT_RANGE = 65535;

public:
    AgentConf(bool master_safe_mode,
              int32_t heartbeat_interval_sec,
              int32_t agent_http_port,
              const ResourceTuple& reserved_resource); 
    /**
     * Construct AgentConf from json.
     *
     * @throws MatrixJsonException if parsing json failed.
     */
    static AgentConf from_json(const Json::Value& json) throw (MatrixJsonException);
    virtual ~AgentConf();

    bool is_valid() const;

    Json::Value to_json() const;

public:
    /**
     * Can not be modified once created!
     */
    const bool _master_safe_mode;
    const int32_t _heartbeat_interval_sec;
    const int32_t _agent_http_port;
    const ResourceTuple _reserved_resource;
};


class HeartbeatMessage
{
public:
    HeartbeatMessage(const HostInfo& host_info,
                     const std::vector<InstanceInfo>& instances,
                     bool first_heartbeat);
    virtual ~HeartbeatMessage();

    Json::Value to_json() const;

public:
    /**
     * Can not be modified once created!
     */
    const HostInfo _host_info;
    const std::vector<InstanceInfo> _instances;
    const bool _first_heartbeat;
};

class HeartbeatResponse
{
public:
    HeartbeatResponse(const AgentConf& agent_conf,
                      const std::vector<InstanceInfo>& expect_instances);
    static HeartbeatResponse from_json(const Json::Value& json) throw (MatrixJsonException);
    virtual ~HeartbeatResponse();

    bool is_valid() const;

    Json::Value to_json() const;
public: 
    /**
     * Can not be modified once created!
     */
    AgentConf _agent_conf;
    std::vector<InstanceInfo> _expect_instances;
};

}

#endif // __INF_COMPUTING_MATRIX_MATRIX_AGENT_REPORTER_HEARTBEAT_H__
