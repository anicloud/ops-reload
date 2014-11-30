#include <set>

#include "inf/computing/matrix/matrix-agent/reporter/Heartbeat.h"

namespace matrix {

AgentConf::AgentConf(bool master_safe_mode,
                     int32_t heartbeat_interval_sec,
                     int32_t agent_http_port,
                     const ResourceTuple& reserved_resource) : 
    _master_safe_mode(master_safe_mode),
    _heartbeat_interval_sec(heartbeat_interval_sec),
    _agent_http_port(agent_http_port),
    _reserved_resource(reserved_resource)
{
}

AgentConf AgentConf::from_json(const Json::Value& json) throw (MatrixJsonException)
{
    bool master_safe_mode;
    int32_t heartbeat_interval_sec;
    int32_t agent_http_port;
    ResourceTuple reserved_resource;
    
    if (json.isMember("master_safe_mode") && json["master_safe_mode"].isInt()) {
        master_safe_mode = json["master_safe_mode"].asBool();
    } else {
        throw MatrixJsonException("AgentConf::master_safe_mode load failed.");
    }

    if (json.isMember("heartbeat_interval_sec") && json["heartbeat_interval_sec"].isInt()) {
        heartbeat_interval_sec = json["heartbeat_interval_sec"].asInt();
    } else {
        throw MatrixJsonException("AgentConf::heartbeat_interval_sec load failed.");
    }

    if (json.isMember("agent_http_port") && json["agent_http_port"].isInt()) {
        agent_http_port = json["agent_http_port"].asInt();
    } else {
        throw MatrixJsonException("AgentConf::agent_http_port load failed.");
    }

    if (json.isMember("reserved_resource") && json["reserved_resource"].isObject() &&
        reserved_resource.from_json(json["reserved_resource"])) {
    } else {
        throw MatrixJsonException("AgentConf::reserved_resource load failed.");
    }

    AgentConf agent_conf(master_safe_mode,
                         heartbeat_interval_sec,
                         agent_http_port,
                         reserved_resource);

    if (!agent_conf.is_valid()) {
        throw MatrixJsonException("AgentConf is not valid.");
    }

    return agent_conf;
    
}

AgentConf::~AgentConf()
{
}

bool AgentConf::is_valid() const
{
    if (_heartbeat_interval_sec > MAX_HEART_BEATINTERVAL_SEC || 
        _heartbeat_interval_sec < MIN_HEART_BEATINTERVAL_SEC) {

        LOG.warn("AgentConf: invalid _heartbeat_interval_sec %d", 
                 _heartbeat_interval_sec);
        return false;
    }
    
    if (_agent_http_port < MIN_PORT_RANGE || 
        _agent_http_port > MAX_PORT_RANGE) {
        LOG.warn("AgentConf: invalid _agent_http_port %u", 
                 _agent_http_port);
        return false;
    }

    if (!_reserved_resource.is_valid()) {
        LOG.warn("AgentConf: invalid _reserved_resource");
        return false;
    }

    return true;
}

Json::Value AgentConf::to_json() const
{
    Json::Value json;
    json["master_safe_mode"] = Json::Int(_master_safe_mode);
    json["heartbeat_interval_sec"] = Json::Int(_heartbeat_interval_sec);
    json["agent_http_port"] = Json::Int(_agent_http_port);
    json["reserved_resource"] = _reserved_resource.to_json();
    return json;
}

HeartbeatMessage::HeartbeatMessage(const HostInfo& host_info,
                                   const std::vector<InstanceInfo>& instances,
                                   bool first_heartbeat) :
    _host_info(host_info),
    _instances(instances),
    _first_heartbeat(first_heartbeat)
{
}

HeartbeatMessage::~HeartbeatMessage()
{
}

Json::Value HeartbeatMessage::to_json() const
{
    Json::Value json;
    for (size_t i = 0 ; i <  _instances.size(); i ++) {
        json["instances"][(int)i] = _instances[i].to_json();
    }
    json["host_info"] = _host_info.to_json();
    json["first_heartbeat"] = Json::Int(_first_heartbeat);
    return json;
}

HeartbeatResponse::HeartbeatResponse(const AgentConf& agent_conf,
                                     const std::vector<InstanceInfo>& expect_instances) :
    _agent_conf(agent_conf),
    _expect_instances(expect_instances)
{
}

HeartbeatResponse::~HeartbeatResponse() {}

HeartbeatResponse HeartbeatResponse::from_json(const Json::Value& json) throw (MatrixJsonException)
{
    std::vector<InstanceInfo> expect_instances;
    if (json.isMember("expect_instances") && json["expect_instances"].isArray()) {

        for(size_t i = 0 ; i < json["expect_instances"].size() ; i ++) {

            InstanceInfo info;

            if (json["expect_instances"][(int)i].isObject() &&
                info.from_json(json["expect_instances"][(int)i])) {

                expect_instances.push_back(info);

            } else {
                throw MatrixJsonException("HeartbeatResponse::expect_instances load failed");
            }
        }
    } else if (json.isMember("expect_instances") && !json["expect_instances"].isArray()) {
        throw MatrixJsonException("HeartbeatResponse::expect_instances load failed");
    }

    if (json.isMember("agent_conf") && json["agent_conf"].isObject()) {
        AgentConf agent_conf = AgentConf::from_json(json["agent_conf"]);
        HeartbeatResponse response(agent_conf, expect_instances);
        if (!response.is_valid()) {
            throw MatrixJsonException("HeartbeatResponse is not valid.");
        }
        return response;
    } else {
        throw MatrixJsonException("HeartbeatResponse::agent_conf load failed.");
    }
}

bool HeartbeatResponse::is_valid() const
{
    std::set< std::pair<std::string, int32_t> > dup_instances;

    for (size_t i = 0 ; i <  _expect_instances.size(); i ++) {

        if (!_expect_instances[i].is_valid()) {
            LOG.warn("HeartbeatResponse::_expect_instances[%lu] is invalid", i);
            return false;
        }
        std::pair<std::string, int32_t> key(_expect_instances[i].get_service_name(), 
                                            _expect_instances[i].get_offset());
        if(_expect_instances[i].get_instance_state() != 
           InstanceInfo::InstanceState::RUNNING) {
            LOG.warn("HeartbeatResponse: expect instance %s:%d state not RUNNING : %d", 
                     key.first.c_str(), 
                     key.second, 
                     _expect_instances[i].get_instance_state());

            return false;
        }

        if(dup_instances.find(key) == dup_instances.end()) {
            dup_instances.insert(key);
        } else {
            LOG.warn("HeartbeatResponse: dup expect instance %s:%d", 
                     key.first.c_str(), 
                     key.second);
            return false;
        }
    }

    return _agent_conf.is_valid();
}

Json::Value HeartbeatResponse::to_json() const
{
    Json::Value json;

    for(size_t i = 0 ; i <  _expect_instances.size(); i ++) {
        json["expect_instances"][(int)i] = _expect_instances[i].to_json();
    }

    json["agent_conf"] = _agent_conf.to_json();

    return json;
}

}

