#include <cassert>

#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/monitor/InstanceMonitor.h"

#include "inf/computing/matrix/matrix-agent/executor/Executor.h"

namespace matrix {

MatrixExecutorCallback::MatrixExecutorCallback()
{
}


MatrixExecutorCallback::~MatrixExecutorCallback()
{
}

void MatrixExecutorCallback::set_manager(MatrixManager* manager)
{
    _manager = manager;
}

void MatrixExecutorCallback::set_monitor(InstanceMonitor* monitor)
{
    _monitor = monitor;
}

void MatrixExecutorCallback::on_deploy(uint64_t generation, bool succeeded)
{
    assert(_manager);
    _manager->report_install_cmd_res(generation, succeeded);
}

void MatrixExecutorCallback::on_remove(uint64_t generation, bool succeeded) 
{
    assert(_manager);
    _manager->report_remove_cmd_res(generation, succeeded);
}

void MatrixExecutorCallback::on_status(uint64_t generation, 
               InstanceInfo::HealthCheckStatus::type status)
{
    assert(_monitor);
    _monitor->report_instance_status(generation, status);
}

}
