#include <unistd.h>

#include <cstdlib>
#include <string>

#include <gtest/gtest.h>

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/reporter/MatrixReporter.h"
#include "inf/computing/matrix/matrix-agent/executor/Executor.h"
#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/rpc/MatrixRpc.h"
#include "inf/computing/matrix/matrix-agent/httpd/MatrixHttpd.h"

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
        InstanceInfo instance("www.baidu.com", 0);
        instance.set_error_code(InstanceInfo::ErrorCode::SUCCESS);
        instance.set_instance_state(InstanceInfo::InstanceState::RUNNING);
        InstanceMeta meta;
        meta._deploy_dir = "/home/mapred/ark";
        meta._deploy_timeout_sec = 600;
        meta._health_check_timeout_sec = 5;
        meta._group = "ark";
        meta._min_port_include = 8000;
        meta._max_port_include = 9000;
        meta._meta_version = "1.1.21";
        meta._package_source = "ftp://ark.baidu.com/package/ark_1-1-21.tgz";
        meta._package_version = "1.1.21";
        meta._package_type = InstanceMeta::PackageType::IMCI;
        meta._port.insert(std::make_pair("agent_rpc_port", 8400));
        meta._process_name.insert(std::make_pair("main", "java"));
        meta._tag.insert(std::make_pair("resource_node", "abaci.appmaster"));
        meta._resource._cpu_num = 10;
        meta._resource._memory_mb = 512;
        meta._resource._disk_total_mb = 1024;
        instance.set_instance_meta(meta);
        _instances.push_back(instance);
    }
    bool get_all_instance_info(std::vector<InstanceInfo>* instances) 
    {
        *instances = _instances;
        return true;
    }
    void push_instance_expect(const std::vector<InstanceInfo>& expect) 
    {
        _expect_instances = expect;
    }
    bool get_host_info(HostInfo * host_info)
    {
        *host_info = _host_info;
        return true;
    }
    void set_reserved_resource(const ResourceTuple& res)
    {
        _reserved_resource = res;
    }

    std::vector<InstanceInfo> _instances;
    std::vector<InstanceInfo> _expect_instances;
    HostInfo _host_info;
    ResourceTuple _reserved_resource;
};

class MockRpc : public MatrixMasterRpc
{
public:
    MockRpc() : _p_msg(NULL),
                _master_safe_mode(false),
                _heartbeat_interval_sec(2),
                _agent_http_port(80)

    {
        _reserved_resource._cpu_num = 10;
        _reserved_resource._memory_mb = 1024;
        _reserved_resource._disk_total_mb = 10240;
        //
        InstanceInfo instance("www.baidu.com", 1);
        instance.set_error_code(InstanceInfo::ErrorCode::SUCCESS);
        instance.set_instance_state(InstanceInfo::InstanceState::RUNNING);
        InstanceMeta meta;
        meta._deploy_dir = "/home/mapred/ark";
        meta._deploy_timeout_sec = 600;
        meta._health_check_timeout_sec = 5;
        meta._group = "ark";
        meta._max_port_include = 9000;
        meta._min_port_include = 8000;
        meta._meta_version = "1.1.21";
        meta._package_source = "ftp://ark.baidu.com/package/ark_1-1-21.tgz";
        meta._package_version = "1.1.21";
        meta._package_type = InstanceMeta::PackageType::IMCI;
        meta._port.insert(std::make_pair("agent_rpc_port", 8400));
        meta._process_name.insert(std::make_pair("main", "java"));
        meta._tag.insert(std::make_pair("resource_node", "abaci.appmaster"));
        meta._resource._cpu_num = 10;
        meta._resource._memory_mb = 512;
        meta._resource._disk_total_mb = 1024;
        instance.set_instance_meta(meta);
        _expect_instances.push_back(instance);

    }
    ~MockRpc()
    {
        if (_p_msg != NULL)
        {
            delete _p_msg;
            _p_msg = NULL;
        }
    }
    HeartbeatResponse heartbeat(const HeartbeatMessage& msg) throw (MatrixRpcException)
    {
        if (_p_msg != NULL) {
            delete _p_msg;
            _p_msg = NULL;
        }
        _p_msg = new HeartbeatMessage(msg);
        AgentConf agent_conf(_master_safe_mode,
                             _heartbeat_interval_sec,
                             _agent_http_port,
                             _reserved_resource);
        return HeartbeatResponse(agent_conf, _expect_instances);
    }

    // HeartbeatMessage is not mutable, so we have to use pointer here.
    HeartbeatMessage* _p_msg;
    bool _master_safe_mode;
    int32_t _heartbeat_interval_sec;
    int32_t _agent_http_port;
    ResourceTuple _reserved_resource;
    std::vector<InstanceInfo> _expect_instances;
};

class MockHttpd : public MatrixHttpd
{
public:
    MockHttpd() : _agent_http_port(80)
    {
    }
    bool start(int32_t listen_port = 0)
    {
        if (listen_port != 0) {
            _agent_http_port = listen_port;
        }
        return true;
    }
    void stop()
    {
    }
    int32_t get_listen_port() const
    {
        return _agent_http_port;
    }
    int32_t _agent_http_port;
};

class MatrixReporterTest : public testing::Test 
{

public:
    MatrixReporterTest() : _executor(_callback), _manager(NULL), _reporter(NULL)
    {
    }

protected:
    virtual void SetUp()
    {
        system("rm -rf /tmp/matrix && mkdir -p /tmp/matrix/meta");
        g_mutable_config.DEFAULT_HEARTBEAT_INTERVAL = 2;
        g_mutable_config.DEFAULT_MANAGER_GC_INTERVAL = 2;
        g_mutable_config.META_PATH = "/tmp/matrix/meta";
        g_config = &g_mutable_config;
        //
        _manager = new MockManager(_executor);
        _reporter = new MatrixReporter(*_manager, _rpc, _httpd);
        _reporter->start();
    }

    virtual void TearDown()
    {
        if (_reporter != NULL) {
            _reporter->stop();
            delete _reporter;
            _reporter = NULL;
        }
        if (_manager != NULL) {
            delete _manager;
            _manager = NULL;
        }
        system("rm -rf /tmp/matrix");
    }

    MockExecutorCallback _callback;
    MockExecutor _executor;
    MockRpc _rpc;
    MockHttpd _httpd;
    //
    MockManager* _manager;
    MatrixReporter* _reporter;

};


TEST_F(MatrixReporterTest, Normal)
{
    sleep(1);
    EXPECT_EQ(_manager->_expect_instances.size(), (size_t)1);
    EXPECT_EQ(_rpc._p_msg->_first_heartbeat, true);
    sleep(3);
    EXPECT_EQ(_manager->_expect_instances.size(), (size_t)1);
    EXPECT_EQ(_rpc._p_msg->_first_heartbeat, false);
}

TEST_F(MatrixReporterTest, ChangeHeartbeatInterval)
{
    // default heartbeat interval is 2s.
    EXPECT_EQ(_reporter->get_interval(), 2000);
    // change heartbeat interval.
    _rpc._heartbeat_interval_sec = 5;
    sleep(3);
    EXPECT_EQ(_reporter->get_interval(), 5000);
    _rpc._heartbeat_interval_sec = 1;
    sleep(6);
    EXPECT_EQ(_reporter->get_interval(), 1000);
}

TEST_F(MatrixReporterTest, ChangeResource)
{
    sleep(3);
    EXPECT_EQ(_manager->_reserved_resource._cpu_num, 10);
    EXPECT_EQ(_manager->_reserved_resource._memory_mb, 1024);
    EXPECT_EQ(_manager->_reserved_resource._disk_total_mb, 10240);
    _rpc._reserved_resource._cpu_num = 20;
    sleep(3);
    EXPECT_EQ(_manager->_reserved_resource._cpu_num, 20);
    EXPECT_EQ(_manager->_reserved_resource._memory_mb, 1024);
    EXPECT_EQ(_manager->_reserved_resource._disk_total_mb, 10240);
}

TEST_F(MatrixReporterTest, ChangeHttpd)
{
    sleep(3);
    EXPECT_EQ(_httpd.get_listen_port(), 80);
    _rpc._agent_http_port = 8080;
    sleep(3);
    EXPECT_EQ(_httpd.get_listen_port(), 8080);
}

TEST_F(MatrixReporterTest, ResponseInvalid)
{
    sleep(3);
    EXPECT_EQ(_manager->_expect_instances.size(), (size_t)1);

    // clear expect instances.
    std::vector<InstanceInfo> expect_instances = _rpc._expect_instances;
    _rpc._expect_instances.clear();
    sleep(3);
    EXPECT_EQ(_manager->_expect_instances.size(), (size_t)0);

    // restore expect instances.
    _rpc._expect_instances = expect_instances;
    // http port invalid.
    _rpc._agent_http_port = 0;
    sleep(3);
    EXPECT_EQ(_manager->_expect_instances.size(), (size_t)0);
    
    // safemode.
    _rpc._agent_http_port = 80;
    _rpc._master_safe_mode = true;
    sleep(3);
    EXPECT_EQ(_manager->_expect_instances.size(), (size_t)0);

    // heartbeat interval invalid.
    _rpc._master_safe_mode = false;
    _rpc._heartbeat_interval_sec = 0;
    sleep(3);
    EXPECT_EQ(_manager->_expect_instances.size(), (size_t)0);

    // instance invalid.
    _rpc._heartbeat_interval_sec = 2;
    _rpc._expect_instances[0].set_instance_state(InstanceInfo::InstanceState::ERROR);
    sleep(3);
    EXPECT_EQ(_manager->_expect_instances.size(), (size_t)0);
}
