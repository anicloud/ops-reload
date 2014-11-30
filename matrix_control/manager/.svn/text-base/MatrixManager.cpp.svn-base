#include "MatrixManager.h"

#include <string.h>
#include <vector>
#include <algorithm>

#include <errno.h>
#include <dirent.h>
#include <sys/types.h>

#include "op/oped/noah/sailor/utility/Logger.h"
#include "inf/computing/matrix/matrix-agent/configure.h"
#include "op/oped/noah/sailor/utility/thread/ReadLocker.h"
#include "op/oped/noah/sailor/utility/thread/WriteLocker.h"

namespace matrix {
MatrixManager::MatrixManager(Executor& executor): _freezed(false),
                             _current_generation(0), _expect_list(NULL), _executor(executor)
{
	assert(g_config);
    
    set_interval(g_config->DEFAULT_MANAGER_GC_INTERVAL * 1000);
    set_name("MatrixManager");
    
    if (!load_instance_from_file()) {
        throw std::runtime_error("failed to load instance from file.");
    }
}

MatrixManager::~MatrixManager()
{
    stop();
    ::sailor::WriteLocker instance_locker(&_instance_lock);
    ::sailor::WriteLocker expect_locker(&_expect_lock);
    
    // delete instance
    clear_instance_list();
    
    // delete expect
    if (_expect_list != NULL) {
        delete _expect_list;
        _expect_list = NULL;
    }
}

void MatrixManager::run()
{
    if (_freezed) {
        LOG.info("MatrixManager is in freeze mode, skip process expect");
        return;
    }
    
    process_instance_expect();
    process_timeo();
}

bool MatrixManager::get_monitor_instance_list(std::map<uint64_t, InstanceInfo>* instance_list)
{
    ::sailor::ReadLocker instance_locker(&_instance_lock);
    assert(instance_list);
    instance_list->clear();
    for (std::map<std::string, Instance*>::const_iterator iter = _instance_list.begin();
         iter != _instance_list.end(); ++ iter)
    {
        Instance* slot = iter->second;
        const InstanceInfo& info = slot->get_instance_info();
        InstanceInfo::InstanceState::type state = info.get_instance_state();
        if (state == InstanceInfo::InstanceState::STARTING ||
            state == InstanceInfo::InstanceState::RUNNING  ){
            // STARTING or RUNNING
            instance_list->insert(std::pair<uint64_t, InstanceInfo>(slot->get_generation(), info));
        }
        else if (state == InstanceInfo::InstanceState::ERROR                        && 
                 (info.get_error_code() == InstanceInfo::ErrorCode::HEALCHECK_FAIL   ||
                 info.get_error_code() == InstanceInfo::ErrorCode::HEALCHECK_TIMEOUT ))
        {
            // ERROR but because of HEALCHECK
            instance_list->insert(std::pair<uint64_t, InstanceInfo>(slot->get_generation(), info));
        }
    }
    return true;
}

void MatrixManager::report_instance_status(const std::string& instance_id, 
                                           const std::string& meta_version,
                                           InstanceInfo::HealthCheckStatus::type status)
{
    ::sailor::ReadLocker instance_locker(&_instance_lock);
    std::map<std::string, Instance*>::iterator iter= _instance_list.find(instance_id);
    if (iter != _instance_list.end()) {
        const InstanceInfo& info = iter->second->get_instance_info();
        const InstanceMeta& meta = info.get_instance_meta();
        if (meta._meta_version == meta_version) {
            iter->second->on_health_check(status);
        } else {
            LOG.trace("MatrixManager: meta_version(monitor [%s]; manager [%s]) not match for instance [%s];", 
                      meta_version.c_str(),
                      meta._meta_version.c_str(),
                      instance_id.c_str());
        }
    }
}

void MatrixManager::report_machine_status(HostInfo::HostState::type state)
{
    _host_info.set_host_state(state);
}

void MatrixManager::report_machine_resource(const ResourceTuple& resource)
{
    _host_info.set_total_resource(resource);
}

void MatrixManager::report_install_cmd_res(uint64_t generation, bool ok)
{
    ::sailor::ReadLocker instance_locker(&_instance_lock);
    
    for (std::map<std::string, Instance*>::iterator iter = _instance_list.begin();
         iter != _instance_list.end(); ++ iter)
    {
        if (generation == iter->second->get_generation()) {
            Instance* slot = iter->second;
            if (ok) {
                slot->on_install(InstanceInfo::ErrorCode::SUCCESS);
            } else {
                slot->on_install(InstanceInfo::ErrorCode::DEPLOY_FAIL);
            }
            break;
        }
    }
}

void MatrixManager::report_remove_cmd_res(uint64_t generation, bool ok)
{
    ::sailor::WriteLocker instance_locker(&_instance_lock);
    
    for (std::map<std::string, Instance*>::iterator iter = _instance_list.begin();
         iter != _instance_list.end(); ++ iter)
    {
        if (generation == iter->second->get_generation()) {
            Instance* slot = iter->second;
            if (ok) {
                // remove ok
                ResourceTuple res = slot->get_instance_info().get_instance_meta()._resource;
                std::string instance_name = slot->get_instance_info().get_instance_name();

                slot->on_remove(InstanceInfo::ErrorCode::SUCCESS);
                LOG.info("MatrixManager: remove Instance [%s] success.", instance_name.c_str());
                _host_info.release_resource(res);   // release resource.
                delete slot;
                _instance_list.erase(instance_name);
            } else {
                slot->on_remove(InstanceInfo::ErrorCode::REMOVE_FAIL);
            }
            break;
        }
    }
}

bool MatrixManager::get_all_instance_info(std::vector<InstanceInfo> * instances)
{
    ::sailor::ReadLocker instance_locker(&_instance_lock);
    assert(instances);
    instances->clear();
    for (std::map<std::string, Instance*>::const_iterator iter = _instance_list.begin();
         iter != _instance_list.end(); ++ iter)
    {
        const InstanceInfo& info = iter->second->get_instance_info();
        instances->push_back(info);
    }
    return true;
}

void MatrixManager::push_instance_expect(const std::vector<InstanceInfo> & expect)
{
    ::sailor::WriteLocker expect_locker(&_expect_lock);
    if (_expect_list == NULL) {
        _expect_list = new std::vector<InstanceInfo>();
    }
    _expect_list->clear();
    *_expect_list = expect;
}

bool MatrixManager::get_host_info(HostInfo * host_info)
{
    ResourceTuple total_resource = _host_info.get_total_resource();
    if (!total_resource.is_valid() || total_resource.is_empty()) {
        LOG.info("MatrixManager: local host resource is not known.");
        return false;
    }
    if (_host_info.get_host_state() == HostInfo::HostState::INVALID) {
        LOG.info("MatrixManager: local host state is not known.");
        return false;
    }
    
    *host_info = _host_info;
    return true;
}

void MatrixManager::set_reserved_resource(const ResourceTuple & res)
{
    _host_info.set_reserved_resource(res);
}

bool MatrixManager::get_freeze()
{
    return _freezed;
}

void MatrixManager::set_freeze(bool f)
{
    _freezed = f;
}

bool MatrixManager::reset_instance(const std::string& instance_name)
{
    ::sailor::WriteLocker instance_locker(&_instance_lock);
    if (!_freezed) {
        LOG.info("MatrixManager: reset can only used in freeze mode.");
        return false;
    }
    
    std::map<std::string, Instance*>::iterator iter = _instance_list.find(instance_name);
    if (iter == _instance_list.end()) {
        LOG.info("MatrixManager: failed to reset instance [%s] because not found.", instance_name.c_str());
        return false;
    }
    
    iter->second->reset_to_running();
    return true;
}

/**
 * @brief process instance from actual to expect.
 *    expect         actual          action
 *    yes            no              Install
 *    yes            yes     same    Nothing
 *    yes            yes     no-same Update
 *    no             yes             remove
 */
void MatrixManager::process_instance_expect()
{
    if (!_host_info.is_ready()) {
        LOG.info("HostInfo is INVALID, ignore processing");
        return;
    }
    
    if (_expect_list == NULL) {
        LOG.info("No EXPECT received. ignore processing");
        return;
    }
    
    ::sailor::ReadLocker read_locker(&_expect_lock);
    ::sailor::WriteLocker write_locker(&_instance_lock);
    
    // generate tmp_instance_list, to find diff between _expect_list and _instance_list.
    std::map<std::string, Instance *> tmp_instace_list(_instance_list);
    
    for (std::vector<InstanceInfo>::iterator expect = _expect_list->begin();
         expect != _expect_list->end(); ++ expect)
    {
        std::map<std::string, Instance *>::iterator iter = _instance_list.find(expect->get_instance_name());
        if (iter == _instance_list.end()) {
            // not in local
            InstanceMeta meta = expect->get_instance_meta();
            // has enough resource
            if (!_host_info.has_enough_resource(meta._resource)) {
                LOG.warn("MatrixManager: no enough resource or for instance[%s], \nneed resource: %s, \nhost info: %s",
                         expect->get_instance_name().c_str(), meta._resource.to_json().toStyledString().c_str(),
                         _host_info.to_json().toStyledString().c_str());
                continue;
            }
            Instance * slot = new Instance(_current_generation, expect->get_service_name(), expect->get_offset());
            // do state transformation
            if (!slot->do_install(meta)) {
                LOG.warn("MatrixManager: ignore install");
                delete slot;
                continue;
            }
            _instance_list.insert(std::pair<std::string, Instance *>(expect->get_instance_name(), slot));
            _host_info.use_resource(expect->get_instance_meta()._resource);
            ++ _current_generation;
            // call executor to install
            _executor.install(slot->get_instance_info(), slot->get_generation());
            LOG.trace("MatrixManager: add new Instance [%s] success.", slot->get_instance_info().get_instance_name().c_str());
        }
        else {
            // instance is already in local
            tmp_instace_list.erase(expect->get_instance_name());
            
            Instance * slot = iter->second;
            const InstanceInfo& instance_info = slot->get_instance_info();
            if (instance_info.get_meta_version() == expect->get_meta_version()) {
                // local and expect is same, ignore it.
                continue;
            }
            
            // local and expect is not same, update it.
            if (!slot->do_update(expect->get_instance_meta())) {
                 LOG.info("MatrixManager: ignore update of instance %s.", iter->first.c_str());
                 continue;
            }
            
            // call executor to do update.
            _executor.update(slot->get_instance_info(), slot->get_generation());
            
            LOG.trace("MatrixManager: update Instance [%s] success.", slot->get_instance_info().get_instance_name().c_str());
        }
    }
    
    // find instances that in local but not in expect. and then remove it.
    for (std::map<std::string, Instance *>::iterator iter = tmp_instace_list.begin();
        iter != tmp_instace_list.end(); ++ iter)
    {
        Instance * slot = iter->second;
        if (!slot->do_remove()) {
            continue;
        }
        
        _executor.remove(slot->get_instance_info(), slot->get_generation());
        
        LOG.trace("MatrixManager: remove Instance [%s] success.", slot->get_instance_info().get_instance_name().c_str());
    }
    
    
}

void MatrixManager::process_timeo()
{
    ::sailor::WriteLocker write_locker(&_instance_lock);
    
    for (std::map<std::string, Instance*>::iterator iter = _instance_list.begin(); 
         iter != _instance_list.end(); ++ iter)
    {
        Instance* slot = iter->second;
        if (slot->check_timeo()) {
            _executor.stop(slot->get_generation());
        }
    }
}

bool MatrixManager::load_instance_from_file()
{
    assert(g_config);
    
    // private so not need to lock.
    if (!_instance_list.empty()) {
        LOG.warn("MatrixManager: instance_list is not empty, could not load from file.");
    }
    _host_info.init_used_resource();
    _current_generation = 0;
    
    DIR * dir = NULL;
    struct dirent dire, *dirp = NULL;
    bool ret = true;
    errno = 0;
    
    std::vector<uint64_t> tmp_generation_list;
    
    dir = opendir(g_config->META_PATH);
    if (NULL == dir) {
        LOG.error("MatrixManager: open dir %s fail: %d.", g_config->META_PATH, errno);
        return false;
    }
    memset(&dire, 0, sizeof(dire));
    errno = 0;
    while (readdir_r(dir, &dire, &dirp) == 0) {
        if (dirp == NULL) {
            LOG.trace("MatrixManager: load_instance_from_file no more entry to read");
            break;
        }
        std::string name = dire.d_name;
        if (name == "." || name == "..") {
            LOG.trace("MatrixManager: load_instance_from_file skip %s", name.c_str());
            continue;
        }
        
        LOG.info("MatrixManager: load_instance_from_file %s.", name.c_str());
        Instance* slot = NULL;
        try {
            slot = new Instance(name);
        }
        catch (std::exception& e) {
            LOG.error("MatrixManager: failed to load instance: %s", e.what());
            ret = false;
            break;
        }
        const InstanceInfo& instance_info = slot->get_instance_info();
        
        // check dup instance_name
        if (_instance_list.find(instance_info.get_instance_name()) != _instance_list.end()) {
            LOG.error("MatrixManager: dup service meta find (dup instance name) %s", name.c_str());
            delete slot;
            ret = false;
            break;
        }
        
        // check dup generation
        if (std::find(tmp_generation_list.begin(), tmp_generation_list.end(), slot->get_generation())
            != tmp_generation_list.end())
        {
            LOG.error("MatrixManager: dup service meta find (dup generation) %s", name.c_str());
            delete slot;
            ret = false;
            break;
        }
        
        _instance_list.insert(std::pair<std::string, Instance*>(instance_info.get_instance_name(), slot));
        _host_info.use_resource(instance_info.get_instance_meta()._resource);
        tmp_generation_list.push_back(slot->get_generation());
        if (slot->get_generation() >= _current_generation) {
            _current_generation = slot->get_generation() + 1;
        }
    }
    if (errno != 0) {
        ret = false;
        LOG.error("MatrixManager: failed to read dir %s, errno: %d", g_config->META_PATH, errno);
    }
    closedir(dir);
    
    if (!ret) {
        clear_instance_list();
        _host_info.init_used_resource();
        _current_generation = 0;
    }
    return ret;
}

void MatrixManager::clear_instance_list()
{
    // private so not need to lock
    LOG.trace("MatrixManager: clear_instance_list...");
    for (std::map<std::string, Instance*>::iterator iter = _instance_list.begin();
         iter != _instance_list.end(); ++ iter)
    {
        delete iter->second;
    }
    _instance_list.clear();
}

} // endof namespace matrix
