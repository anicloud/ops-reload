#include <sys/types.h>
#include <sys/time.h>

#include <algorithm>
#include <string>
#include <sstream>
#include <stdexcept>

#include "op/oped/noah/sailor/utility/Logger.h"
#include "op/oped/noah/sailor/utility/Cgroups.h"
#include "op/oped/noah/sailor/utility/thread/MutexLocker.h"

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/common/Utility.h"
#include "inf/computing/matrix/matrix-agent/manager/Instance.h"

#include "inf/computing/matrix/matrix-agent/monitor/InstanceMonitor.h"

namespace matrix {

using std::string;

InstanceMonitor::InstanceMonitor(MatrixManager& manager, Executor& executor) :
    MatrixThread("MatrixInstanceMonitor", g_config->DEFAULT_INSTANCE_MONITOR_INTERVAL * 1000), 
    _manager(manager), 
    _executor(executor),
    _cgroup(g_config->CGROUP_ROOT)
{
    LOG.trace("InstanceMonitor is constructed.");
}

InstanceMonitor::~InstanceMonitor()
{
    stop();
    LOG.trace("InstanceMonitor is destroyed.");
}

void InstanceMonitor::report_instance_status(uint64_t generation, 
                                             InstanceInfo::HealthCheckStatus::type status) 
{
    ::sailor::MutexLocker locker(&_instance_mutex);

    // make decode generation back.
    generation = decode_generation(generation);

    LOG.info("%s: executor report status [%s] for generation [%lu].",
             _name.c_str(),
             InstanceInfo::HealthCheckStatus::to_string(status).c_str(),
             generation);

    // find in on checking instances.
    std::map<uint64_t, OnCheckingInstance>::iterator itr = on_checking_instances.find(generation);
    if (itr != on_checking_instances.end()) {

        const InstanceInfo& instance = itr->second._instance;
        const InstanceMeta& meta = instance.get_instance_meta();

        switch (status) {
        case InstanceInfo::HealthCheckStatus::STARTING:
            // 
            break;
        case InstanceInfo::HealthCheckStatus::ERROR:
            //
            break;
        case InstanceInfo::HealthCheckStatus::RUNNING: 
            status = basic_health_check(instance) ? InstanceInfo::HealthCheckStatus::RUNNING :
                                                    InstanceInfo::HealthCheckStatus::ERROR;
            break;
        default:
            LOG.error("%s: unknown health check status [%d].",
                      _name.c_str(), status);
            throw std::runtime_error("unknown health check status.");
            break;
        }

        LOG.info("%s: report status [%s] for instance [%s] to manager.",
                 _name.c_str(), 
                 InstanceInfo::HealthCheckStatus::to_string(status).c_str(),
                instance.get_instance_name().c_str());

        _manager.report_instance_status(instance.get_instance_name(),
                                        meta._meta_version,
                                        status);

        // remove from on_checking.
        LOG.trace("%s: remove instance [%s] from on checking list.",
                  _name.c_str(), instance.get_instance_name().c_str());

        on_checking_instances.erase(itr);

    } else {
        LOG.trace("%s: generation [%lu] not in on checking list, may have been removed due to timeout.",
                  _name.c_str(), generation);
    }
}

bool InstanceMonitor::basic_health_check(const InstanceInfo& instance)
{
    const InstanceMeta& instance_meta = instance.get_instance_meta();
    const std::string instance_id = instance.get_instance_name();

    //get lintening port /proc/net/tcp(6)
    std::map<int32_t, uint64_t> port_fd_list;
    get_port_and_fd_list(&port_fd_list);

    // get all pids in cgroup.
    sailor::Cgroups::Group g = _cgroup.group(instance_id);
    std::list<pid_t> pid_in_cgroup;
    if (!g.get_processes(&pid_in_cgroup)) {
        LOG.info("%s: get processes in cgroup error, instance_id: %s",
                 _name.c_str(), instance_id.c_str());
        return false;
    }

    //monitor process name
    const std::map<string, string>& process_to_monitor = instance_meta._process_name;
    if (!monitor_process(instance_id, process_to_monitor, pid_in_cgroup)) {
        LOG.info("%s: monitor process name NOT ok", _name.c_str());
        return false;
    }

    //monitor port
    const std::map<string, int32_t>& ports_to_monitor = instance_meta._port;
    if(!monitor_port(instance_id, ports_to_monitor, port_fd_list, pid_in_cgroup)) {
        LOG.info("%s: monitor port NOT ok", _name.c_str());
        return false;
    }
    return true;
}

void InstanceMonitor::check_timeout()
{
    // get current time.
    struct timeval now;
    if (gettimeofday(&now, NULL) != 0) {
        LOG.error("%s: gettimeofday failed.", _name.c_str());
        return;
    }

    // loop on checking instances.
    std::map<uint64_t, OnCheckingInstance>::iterator itr = on_checking_instances.begin();

    while (itr != on_checking_instances.end()) {

        const OnCheckingInstance& checking = itr->second; 
        const InstanceInfo& instance = checking._instance;
        const InstanceMeta& meta = instance.get_instance_meta();
        // health check timeout.
        if (now.tv_sec - checking._start_timestamp.tv_sec >= 
            instance.get_instance_meta()._health_check_timeout_sec) {

            LOG.info("%s: status timeout for instance [%s]", 
                     _name.c_str(),
                     instance.get_instance_name().c_str());

            // stop task in executor.
            LOG.trace("%s: call executor to stop timeout task.", _name.c_str());
            // TODO: uncomment this line later. wfl. _executor.stop(encode_generation(itr->first));

            // notify manager.
            LOG.trace("%s: notify manager timeout event.", _name.c_str());
            _manager.report_instance_status(instance.get_instance_name(), 
                                            meta._meta_version,
                                            InstanceInfo::HealthCheckStatus::TIMEOUT);

            // remove from on checking instances..
            LOG.trace("%s: remove from on checking instances.", _name.c_str());
            on_checking_instances.erase(itr++);
        } else {
            ++itr;
        }

    }

}


void InstanceMonitor::run()
{

    LOG.info("%s: start to monitor instance status", _name.c_str());

    // get instances that need check from manager.
    std::map<uint64_t, InstanceInfo> instances;
    if (!_manager.get_monitor_instance_list(&instances)) {
        LOG.error("%s: get instances to monitor failed.", _name.c_str());
        return;
    }

    // acquire mutex before check.
    ::sailor::MutexLocker locker(&_instance_mutex);

    // loop instances that need check.
    for (std::map<uint64_t, InstanceInfo>::const_iterator itr = instances.begin();
         itr != instances.end(); ++itr) {

        uint64_t generation = itr->first;
        const InstanceInfo& instance = itr->second;
        std::string instance_id = instance.get_instance_name();

        LOG.debug("%s: monitor instance [%s]", _name.c_str(), instance_id.c_str()); 

        // skip instance that already is on checking.
        if (on_checking_instances.find(generation) == on_checking_instances.end()) {

            // ask executor to execute status
            LOG.trace("%s: call executor to execute status", _name.c_str());
            _executor.status(instance, encode_generation(generation));

            // add current instance to on checking instances.
            LOG.trace("%s: add instance [%s] to on checking list.",
                      _name.c_str(), instance.get_instance_name().c_str());

            struct timeval now;
            if (gettimeofday(&now, NULL) == 0) {

                OnCheckingInstance checkingInstance(instance, now);
                on_checking_instances.insert(std::make_pair(generation, checkingInstance));
            }
        } else {
            LOG.trace("%s: skip on checking instance [%s]", 
                      _name.c_str(),
                      instance_id.c_str());
        }
    }

    // check status timeout.
    check_timeout();

    LOG.info("%s: finish monitor instance status", _name.c_str());
}

bool InstanceMonitor::monitor_process(const string& instance_id, 
        const std::map<string, string>& process_to_monitor,
        const std::list<pid_t>& pids)
{
    //get process name list
    std::list<string> process_in_cgroup;
    get_process_names(pids, &process_in_cgroup);

    //check process name exists.
    for (std::map<string, string>::const_iterator iter = process_to_monitor.begin();
            iter != process_to_monitor.end(); ++iter) {
        const string& process_name = iter->second;
        if (std::find(process_in_cgroup.begin(), process_in_cgroup.end(), process_name)
                == process_in_cgroup.end()) {
            LOG.info("%s: Instance %s has no process %s.", _name.c_str(), 
                    instance_id.c_str(), process_name.c_str());
            return false;
        }
    }

    return true;
}

bool InstanceMonitor::monitor_port(const string& instance_id,
        const std::map<string, int32_t>& ports_to_monitor, 
        const std::map<int32_t, uint64_t>& port_fd_list,
        const std::list<pid_t>& pids)
{
    std::list<uint64_t> fds;

    for (std::map<string, int32_t>::const_iterator iter = ports_to_monitor.begin();
            iter != ports_to_monitor.end(); ++iter) {

        LOG.debug("%s: try to monitor port %d", _name.c_str(), iter->second);

        // check port is here
        std::map<int32_t, uint64_t>::const_iterator pos = port_fd_list.find(iter->second);
        if (pos == port_fd_list.end()) {
            LOG.info("%s: Instance %s has no port %d.", _name.c_str(), 
                    instance_id.c_str(), iter->second);
            return false;
        }

        // check port is in process
        if (fds.empty()) {
            // do not get twice
            get_socket_fd(pids, &fds);
        }

        if (std::find(fds.begin(), fds.end(), pos->second) == fds.end()) {
            LOG.info("%s: port %d is not belong to %s.", _name.c_str(), iter->second, 
                    instance_id.c_str());
            return false;
        }
    }

    return true;
}

uint64_t InstanceMonitor::decode_generation(const uint64_t& generation)
{
    return generation & 0x7FFFFFFFFFFFFFFF;
}

uint64_t InstanceMonitor::encode_generation(const uint64_t& generation)
{
    return generation | 0x8000000000000000;
}

} /// namespace matrix
