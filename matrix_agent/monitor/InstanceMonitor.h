#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_INSTANCE_MONITOR_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_INSTANCE_MONITOR_H__

#include <sys/types.h>

#include <map>

#include "op/oped/noah/sailor/utility/Cgroups.h"
#include "op/oped/noah/sailor/utility/thread/Mutex.h"

#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/executor/Executor.h"
#include "inf/computing/matrix/matrix-agent/common/MatrixThread.h"

namespace matrix {

/**
 * Monitor instance's life.
 *
 * Three aspects are checked:
 *     1. Process Port (RUNNING, ERROR)
 *     2. Process Name (RUNNING, ERROR)
 *     3. status.sh given by package (RUNNING, STARTING, ERROR).
 *        If status.sh is not given, always return RUNNING.
 *
 * If STARTING is got, report STARTING to manager.
 * If any check failed, report ERROR to manager.
 * If timeout happened(for status.sh), report TIMEOUT to manager.
 * When all checks are ok, report RUNNING to manager.
 *
 */
class InstanceMonitor : public MatrixThread
{
public:
    InstanceMonitor(MatrixManager& manager, Executor& executor);
    virtual ~InstanceMonitor();

    // for executor callback.
    // report the status checked by status.sh
    void report_instance_status(uint64_t generation, InstanceInfo::HealthCheckStatus::type status);

private:
    virtual void run();
    // check status timeout.
    // caller should aquire mutex.
    void check_timeout();
    // basic health check based on process name and port.
    bool basic_health_check(const InstanceInfo& instance);
    // monitor by process name.
    bool monitor_process(const std::string& instance_id, 
                         const std::map<std::string, std::string>& process_to_monitor,
                         const std::list<pid_t>& pids);
    // monitor by process port.
    bool monitor_port(const std::string& instance_id, 
                      const std::map<std::string, int32_t>& ports_to_monitor,
                      const std::map<int32_t, uint64_t>& port_fd_list,
                      const std::list<pid_t>& pids);

    /**
     * make highest bit of generation to 1 when pass to executor.
     * so install & update & remove cmd will not be blocked by status.sh
     */
    uint64_t encode_generation(const uint64_t& generation);
    uint64_t decode_generation(const uint64_t& generation);

private:
    MatrixManager& _manager;
    Executor& _executor;
    sailor::Cgroups  _cgroup;

    /**
     * Instance that is on checking.
     */
    struct OnCheckingInstance {
        OnCheckingInstance(const InstanceInfo& instance,
                           const struct timeval& start_timestamp) :
            _instance(instance), 
            _start_timestamp(start_timestamp) {}
        // orig instance.
        const InstanceInfo _instance;
        // use to handle timeout.
        const struct timeval _start_timestamp;
    };
    // generation -> OnCheckingInstance
    std::map<uint64_t, OnCheckingInstance> on_checking_instances;

    mutable ::sailor::Mutex _instance_mutex;
};

} /// namespace matrix

#endif ///__INF_COMPUTING_MATRIX_MATRIX_AGENT_INSTANCE_MONITOR_H__
