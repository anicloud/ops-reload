#include <unistd.h>

#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>

#include <gtest/gtest.h>

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/executor/Executor.h"
#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/monitor/InstanceMonitor.h"

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

    MockManager(MockExecutor& executor) : MatrixManager(executor),
                                          _instance_id(""),
                                          _status(InstanceInfo::HealthCheckStatus::ERROR)
    {
        _meta._deploy_dir = "/home/mapred/ark";
        _meta._deploy_timeout_sec = 600;
        _meta._health_check_timeout_sec = 5;
        _meta._group = "ark";
        _meta._min_port_include = 40000;
        _meta._max_port_include = 50000;
        _meta._meta_version = "1.1.21";
        _meta._package_source = "ftp://ark.baidu.com/package/ark_1-1-21.tgz";
        _meta._package_version = "1.1.21";
        _meta._package_type = InstanceMeta::PackageType::IMCI;
        _meta._port.insert(std::make_pair("nc", 45678));
        _meta._process_name.insert(std::make_pair("main", "nc"));
        _meta._tag.insert(std::make_pair("resource_node", "abaci.appmaster"));
        _meta._resource._cpu_num = 10;
        _meta._resource._memory_mb = 512;
        _meta._resource._disk_total_mb = 1024;
    }

    bool get_monitor_instance_list(std::map<uint64_t, InstanceInfo>* instance_list)
    {
        InstanceInfo instance("www.baidu.com", 0);
        instance.set_error_code(InstanceInfo::ErrorCode::SUCCESS);
        instance.set_instance_state(InstanceInfo::InstanceState::RUNNING);
        instance.set_instance_meta(_meta);
        instance_list->insert(std::make_pair(0, instance));
        return true;
    }
    void report_instance_status(const std::string& instance_id, 
                                const std::string& meta_version,
                                InstanceInfo::HealthCheckStatus::type status)
    {
        _instance_id = instance_id;
        _status = status;
        EXPECT_EQ(_meta._meta_version, meta_version);
    }

    std::string _instance_id;
    InstanceInfo::HealthCheckStatus::type _status;
    InstanceMeta _meta;
    
};

class InstanceMonitorTest : public testing::Test 
{

public:
    InstanceMonitorTest() : _executor(_callback), _manager(NULL), _monitor(NULL), _pid(0)
    {
    }

protected:
    virtual void SetUp()
    {
        ASSERT_TRUE(system("rm -rf /tmp/matrix") == 0);
        ASSERT_TRUE(system("mkdir -p /tmp/matrix/meta") == 0);
        // mock cgroup.
        ASSERT_TRUE(system("mkdir -p /tmp/matrix/cgroup/freezer/0.www.baidu.com") == 0);

        g_mutable_config.DEFAULT_INSTANCE_MONITOR_INTERVAL = 2;
        g_mutable_config.DEFAULT_MANAGER_GC_INTERVAL = 2;
        g_mutable_config.META_PATH = "/tmp/matrix/meta";
        g_mutable_config.CGROUP_ROOT = "/tmp/matrix/cgroup";

        g_config = &g_mutable_config;

        start_nc(45678);

        _manager = new MockManager(_executor);
        _monitor = new InstanceMonitor(*_manager, _executor);
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
        stop_nc();
        ASSERT_TRUE(system("rm -rf /tmp/matrix") == 0);

    }

    void start_nc(int32_t port)
    {
        char port_str[6];
        snprintf(port_str, sizeof(port_str), "%d", port);
        _pid = fork();
        ASSERT_TRUE(_pid >= 0);
        if (_pid == 0) {
            //child 
            char* argv[] = {"nc", "-l", "-p", port_str, NULL};
            execv("/usr/bin/nc", argv);
        }
        std::ostringstream cmd;
        cmd << "echo " << _pid << " > /tmp/matrix/cgroup/freezer/0.www.baidu.com/tasks";
        system(cmd.str().c_str());
        sleep(2);
    }

    void stop_nc()
    {
        if (_pid != 0) {
            kill( _pid, 9);
            _pid = 0;
            sleep(2);
        }
    }

    MockExecutorCallback _callback;
    MockExecutor _executor;
    //
    MockManager* _manager;
    InstanceMonitor* _monitor;
    //
    pid_t _pid;

};


TEST_F(InstanceMonitorTest, Normal)
{
    _monitor->start();
    sleep(2);
    // executor callback.
    _monitor->report_instance_status(0 | 0x8000000000000000,
                                     InstanceInfo::HealthCheckStatus::RUNNING);
    EXPECT_EQ(InstanceInfo::HealthCheckStatus::RUNNING, _manager->_status);
    EXPECT_EQ("0.www.baidu.com", _manager->_instance_id);
}

TEST_F(InstanceMonitorTest, PortError)
{
    stop_nc();
    start_nc(45679);
    _monitor->start();
    sleep(2);

    _monitor->report_instance_status(0 | 0x8000000000000000,
                                     InstanceInfo::HealthCheckStatus::RUNNING);
    EXPECT_EQ(InstanceInfo::HealthCheckStatus::ERROR, _manager->_status);
    EXPECT_EQ("0.www.baidu.com", _manager->_instance_id);
}

TEST_F(InstanceMonitorTest, ProcessNameError)
{
    _manager->_meta._process_name["main"] = "i_am_not_nc";
    _monitor->start();
    sleep(2);

    _monitor->report_instance_status(0 | 0x8000000000000000,
                                     InstanceInfo::HealthCheckStatus::RUNNING);
    EXPECT_EQ(InstanceInfo::HealthCheckStatus::ERROR, _manager->_status);
    EXPECT_EQ("0.www.baidu.com", _manager->_instance_id);
}


TEST_F(InstanceMonitorTest, Starting)
{
    _monitor->start();
    sleep(2);

    _monitor->report_instance_status(0 | 0x8000000000000000,
                                     InstanceInfo::HealthCheckStatus::STARTING);

    EXPECT_EQ(InstanceInfo::HealthCheckStatus::STARTING, _manager->_status);
    EXPECT_EQ("0.www.baidu.com", _manager->_instance_id);
}

TEST_F(InstanceMonitorTest, Timeout)
{
    _monitor->start();
    // wait for timeout.
    sleep(10);

    EXPECT_EQ(InstanceInfo::HealthCheckStatus::TIMEOUT, _manager->_status);
    EXPECT_EQ("0.www.baidu.com", _manager->_instance_id);
}

TEST_F(InstanceMonitorTest, UnknownGeneration)
{
    _monitor->start();
    // wait for timeout.
    sleep(2);

    _monitor->report_instance_status(1 | 0x8000000000000000,
                                     InstanceInfo::HealthCheckStatus::STARTING);
    EXPECT_EQ("", _manager->_instance_id);
}
