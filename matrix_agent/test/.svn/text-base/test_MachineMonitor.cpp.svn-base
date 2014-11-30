#include <unistd.h>

#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

#include "op/oped/noah/sailor/utility/Logger.h"

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/executor/Executor.h"
#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/monitor/MachineMonitor.h"

using namespace matrix;

static struct Configure g_mutable_config;

class MockExecutorCallback : public Executor::Callback {
public:

    void on_deploy(uint64_t generation, bool succeeded)
    {
    }

    void on_remove(uint64_t generation, bool succeeded)
    {
    }

    void on_status(uint64_t generation, 
                   InstanceInfo::HealthCheckStatus::type status)
    {
    }
};

class MockExecutor : public Executor
{
public:
    MockExecutor(MockExecutorCallback& cb) : Executor(cb) {}

    void install(const InstanceInfo &info, uint64_t generation) 
    {
    }
    void update(const InstanceInfo &info, uint64_t generation) 
    {
    }
    void remove(const InstanceInfo &info, uint64_t generation) 
    {
    }
    void status(const InstanceInfo& info, uint64_t generation)
    {
    }
    void stop(uint64_t generation)
    {
    }
};

class MockManager : public MatrixManager
{
public:
    MockManager(MockExecutor& executor) : MatrixManager(executor) 
    {
    }
    void report_machine_status(HostInfo::HostState::type state)
    {
        _state = state;
    }
    void report_machine_resource(const ResourceTuple& resource)
    {
        _resource = resource;
    }

    HostInfo::HostState::type _state;
    ResourceTuple _resource;
};

class HangMachineMonitor : public MachineMonitor
{
public:
    HangMachineMonitor(MatrixManager& manager) : MachineMonitor(manager)
    {
    }

    void run() {
        MachineMonitor::run();
        if (_hang) {
            LOG.info("HangMachineMonitor: start hang!");
            sleep(10);
            LOG.info("HangMachineMonitor: finish hang!");
        }
    }

    void die() {
        LOG.error("HangMachineMonitor: I am immortal! Ha..Ha..Ha..");
    }
    bool _hang;
};

class MatrixReporterTest : public testing::Test 
{

public:
    MatrixReporterTest() : _executor(_callback), _manager(NULL), _monitor(NULL)
    {
    }

protected:
    virtual void SetUp()
    {
        system("chmod -R 777 /tmp/matrix > /dev/null 2>&1; rm -rf /tmp/matrix && mkdir -p /tmp/matrix/meta");

        g_mutable_config.DEFAULT_MACHINE_MONITOR_INTERVAL = 1;
        g_mutable_config.DEFAULT_MANAGER_GC_INTERVAL = 1;
        g_mutable_config.MATRIX_ROOT = "/tmp/matrix";
        g_mutable_config.META_PATH = "/tmp/matrix/meta";

        g_config = &g_mutable_config;

        _manager = new MockManager(_executor);
        _monitor = new MachineMonitor(*_manager);
        _monitor->start();
    }

    virtual void TearDown()
    {
        if (_monitor != NULL) {
            _monitor->stop();
            delete _monitor;
            _monitor = NULL;
        }
        if (_manager != NULL) {
            delete _manager;
        }
        system("chmod -R 777 /tmp/matrix > /dev/null 2>&1 && rm -rf /tmp/matrix");
    }

    MockExecutorCallback _callback;
    MockExecutor _executor;
    //
    MockManager* _manager;
    MachineMonitor* _monitor;

};


TEST_F(MatrixReporterTest, Normal)
{
    sleep(2);
    EXPECT_EQ(_manager->_state, HostInfo::HostState::AVAILABLE);
    EXPECT_EQ(_manager->_resource._cpu_num, _monitor->get_cpu_num());
    EXPECT_EQ(_manager->_resource._memory_mb, _monitor->get_total_memory());
    EXPECT_EQ(_manager->_resource._disk_total_mb, _monitor->get_total_disk());
}

TEST_F(MatrixReporterTest, DiskError)
{
    _monitor->stop();
    HangMachineMonitor hang_monitor(*_manager);
    hang_monitor._hang = false;
    hang_monitor.start();
    system("chmod -R 000 /tmp/matrix > /dev/null 2>&1");
    sleep(2);
    EXPECT_EQ(_manager->_state, HostInfo::HostState::FAILED);
    system("chmod -R 777 /tmp/matrix");
    sleep(2);
    EXPECT_EQ(_manager->_state, HostInfo::HostState::AVAILABLE);
}

TEST_F(MatrixReporterTest, MonitorThreadHang)
{
    _monitor->stop();
    HangMachineMonitor hang_monitor(*_manager);
    hang_monitor._hang = true;
    hang_monitor.start();
    sleep(10);
    EXPECT_EQ(_manager->_state, HostInfo::HostState::FAILED);
    hang_monitor._hang = false;
    sleep(2);
    EXPECT_EQ(_manager->_state, HostInfo::HostState::AVAILABLE);
}

