#include "HostInfo.h"

#include <sstream>

#include "op/oped/noah/sailor/utility/Logger.h"
#include "op/oped/noah/sailor/utility/thread/MutexLocker.h"

namespace matrix {

ResourceTuple::ResourceTuple():_cpu_num(0), _memory_mb(0), _disk_total_mb(0)
{
    // nothing
}

ResourceTuple::ResourceTuple(int64_t cpu_num, int64_t memory_mb, int64_t disk_total_mb):
                             _cpu_num(cpu_num), _memory_mb(memory_mb), _disk_total_mb(disk_total_mb)
{
    // nothing
}

bool ResourceTuple::is_valid() const
{
    if (_cpu_num < 0 || _memory_mb < 0 || _disk_total_mb < 0 ){
        return false;
    }
    return true;
}

bool ResourceTuple::is_empty() const
{
    if (_cpu_num == 0 || _memory_mb == 0 || _disk_total_mb == 0 ){
        return true;
    }
    return false;
}
    
bool ResourceTuple::from_json(const Json::Value& t)
{
    bool ret = false;
    std::string err_msg;
    do {
        if (t.isMember("cpu_num") && t["cpu_num"].isInt()) {
            _cpu_num = t["cpu_num"].asInt64();
        } else {
            err_msg = "cpu_num";
            break;
        }
        if (t.isMember("memory_mb") && t["memory_mb"].isInt()) {
            _memory_mb = t["memory_mb"].asInt64();
        } else {
            err_msg = "memory_mb";
            break;
        }
        if (t.isMember("disk_total_mb") && t["disk_total_mb"].isInt()) {
            _disk_total_mb = t["disk_total_mb"].asInt64();
        } else {
            err_msg = "disk_total_mb";
            break;
        }
        
        ret = true;
    } while (false);
    
    if (!ret) {
        LOG.warn("ResourceTuple: parse %s error.", err_msg.c_str());
    }
    return ret;
}
    
const Json::Value ResourceTuple::to_json() const
{
    Json::Value root;
    root["cpu_num"]   = Json::Int64(_cpu_num);
    root["memory_mb"] = Json::Int64(_memory_mb);
    root["disk_total_mb"]   = Json::Int64(_disk_total_mb);
    return root;
}

ResourceTuple ResourceTuple::operator +(const ResourceTuple& another) const
{
    ResourceTuple tmp;
    tmp._cpu_num       = _cpu_num       + another._cpu_num;
    tmp._memory_mb     = _memory_mb     + another._memory_mb;
    tmp._disk_total_mb = _disk_total_mb + another._disk_total_mb;
    return tmp;
}

ResourceTuple ResourceTuple::operator -(const ResourceTuple& another) const
{
    ResourceTuple tmp;
    tmp._cpu_num       = _cpu_num       - another._cpu_num;
    tmp._memory_mb     = _memory_mb     - another._memory_mb;
    tmp._disk_total_mb = _disk_total_mb - another._disk_total_mb;
    return tmp;
}

bool ResourceTuple::operator >(const ResourceTuple& another) const
{
    if (_cpu_num > another._cpu_num && _memory_mb > another._memory_mb && _disk_total_mb > another._disk_total_mb) {
        return true;
    }
    return false;
}

bool ResourceTuple::operator >=(const ResourceTuple& another) const
{
    if (_cpu_num >= another._cpu_num && _memory_mb >= another._memory_mb && _disk_total_mb >= another._disk_total_mb) {
        return true;
    }
    return false;
}

bool ResourceTuple::operator ==(const ResourceTuple& another) const
{
    if (_cpu_num == another._cpu_num && _memory_mb == another._memory_mb && _disk_total_mb == another._disk_total_mb) {
        return true;
    }
    return false;
}
    
HostInfo::HostInfo():_state(HostState::INVALID)
{
    // nothing
}

HostInfo::HostInfo(const HostInfo& other):_state(other._state), _total_resource(other._total_resource),
                                           _reserved_resource(other._reserved_resource),
                                           _used_resource(other._used_resource), _mutex()
{
    // nothing
}

HostInfo::~HostInfo()
{
    // nothing
}

HostInfo& HostInfo::operator =(const HostInfo& from)
{
    _state             = from._state;
    _total_resource    = from._total_resource;
    _reserved_resource = from._reserved_resource;
    _used_resource     = from._used_resource;
    return *this;
}

bool HostInfo::is_ready() const
{
    ::sailor::MutexLocker locker(&_mutex);
    
    if (!_total_resource.is_valid() || !_reserved_resource.is_valid() || !_used_resource.is_valid()) {
        LOG.debug("HostInfo: resource is invalid.");
        return false;
    }
    
    if (_state == HostState::INVALID) {
        LOG.debug("HostInfo: state is invalid.");
        return false;
    }
    
    if (_total_resource.is_empty() || _reserved_resource.is_empty()) {
        LOG.debug("HostInfo: total resource or reserved resource is empty.");
        return false;
    }
    
    if (!(_total_resource >= _used_resource + _reserved_resource )) {
        LOG.error("HostInfo: could never happen. _used_resource + _reserved_resource > _total_resource.");
        return false;
    }
    
    return true;
}

bool HostInfo::has_enough_resource(const ResourceTuple& resource) const
{
    ::sailor::MutexLocker locker(&_mutex);
    if (_total_resource >= _used_resource + resource + _reserved_resource ) {
        return true;
    }
    return false;
}

bool HostInfo::use_resource(const ResourceTuple& resource)
{
    ::sailor::MutexLocker locker(&_mutex);
    _used_resource = _used_resource + resource;
    return true;
}

bool HostInfo::release_resource(const ResourceTuple& resource)
{
    ::sailor::MutexLocker locker(&_mutex);
    _used_resource = _used_resource - resource;
    return true;
}

bool HostInfo::init_used_resource()
{
    ::sailor::MutexLocker locker(&_mutex);
    _used_resource._cpu_num       = 0;
    _used_resource._memory_mb     = 0;
    _used_resource._disk_total_mb = 0;
    return true;
}

Json::Value HostInfo::to_json() const
{
    ::sailor::MutexLocker locker(&_mutex);
    Json::Value t;
    t["total_resource"]    = _total_resource.to_json();
    t["reserved_resource"] = _reserved_resource.to_json();
    t["used_resource"]     = _used_resource.to_json();
    return t;
}

HostInfo::HostState::type HostInfo::get_host_state() const
{
    return _state;
}

const ResourceTuple& HostInfo::get_total_resource() const
{
    ::sailor::MutexLocker locker(&_mutex);
    return _total_resource;
}

void HostInfo::set_host_state(HostInfo::HostState::type state)
{
    ::sailor::MutexLocker locker(&_mutex);
    _state = state;
}

void HostInfo::set_total_resource(const ResourceTuple& resource)
{
    ::sailor::MutexLocker locker(&_mutex);
    if (!resource.is_valid()) {
        LOG.warn("HostInfo: set total resource but is invalid: %s", resource.to_json().toStyledString().c_str());
        return;
    }
    _total_resource = resource;
}

void HostInfo::set_reserved_resource(const ResourceTuple& resource)
{
    ::sailor::MutexLocker locker(&_mutex);
    if (!resource.is_valid()) {
        LOG.warn("HostInfo: set reserved resource but is invalid: %s", resource.to_json().toStyledString().c_str());
        return;
    }
    
    if (!(_total_resource >= resource + _used_resource )) {
        LOG.warn("HostInfo: failed to set reserved resource, because reverted_resource + _used_resource > _total_resource.");
        return;
    }
    _reserved_resource = resource;
}

}   // namespace matrix
