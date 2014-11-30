#include "InstanceInfo.h"

#include <map>
#include <sstream>

#include <assert.h>
#include <json/json.h>

#include "op/oped/noah/sailor/utility/Logger.h"

#include "inf/computing/matrix/matrix-agent/common/Utility.h"

namespace matrix
{

InstanceMeta::InstanceMeta()
{
    // nothing
}
    
InstanceMeta::~InstanceMeta()
{
    
}

bool InstanceMeta::is_valid() const
{
    if (_meta_version.size() > MAX_META_VERSION_LENGTH || _meta_version.size() == 0 || 
        !string_check_version(_meta_version)) {
        LOG.warn("InstanceMeta::_meta_version %s invalid", _meta_version.c_str());
        return false;
    }
    
    if (_package_source.size() > MAX_PACKAGE_SOURCE_LENGTH || _package_source.size() == 0 || 
        !string_check_url(_package_source)) {
        LOG.warn("InstanceMeta::_package_source %s invalid", _package_source.c_str());
        return false;
    }
    
    // package_version could be empty
    if (_package_version.size() > MAX_PACKAGE_VERSION_LENGTH || 
        !string_check_version(_package_version)) {
        LOG.warn("InstanceMeta::_package_version %s invalid", _package_version.c_str());
        return false;
    }
    
    // deploy_dir could be empty
    if (_deploy_dir.size() > MAX_DEPLOY_DIR_LENGTH || 
        !string_check_abspath(_deploy_dir)) {
        LOG.warn("InstanceMeta::_deploy_dir %s invalid", _deploy_dir.c_str());
        return false;
    }
    
    if (_max_port_include >= 65535 || _max_port_include <= 0 ||
        _min_port_include >= 65535 || _min_port_include <= 0 ){
        LOG.warn("InstanceMeta::_max_port_include or _min_port_include %d, %d invalid",
                 _max_port_include, _min_port_include);
        return false;
    }
    
    for (std::map<std::string, int32_t>::const_iterator iter = _port.begin();
         iter != _port.end(); ++ iter)
    {
        if (iter->first.size() > MAX_PORT_K_LENGTH || iter->first.size() == 0 ||
            !string_check_base(iter->first)) {
            LOG.warn("InstanceMeta::_port name %s invalid", iter->first.c_str());
            return false;
        }
        
        if (iter->second >= 65535 || iter->second <=0 || 
            iter->second > _max_port_include || iter->second < _min_port_include) {
            LOG.warn("InstanceMeta::port %d invalid", iter->second);
            return false;
        }
    }
    
    for (std::map<std::string, std::string>::const_iterator iter = _process_name.begin();
         iter != _process_name.end(); ++ iter)
    {
        if (iter->first.size() > MAX_PROCESS_K_LENGTH || iter->first.size() == 0 ||
            !string_check_base(iter->first)) {
            LOG.warn("InstanceMeta::process_name key %s invalid", iter->first.c_str());
            return false;
        }
        
        if (iter->second.size() > MAX_PROCESS_V_LENGTH || iter->second.size() == 0 ||
            !string_check_path(iter->second)) {
            LOG.warn("InstanceMeta::process_name value %s invalid", iter->second.c_str());
            return false;
        }
    }
    
    for (std::map<std::string, std::string>::const_iterator iter = _tag.begin();
         iter != _tag.end(); ++ iter)
    {
        if (iter->first.size() > MAX_TAG_K_LENGTH || iter->first.size() == 0 ||
            !string_check_base(iter->first)) {
            LOG.warn("InstanceMeta::tag key %s invalid", iter->first.c_str());
            return false;
        }
        
        if (iter->second.size() > MAX_TAG_V_LENGTH || iter->second.size() == 0 ||
            !string_check_path(iter->second)) {
            LOG.warn("InstanceMeta::tag value %s invalid", iter->second.c_str());
            return false;
        }
    }
    
    if (!_resource.is_valid()) {
        LOG.warn("InstanceMeta::resource %s invalid", _resource.to_json().toStyledString().c_str());
        return false;
    }
    
    if (_deploy_timeout_sec <0) {
        LOG.warn("InstanceMeta::_deploy_timeout_sec %d invalid", _deploy_timeout_sec);
        return false;
    }
    
    if (_health_check_timeout_sec <0) {
        LOG.warn("InstanceMeta::_health_check_timeout_sec %d invalid", _health_check_timeout_sec);
        return false;
    }
    
    if (_group.size() > MAX_GROUP_LENGTH || _group.size() == 0 || 
        !string_check_base(_group)) {
        LOG.warn("InstanceMeta::_group %s invalid", _group.c_str());
        return false;
    }
    
    return true;
}

bool InstanceMeta::from_json(const Json::Value& t) {
    bool ret = false;
    std::string err_msg;
    do {
        if (t.isMember("meta_version") && t["meta_version"].isString()) {
            _meta_version = t["meta_version"].asString();
        } else {
            err_msg = "meta_version";
            break;
        }
        if (t.isMember("package_source") && t["package_source"].isString()) {
            _package_source = t["package_source"].asString();
        } else {
            err_msg = "package_source";
            break;
        }
        if (t.isMember("package_version") && t["package_version"].isString()) {
            _package_version = t["package_version"].asString();
        } else {
            err_msg = "package_version";
            break;
        }
        if (t.isMember("package_type") && t["package_type"].isInt()) {
            _package_type = (PackageType::type)(t["package_type"].asInt());
        } else {
            err_msg = "package_type";
            break;
        }
        if (t.isMember("deploy_dir") && t["deploy_dir"].isString()) {
            _deploy_dir = t["deploy_dir"].asString();
        } else {
            err_msg = "deploy_dir";
            break;
        }
        if (t.isMember("port") && t["port"].isObject() &&
            map_from_json(t["port"], &_port)            ){
            // nothing
        } else {
            err_msg = "port";
            break;
        }
        if (t.isMember("max_port_include") && t["max_port_include"].isInt()) {
            _max_port_include = t["max_port_include"].asInt();
        } else {
            err_msg = "max_port_include";
            break;
        }
        if (t.isMember("min_port_include") && t["min_port_include"].isInt()) {
            _min_port_include = t["min_port_include"].asInt();
        } else {
            err_msg = "min_port_include";
            break;
        }
        if (t.isMember("process_name") && t["process_name"].isObject() &&
            map_from_json(t["process_name"], &_process_name)            ){
            // nothing
        } else {
            err_msg = "process_name";
            break;
        }
        if (t.isMember("tag") && t["tag"].isObject() &&
            map_from_json(t["tag"], &_tag)            ){
            // nothing
        } else {
            err_msg = "tag";
            break;
        }
        if (t.isMember("resource") && t["resource"].isObject() &&
            _resource.from_json(t["resource"])                 ){
            // nothing
        } else {
            err_msg = "resource";
            break;
        }
        if (t.isMember("deploy_timeout_sec") && t["deploy_timeout_sec"].isInt()) {
            _deploy_timeout_sec = t["deploy_timeout_sec"].asInt();
        } else {
            err_msg = "deploy_timeout_sec";
            break;
        }
        if (t.isMember("health_check_timeout_sec") && t["health_check_timeout_sec"].isInt()) {
            _health_check_timeout_sec = t["health_check_timeout_sec"].asInt();
        } else {
            err_msg = "health_check_timeout_sec";
            break;
        }
        
        if (t.isMember("group") && t["group"].isString()) {
            _group = t["group"].asString();
        } else {
            err_msg = "group";
            break;
        }
        ret = true;
    } while(false);
    
    if (!ret) {
        LOG.warn("InstanceMeta: parse %s error.", err_msg.c_str());
    }
    return ret;
}

Json::Value InstanceMeta::to_json() const
{
    Json::Value root, t;
    root["meta_version"] = _meta_version;
    root["package_source"] = _package_source;
    root["package_version"] = _package_version;
    root["package_type"] = Json::Int(_package_type);
    root["deploy_dir"] = _deploy_dir;
    // port
    t.clear();
    for (std::map<std::string, int32_t>::const_iterator iter = _port.begin();
        iter != _port.end(); ++ iter)
    {
        t[iter->first] = iter->second;
    }
    root["port"] = t;
    root["max_port_include"] = Json::Int(_max_port_include);
    root["min_port_include"] = Json::Int(_min_port_include);
    // process_name
    t.clear();
    for (std::map<std::string, std::string>::const_iterator iter = _process_name.begin();
        iter != _process_name.end(); ++ iter)
    {
        t[iter->first] = iter->second;
    }
    root["process_name"] = t;
    // tag
    t.clear();
    for (std::map<std::string, std::string>::const_iterator iter = _tag.begin();
        iter != _tag.end(); ++ iter)
    {
        t[iter->first] = iter->second;
    }
    root["tag"] = t;
    
    root["resource"] = _resource.to_json();
    root["deploy_timeout_sec"] = Json::Int(_deploy_timeout_sec);
    root["health_check_timeout_sec"] = Json::Int(_health_check_timeout_sec);
    root["group"] = _group;
    
    return root;
}

inline bool InstanceMeta::map_from_json(const Json::Value& t, std::map<std::string, int32_t>* map)
{
    assert(map);
    map->clear();
    if (!t.isObject()) {
        return false;
    }
    Json::Value::Members mem = t.getMemberNames();
    for (Json::Value::Members::const_iterator iter = mem.begin();
         iter != mem.end(); ++ iter)
    {
        if (t[*iter].isInt()) {
            map->insert(std::pair<std::string, int32_t>(*iter, t[*iter].asInt()));
        } else {
            return false;
        }
    }
    return true;
}

inline bool InstanceMeta::map_from_json(const Json::Value& t, std::map<std::string, std::string>* map)
{
    assert(map);
    map->clear();
    if (!t.isObject()) {
        return false;
    }
    Json::Value::Members mem = t.getMemberNames();
    for (Json::Value::Members::const_iterator iter = mem.begin();
         iter != mem.end(); ++ iter)
    {
        if (t[*iter].isString()) {
            map->insert(std::pair<std::string, std::string>(*iter, t[*iter].asString()));
        } else {
            return false;
        }
    }
    return true;
}
    
InstanceInfo::InstanceInfo():_service_name(""), _offset(0),
                _instance_state(InstanceState::INVALID), _error_code(ErrorCode::SUCCESS)
{
    
}

InstanceInfo::InstanceInfo(std::string service_name, int32_t offset):
                _service_name(service_name), _offset(offset),
                _instance_state(InstanceState::INVALID), _error_code(ErrorCode::SUCCESS)
{
    
}

InstanceInfo::~InstanceInfo()
{
}

bool InstanceInfo::from_json(const Json::Value& t)
{
    bool ret = false;
    std::string err_msg;
    do {
        if (t.isMember("service_name") && t["service_name"].isString()) {
            _service_name = t["service_name"].asString();
        } else {
            err_msg = "parse service_name error.";
            break;
        }
        if (t.isMember("offset") && t["offset"].isInt()) {
            _offset = t["offset"].asInt();
        } else {
            err_msg = "parse offset error.";
            break;
        }
        if (t.isMember("instance_state") && t["instance_state"].isInt()) {
            _instance_state = (InstanceState::type)(t["instance_state"].asInt());
        } else {
            err_msg = "parse instance_state error.";
            break;
        }
        if (t.isMember("error_code") && t["error_code"].isInt()) {
            _error_code = (ErrorCode::type)(t["error_code"].asInt());
        } else {
            err_msg = "parse error_code error.";
            break;
        }
        if (t.isMember("instance_meta") && t["instance_meta"].isObject() &&
            _instance_meta.from_json(t["instance_meta"])                 ){
            // nothing
        } else {
            err_msg = "parse instance_meta error.";
            break;
        }
        ret = true;
    } while(false);
    
    if (!ret) {
        LOG.warn("InstanceInfo: %s", err_msg.c_str());
    }
    return ret;
}

Json::Value InstanceInfo::to_json() const
{
    Json::Value root;
    
    root["service_name"] = _service_name;
    root["offset"] = Json::Int(_offset);
    root["instance_state"] = Json::Int(_instance_state);
    root["error_code"] = Json::Int(_error_code);
    root["instance_meta"] = _instance_meta.to_json();
    
    return root;
}

std::string InstanceInfo::get_instance_name() const
{
    std::ostringstream instance_name;
    instance_name << _offset << "." << _service_name;
    return instance_name.str();
}

std::string InstanceInfo::get_meta_version() const
{
    return _instance_meta._meta_version;
}

std::string InstanceInfo::get_service_name() const
{
    return _service_name;
}

int32_t InstanceInfo::get_offset() const
{
    return _offset;
}

InstanceInfo::InstanceState::type InstanceInfo::get_instance_state() const
{
    return _instance_state;
}

InstanceInfo::ErrorCode::type InstanceInfo::get_error_code() const
{
    return _error_code;
}

const InstanceMeta& InstanceInfo::get_instance_meta() const
{
    return _instance_meta;
}

void InstanceInfo::set_instance_state(const InstanceState::type state)
{
    _instance_state = state;
}

void InstanceInfo::set_error_code(const ErrorCode::type error_code)
{
    _error_code = error_code;
}

void InstanceInfo::set_instance_meta(const InstanceMeta& meta)
{
    _instance_meta = meta;
}

bool InstanceInfo::is_valid() const {
    if (!string_check_name(_service_name)) {
        LOG.warn("InstanceInfo: service_name [%s] is invalid.", _service_name.c_str());
        return false;
    }
    if (_offset < 0) {
        LOG.warn("InstanceInfo: offset [%d] is invalid.", _offset);
        return false;
    }
    try {
        std::string tmp;
        tmp = InstanceState::to_string(_instance_state);
        tmp = ErrorCode::to_string(_error_code);
    } catch (std::exception& ex) {
        LOG.warn("InstanceInfo: %s.", ex.what());
        return false;
    }
    return _instance_meta.is_valid();
}


}  // namespace matrix



