#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_MATRIX_MANAGER_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_MATRIX_MANAGER_H__

#include <map>
#include <vector>

#include <sys/types.h>

#include "op/oped/noah/sailor/utility/thread/ReadWriteLock.h"

#include "inf/computing/matrix/matrix-agent/manager/Instance.h"
#include "inf/computing/matrix/matrix-agent/manager/HostInfo.h"
#include "inf/computing/matrix/matrix-agent/executor/Executor.h"
#include "inf/computing/matrix/matrix-agent/common/MatrixThread.h"

namespace matrix {

class MatrixManager : public MatrixThread
{
public:
    explicit MatrixManager(Executor& executor);
    ~MatrixManager();
	
    void run();
    
public:
    // for instance monitor
    virtual bool get_monitor_instance_list(std::map<uint64_t, InstanceInfo>* instance_list);
    virtual void report_instance_status(const std::string& instance_id, 
                                        const std::string& meta_version,
                                        InstanceInfo::HealthCheckStatus::type status);
    
    // for machine monitor
    virtual void report_machine_status(HostInfo::HostState::type state);
    virtual void report_machine_resource(const ResourceTuple& resource);
    
    // for executor callback
    virtual void report_install_cmd_res(uint64_t generation, bool ok);
    virtual void report_remove_cmd_res(uint64_t generation, bool ok);
    
    // for reportor
    virtual bool get_all_instance_info(std::vector<InstanceInfo>* instances);
    virtual void push_instance_expect(const std::vector<InstanceInfo>& expect);
    virtual bool get_host_info(HostInfo * host_info);
    virtual void set_reserved_resource(const ResourceTuple & res);
    
    // for agent control
    bool get_freeze();
    void set_freeze(bool f);
    bool reset_instance(const std::string& instance_name);
    
private:
    void process_instance_expect();
    void process_timeo();
    bool load_instance_from_file();
    void clear_instance_list();

private:

    bool _freezed;
    uint64_t _current_generation; // load instance 表之后，设置为最大的generation + 1；
	
    /*
     * _expect_list is a point bucause it can be used to
     * judge whether is initialized.
     */
	std::vector<InstanceInfo> * _expect_list;
	std::map<std::string, Instance *> _instance_list;
	HostInfo _host_info;
    
    ::sailor::ReadWriteLock _expect_lock;
    ::sailor::ReadWriteLock _instance_lock;
    
    Executor& _executor;
};  // class MatrixManager

}   // namespace matrix

#endif   //__INF_COMPUTING_MATRIX_MATRIX_AGENT_MANAGER_MATRIX_MANAGER_H__
