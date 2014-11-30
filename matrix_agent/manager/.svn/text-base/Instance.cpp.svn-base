#include "Instance.h"

#include <sstream>
#include <fstream>
#include <stdexcept>

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#include <json/json.h>
#include "op/oped/noah/sailor/utility/File.h"
#include "op/oped/noah/sailor/utility/Logger.h"
#include "op/oped/noah/sailor/utility/thread/MutexLocker.h"

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/manager/InstanceInfo.h"

namespace matrix  {


// state machine
const Instance::Translation Instance::_state_trans_table[] =
{
    {InstanceInfo::InstanceState::INVALID,   Instance::Events::INSTALL,      InstanceInfo::InstanceState::DEPLOYING},
    {InstanceInfo::InstanceState::DEPLOYING, Instance::Events::INSTALL_OK,   InstanceInfo::InstanceState::STARTING},
    {InstanceInfo::InstanceState::DEPLOYING, Instance::Events::INSTALL_FAIL, InstanceInfo::InstanceState::ERROR},
    {InstanceInfo::InstanceState::DEPLOYING, Instance::Events::INSTALL_TIMEO,InstanceInfo::InstanceState::ERROR},
    {InstanceInfo::InstanceState::STARTING,  Instance::Events::HEALTH_OK,    InstanceInfo::InstanceState::RUNNING},
    {InstanceInfo::InstanceState::STARTING,  Instance::Events::HEALTH_FAIL,  InstanceInfo::InstanceState::ERROR},
    {InstanceInfo::InstanceState::STARTING,  Instance::Events::HEALTH_TIMEO, InstanceInfo::InstanceState::ERROR},
    {InstanceInfo::InstanceState::STARTING,  Instance::Events::REMOVE,       InstanceInfo::InstanceState::REMOVING},
    {InstanceInfo::InstanceState::RUNNING,   Instance::Events::INSTALL,      InstanceInfo::InstanceState::DEPLOYING},
    {InstanceInfo::InstanceState::RUNNING,   Instance::Events::REMOVE,       InstanceInfo::InstanceState::REMOVING},
    {InstanceInfo::InstanceState::RUNNING,   Instance::Events::HEALTH_FAIL,  InstanceInfo::InstanceState::ERROR},
    {InstanceInfo::InstanceState::RUNNING,   Instance::Events::HEALTH_TIMEO, InstanceInfo::InstanceState::ERROR},
    {InstanceInfo::InstanceState::RUNNING,   Instance::Events::HEALTH_START, InstanceInfo::InstanceState::STARTING},
    {InstanceInfo::InstanceState::REMOVING,  Instance::Events::REMOVE_OK,    InstanceInfo::InstanceState::INVALID},
    {InstanceInfo::InstanceState::REMOVING,  Instance::Events::REMOVE_FAIL,  InstanceInfo::InstanceState::ERROR},
    {InstanceInfo::InstanceState::REMOVING,  Instance::Events::REMOVE_TIMEO, InstanceInfo::InstanceState::ERROR},
    {InstanceInfo::InstanceState::ERROR,     Instance::Events::INSTALL,      InstanceInfo::InstanceState::DEPLOYING},
    {InstanceInfo::InstanceState::ERROR,     Instance::Events::REMOVE,       InstanceInfo::InstanceState::REMOVING},
    {InstanceInfo::InstanceState::ERROR,     Instance::Events::HEALTH_OK,    InstanceInfo::InstanceState::RUNNING},
    {InstanceInfo::InstanceState::ERROR,     Instance::Events::HEALTH_START, InstanceInfo::InstanceState::STARTING},
};

Instance::Instance(uint64_t generation, std::string service_name, int32_t offset): _generation(generation),
                        _instance_info(service_name, offset), _event_start_timestamp(0), _event_timeout_sec(0)
{
    // nothing
}

Instance::Instance(std::string meta_file)
{
    assert(g_config);
    
    std::string file_path = std::string(g_config->META_PATH) + "/" + meta_file;
    std::ostringstream content;
    std::ifstream ifs(file_path.c_str());
    if (ifs) {
        content << ifs.rdbuf();
        ifs.close();
    } else {
        throw std::runtime_error("Read meta from file error.");
    }

    if (!from_string(content.str())) {
        throw std::runtime_error("Construct from string error.");
    }
    
    if (!_instance_info.is_valid()) {
        throw std::runtime_error("InstanceInfo is invalid.");
    }
}

Instance::~Instance()
{
    // nothing
}


bool Instance::do_install(const InstanceMeta& instance_meta)
{
    ::sailor::MutexLocker locker(&_mutex);
    
    if (!instance_meta.is_valid()) {
        LOG.error("install instance but instance_meta is not valid.");
        return false;
    }
    
    if (!do_event(Events::INSTALL)) {
        return false;
    }
    
    _instance_info.set_instance_meta(instance_meta);
    _instance_info.set_error_code(InstanceInfo::ErrorCode::SUCCESS);
    set_timeout();
    save_to_file();
    
    LOG.trace("Instance [%s]: do install ok.", _instance_info.get_instance_name().c_str());
    return true;
}

bool Instance::do_update(const InstanceMeta& instance_meta)
{
    ::sailor::MutexLocker locker(&_mutex);

    if (_instance_info.get_error_code() == InstanceInfo::ErrorCode::REMOVE_FAIL    ||
        _instance_info.get_error_code() == InstanceInfo::ErrorCode::REMOVE_TIMEOUT ){
        LOG.info("Instance [%s]: already REMOVE_FAIL or REMOVE_TIMEOUT, ignore update.",
                 _instance_info.get_instance_name().c_str());
        return false;
    }

    if (_instance_info.get_meta_version() == instance_meta._meta_version) {
        return false;
    }
    
    if (!instance_meta.is_valid()) {
        LOG.warn("Instance [%s]: instance but instance_meta is not valid.", 
                 _instance_info.get_instance_name().c_str());
        return false;
    }
    
    if (! (_instance_info.get_instance_meta()._resource == instance_meta._resource)) {
        LOG.warn("Instance [%s]: resource is not equal when update.", 
                 _instance_info.get_instance_name().c_str());
        return false;
    }
    
    if (!do_event(Events::INSTALL)) {
        return false;
    }
    
    _instance_info.set_instance_meta(instance_meta);
    _instance_info.set_error_code(InstanceInfo::ErrorCode::SUCCESS);
    set_timeout();
    save_to_file();
    
    LOG.trace("Instance [%s]: do update ok.", _instance_info.get_instance_name().c_str());
    return true;
}

bool Instance::do_remove()
{
    ::sailor::MutexLocker locker(&_mutex);
    
    if (_instance_info.get_instance_state() == InstanceInfo::InstanceState::ERROR &&
       (_instance_info.get_error_code() == InstanceInfo::ErrorCode::REMOVE_FAIL   || 
        _instance_info.get_error_code() == InstanceInfo::ErrorCode::REMOVE_TIMEOUT))
    {
        // when remove fail or remove timeout, ignore it.
        return false;
    }

    if (!do_event(Events::REMOVE)) {
        return false;
    }
    
    _instance_info.set_error_code(InstanceInfo::ErrorCode::SUCCESS);
    set_timeout();
    save_to_file();
    
    LOG.trace("Instance [%s]: do remove ok.", _instance_info.get_instance_name().c_str());
    return true;
}

void Instance::on_health_check(InstanceInfo::HealthCheckStatus::type type)
{
    ::sailor::MutexLocker locker(&_mutex);
    
    if (_instance_info.get_error_code() == InstanceInfo::ErrorCode::DEPLOY_FAIL    ||
        _instance_info.get_error_code() == InstanceInfo::ErrorCode::DEPLOY_TIMEOUT ){
        LOG.warn("Instance [%s]: already DEPLOY_FAIL or DEPLOY_TIMEOUT but receive on_health_check [%s]",
                 _instance_info.get_instance_name().c_str(),
                 InstanceInfo::HealthCheckStatus::to_string(type).c_str());
        return;
    }

    if (_instance_info.get_error_code() == InstanceInfo::ErrorCode::REMOVE_FAIL    ||
        _instance_info.get_error_code() == InstanceInfo::ErrorCode::REMOVE_TIMEOUT ){
        LOG.info("Instance [%s]: already REMOVE_FAIL or REMOVE_TIMEOUT but receive on_health_check [%s]",
                 _instance_info.get_instance_name().c_str(),
                 InstanceInfo::HealthCheckStatus::to_string(type).c_str());
        return;
    }
    
    switch(type) {
    case InstanceInfo::HealthCheckStatus::RUNNING:
        if (do_event(Events::HEALTH_OK)) {
            _instance_info.set_error_code(InstanceInfo::ErrorCode::SUCCESS);
           break;
        }
        return;
    case InstanceInfo::HealthCheckStatus::ERROR:
        if (do_event(Events::HEALTH_FAIL)                                                  ||
                (_instance_info.get_instance_state() == InstanceInfo::InstanceState::ERROR &&
                 _instance_info.get_error_code() != InstanceInfo::ErrorCode::HEALCHECK_FAIL))
        {
            _instance_info.set_error_code(InstanceInfo::ErrorCode::HEALCHECK_FAIL);
           break;
        }
        return;
    case InstanceInfo::HealthCheckStatus::STARTING:
        if (do_event(Events::HEALTH_START)) {
            _instance_info.set_error_code(InstanceInfo::ErrorCode::SUCCESS);
           break;
        }
        return;
    case InstanceInfo::HealthCheckStatus::TIMEOUT:
        if (do_event(Events::HEALTH_TIMEO)                                                    ||
                (_instance_info.get_instance_state() == InstanceInfo::InstanceState::ERROR    &&
                 _instance_info.get_error_code() != InstanceInfo::ErrorCode::HEALCHECK_TIMEOUT))
        {
            _instance_info.set_error_code(InstanceInfo::ErrorCode::HEALCHECK_TIMEOUT);
           break;
        }
        return;
    default:
        throw std::runtime_error("invalid Events in on_health_check");
    }
    
    save_to_file();
    LOG.trace("Instance [%s]: on health check [%s] ok.", _instance_info.get_instance_name().c_str(),
              InstanceInfo::HealthCheckStatus::to_string(type).c_str());
}

void Instance::on_install(InstanceInfo::ErrorCode::type type)
{
    ::sailor::MutexLocker locker(&_mutex);
    
    switch(type) {
    case InstanceInfo::ErrorCode::SUCCESS:
        if (!do_event(Events::INSTALL_OK)) {
           return;
        }
        break;
    case InstanceInfo::ErrorCode::DEPLOY_FAIL:
        if (!do_event(Events::INSTALL_FAIL)) {
           return;
        }
        break;
    case InstanceInfo::ErrorCode::DEPLOY_TIMEOUT:
        if (!do_event(Events::INSTALL_TIMEO)) {
           return;
        }
        break;
    default:
        throw std::runtime_error("invalid Events in on_health_check");
    }
    _instance_info.set_error_code(type);
    clear_timeout();
    save_to_file();
    
    LOG.trace("Instance [%s]: on install res [%s] ok.", _instance_info.get_instance_name().c_str(),
              InstanceInfo::ErrorCode::to_string(type).c_str());
}

void Instance::on_remove(InstanceInfo::ErrorCode::type type)
{
    ::sailor::MutexLocker locker(&_mutex);
    
    switch(type) {
    case InstanceInfo::ErrorCode::SUCCESS:
        // REMOVE_OK must be success.
        do_event(Events::REMOVE_OK);
        break;
    case InstanceInfo::ErrorCode::REMOVE_FAIL:
        if (!do_event(Events::REMOVE_FAIL)) {
           return;
        }
        break;
    case InstanceInfo::ErrorCode::REMOVE_TIMEOUT:
        if (!do_event(Events::REMOVE_TIMEO)) {
           return;
        }
        break;
    default:
        throw std::runtime_error("invalid Events in on_health_check");
    }
    _instance_info.set_error_code(type);
    clear_timeout();
    save_to_file();
    
    if (type == InstanceInfo::ErrorCode::SUCCESS) {
        remove_meta_file();
    }
    
    LOG.trace("Instance [%s]: on remove res [%s] ok.", _instance_info.get_instance_name().c_str(),
              InstanceInfo::ErrorCode::to_string(type).c_str());
}

bool Instance::check_timeo()
{
    ::sailor::MutexLocker locker(&_mutex);
    
    if (_event_start_timestamp == 0) {
        return false;
    }
    
    struct timeval now;
    if (gettimeofday(&now, NULL) < 0) {
        LOG.error("gettimeofday failed. could not set timeout.");
        return false;
    }
    
    if (now.tv_sec - _event_start_timestamp < _event_timeout_sec) {
        return false;
    }
    
    //TODO: starting's timeout
    if (_instance_info.get_instance_state() == InstanceInfo::InstanceState::DEPLOYING) {
        LOG.info("Instance [%s]: installing timeout.", _instance_info.get_instance_name().c_str());
        locker.unlock();
        on_install(InstanceInfo::ErrorCode::DEPLOY_TIMEOUT);
        return true;
    }
    if (_instance_info.get_instance_state() == InstanceInfo::InstanceState::STARTING) {
        LOG.info("Instance [%s]: starting timeout.", _instance_info.get_instance_name().c_str());
        locker.unlock();
        on_install(InstanceInfo::ErrorCode::DEPLOY_TIMEOUT);
        return true;
    }
    if (_instance_info.get_instance_state() == InstanceInfo::InstanceState::REMOVING) {
        LOG.info("Instance [%s]: removing timeout.", _instance_info.get_instance_name().c_str());
        locker.unlock();
        on_remove(InstanceInfo::ErrorCode::REMOVE_TIMEOUT);
        return true;
    }
    
    return false;
}

bool Instance::reset_to_running()
{
    ::sailor::MutexLocker locker(&_mutex);
    
    LOG.info("Instance [%s]: reset state to running.", _instance_info.get_instance_name().c_str());
    _instance_info.set_instance_state(InstanceInfo::InstanceState::RUNNING);
    _instance_info.set_error_code(InstanceInfo::ErrorCode::SUCCESS);
    return true;
}

bool Instance::from_string(const std::string& content)
{
    ::sailor::MutexLocker locker(&_mutex);
    
    Json::Reader reader;
    Json::Value root;
    if (!reader.parse(content, root)) {
        LOG.warn("Instance: parse json error, %s", reader.getFormatedErrorMessages().c_str());
        return false;
    }

    if (!from_json(root)) {
        LOG.warn("Instance: read content from json error.");
        return false;
    }
    
    return true;
}

std::string Instance::to_string() const
{
    const Json::Value& t = to_json();
    return t.toStyledString();
}

bool Instance::do_event(Instance::Events::type ev)
{
    for (size_t i = 0; i < sizeof(_state_trans_table)/sizeof(Translation); i++) {
        if (_state_trans_table[i].ev == ev && _state_trans_table[i].from == _instance_info.get_instance_state()) {
            LOG.info("Instance [%s]: process event [%s], state [%s -> %s]",
                     _instance_info.get_instance_name().c_str(), Events::to_string(ev).c_str(),
                     InstanceInfo::InstanceState::to_string(_state_trans_table[i].from).c_str(),
                     InstanceInfo::InstanceState::to_string(_state_trans_table[i].to).c_str());
            
            _instance_info.set_instance_state(_state_trans_table[i].to);
            return true;
        }
    }
    LOG.info("Instance [%s]: ignore event [%s], current state [%s]", 
               _instance_info.get_instance_name().c_str(), Events::to_string(ev).c_str(),
               InstanceInfo::InstanceState::to_string(_instance_info.get_instance_state()).c_str());
    return false;
}

void Instance::set_timeout()
{
    assert(g_config);
    if (_instance_info.get_instance_state() == InstanceInfo::InstanceState::DEPLOYING ||
        _instance_info.get_instance_state() == InstanceInfo::InstanceState::STARTING  ){
        _event_timeout_sec = _instance_info.get_instance_meta()._deploy_timeout_sec != 0 ?
                             _instance_info.get_instance_meta()._deploy_timeout_sec      :
                             g_config->DEFAULT_MANAGER_INSTALL_TIMEO;
    }
    else if (_instance_info.get_instance_state() == InstanceInfo::InstanceState::REMOVING) {
        _event_timeout_sec = g_config->DEFAULT_MANAGER_REMOVE_TIMEO;
    }
    else {
        LOG.warn("Instance: invalid state to set timeout.");
        return;
    }
    
    struct timeval now;
    if (gettimeofday(&now, NULL) < 0) {
        LOG.error("gettimeofday failed. could not set timeout.");
        return;
    }
    _event_start_timestamp = now.tv_sec;
}

void Instance::clear_timeout()
{
    _event_start_timestamp = 0;
    _event_timeout_sec = 0;
}

bool Instance::from_json(const Json::Value& t)
{
    bool ret = false;
    std::string err_msg;
    do {
        if (t.isMember("generation") && t["generation"].isNumeric()) {
            _generation = t["generation"].asUInt64();
        } else {
            err_msg = "parse generation error.";
            break;
        }
        if (t.isMember("event_start_timestamp") && t["event_start_timestamp"].isIntegral()) {
            _event_start_timestamp = t["event_start_timestamp"].asInt();
        } else {
            err_msg = "parse event_start_timestamp error.";
            break;
        }
        if (t.isMember("event_timeout_sec") && t["event_timeout_sec"].isIntegral()) {
            _event_timeout_sec = t["event_timeout_sec"].asInt();
        } else {
            err_msg = "parse event_timeout_sec error.";
            break;
        }
        if (t.isMember("instance_info") && t["instance_info"].isObject() && 
            _instance_info.from_json(t["instance_info"])                 ){
            // nothing
        }
        else {
            err_msg = "parse instance_info error.";
            break;
        }
        ret = true;
    } while (false);
    
    if (!ret) {
        LOG.warn("Instance: %s", err_msg.c_str());
    }
    return ret;
}

const Json::Value Instance::to_json() const
{
    Json::Value root;
    
    root["generation"] = Json::UInt64(_generation);
    root["event_start_timestamp"] = Json::UInt(_event_start_timestamp);
    root["event_timeout_sec"] = Json::UInt(_event_timeout_sec);
    root["instance_info"] = _instance_info.to_json();
    
    return root;
}

bool Instance::save_to_file() const
{
    assert(g_config);
    
    std::ostringstream buffer;
    buffer << g_config->META_PATH << "/"
           << _generation << "." << _instance_info.get_instance_name();
    std::string file_path = buffer.str();
    ::sailor::File file(file_path);
    
    if (!file.open_for_writing()) {
        LOG.warn("Instance [%s]: failed to open [%s] for writing: %d", 
                  _instance_info.get_instance_name().c_str(),
                  file_path.c_str(), errno);
        return false;
    }
    const std::string& content = to_string();
    if (!file.write_all(content, 1000)) {
        LOG.warn("Instance [%s]: failed to write to [%s]: %d", 
                  _instance_info.get_instance_name().c_str(),
                  file_path.c_str(), errno);
        file.cancel();
        return false;
    }
    if (!file.close()) {
        LOG.warn("Instance [%s]: failed to write to [%s]: %d", 
                  _instance_info.get_instance_name().c_str(),
                  file_path.c_str(), errno);
        return false;
    }
    
    LOG.trace("Instance [%s]: save meta to file.", _instance_info.get_instance_name().c_str());
    return true;
}

bool Instance::remove_meta_file() const
{
    std::ostringstream file_path;
    file_path << g_config->META_PATH << "/"
           << _generation << "." << _instance_info.get_instance_name();
    if (unlink(file_path.str().c_str())) {
        LOG.error("Instance : failed to remove instance meta [%s]: %d", file_path.str().c_str(), errno);
        return false;
    }
    LOG.trace("Instance : success to remove instance meta file [%s].", file_path.str().c_str());
    return true;
}

const InstanceInfo& Instance::get_instance_info() const {
    return _instance_info;
}

uint64_t Instance::get_generation() const {
    return _generation;
}

}   // namespace matrix
