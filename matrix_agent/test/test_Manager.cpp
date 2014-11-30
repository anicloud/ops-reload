#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <gtest/gtest.h>

#include "op/oped/noah/sailor/utility/Logger.h"

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/executor/Executor.h"
#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"

using namespace matrix;

static struct Configure g_mutable_config;

class MockExecutorCallback : public Executor::Callback {
public:

    MockExecutorCallback(){}
    void on_deploy(uint64_t generation, bool succeeded)
    {
        _manager->report_install_cmd_res(generation, succeeded);
    }

    void on_remove(uint64_t generation, bool succeeded)
    {
        _manager->report_remove_cmd_res(generation, succeeded);
    }

    void on_status(uint64_t generation, 
                   InstanceInfo::HealthCheckStatus::type status)
    {
    }
    
    void set_manager(MatrixManager* manager)
    {
        _manager = manager;
    }
private:
    MatrixManager * _manager;
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

class MatrixManagerTest : public testing::Test
{
protected:
    virtual void SetUp()
    {
        g_mutable_config.META_PATH = "/tmp/matrix_meta";
        g_mutable_config.DEFAULT_MANAGER_GC_INTERVAL = 1;
        g_mutable_config.DEFAULT_MANAGER_REMOVE_TIMEO = 5;
        g_config = &g_mutable_config;
        
        system("mkdir /tmp/matrix_meta");
    }
    virtual void TearDown()
    {
        system("rm -rf /tmp/matrix_meta");
    }
};

static void get_instance_meta(InstanceMeta * meta)
{
    meta->_meta_version = "meta_version";
    meta->_package_source = "http://package.source";
    meta->_package_version = "";
    meta->_package_type = InstanceMeta::PackageType::ARCHER;
    meta->_deploy_dir = "/home/work";
    meta->_port = std::map<std::string, int32_t>();
    meta->_max_port_include = 1;
    meta->_min_port_include = 1;
    meta->_process_name = std::map<std::string, std::string>();
    meta->_tag = std::map<std::string, std::string>();;
    meta->_resource = ResourceTuple(10, 100, 100);
    meta->_deploy_timeout_sec = 10;
    meta->_health_check_timeout_sec = 10;
    meta->_group = "group";
    ASSERT_TRUE(meta->is_valid());
}

TEST_F(MatrixManagerTest, load_instance)
{
    
    Instance instance0(0, "test.baidu.jx", 0);
    Instance instance1(1, "test.baidu.jx", 1);
    Instance instance2(2, "test.baidu.jx", 2);
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    
    ASSERT_TRUE(instance0.do_install(meta));
    ASSERT_EQ(instance0.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance0.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    meta._meta_version = "meta_version_1";
    ASSERT_TRUE(instance1.do_install(meta));
    ASSERT_EQ(instance1.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance1.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance1.on_install(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance1.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance1.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    meta._meta_version = "meta_version_2222";
    ASSERT_TRUE(instance2.do_install(meta));
    ASSERT_EQ(instance2.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance2.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance2.on_install(InstanceInfo::ErrorCode::DEPLOY_FAIL);
    ASSERT_EQ(instance2.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance2.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::DEPLOY_FAIL);
    
    // let manager load them.
    MockExecutorCallback callback;
    MockExecutor executor(callback);
    MatrixManager manager(executor);
    
    std::vector<InstanceInfo> instance_list;
    manager.get_all_instance_info(&instance_list);
    ASSERT_EQ(instance_list.size(), (size_t)3);
    
    ASSERT_EQ(instance_list[0].get_offset(), 0ul);
    ASSERT_EQ(instance_list[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(instance_list[0].get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance_list[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_EQ(instance_list[1].get_offset(), 1ul);
    ASSERT_EQ(instance_list[1].get_instance_name(), "1.test.baidu.jx");
    ASSERT_EQ(instance_list[1].get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance_list[1].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance_list[1].get_instance_meta()._meta_version, "meta_version_1");
    
    ASSERT_EQ(instance_list[2].get_offset(), 2ul);
    ASSERT_EQ(instance_list[2].get_instance_name(), "2.test.baidu.jx");
    ASSERT_EQ(instance_list[2].get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance_list[2].get_error_code(), InstanceInfo::ErrorCode::DEPLOY_FAIL);
    ASSERT_EQ(instance_list[2].get_instance_meta()._meta_version, "meta_version_2222");
    
    system("rm -f /tmp/matrix_meta/*");
}

TEST_F(MatrixManagerTest, process_expect)
{
    MockExecutorCallback callback;
    MockExecutor executor(callback);
    MatrixManager manager(executor);
    callback.set_manager(&manager);
    
    HostInfo host_info;
    ASSERT_FALSE(manager.get_host_info(&host_info));
    
    ResourceTuple resource(1000, 1000, 1000);
    ResourceTuple revert_resource(10, 10, 10);
    manager.report_machine_resource(resource);
    ASSERT_FALSE(manager.get_host_info(&host_info));
    manager.report_machine_status(HostInfo::HostState::AVAILABLE);
    ASSERT_TRUE(manager.get_host_info(&host_info));
    manager.set_reserved_resource(revert_resource);
    manager.start();
    
    // prepare expect list.
    InstanceMeta meta;
    get_instance_meta(&meta);
    std::vector<InstanceInfo> expect_list;
    InstanceInfo info0("test.baidu.jx", 0);
    InstanceInfo info1("test.baidu.jx", 1);
    InstanceInfo info2("test.baidu.jx", 2);
    info0.set_instance_meta(meta);
    info1.set_instance_meta(meta);
    info2.set_instance_meta(meta);
    expect_list.push_back(info0);
    expect_list.push_back(info1);
    expect_list.push_back(info2);
    
    manager.push_instance_expect(expect_list);
    sleep(2);
    std::vector<InstanceInfo> acture;
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)3);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_EQ(acture[1].get_instance_name(), "1.test.baidu.jx");
    ASSERT_EQ(acture[1].get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(acture[1].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_EQ(acture[2].get_instance_name(), "2.test.baidu.jx");
    ASSERT_EQ(acture[2].get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(acture[2].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // on deploy success or fail
    callback.on_deploy(0, true);
    callback.on_deploy(1, false);
    callback.on_deploy(2, true);
    callback.on_deploy(3, false);  // not exist.
    
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)3);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_EQ(acture[1].get_instance_name(), "1.test.baidu.jx");
    ASSERT_EQ(acture[1].get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(acture[1].get_error_code(), InstanceInfo::ErrorCode::DEPLOY_FAIL);
    
    ASSERT_EQ(acture[2].get_instance_name(), "2.test.baidu.jx");
    ASSERT_EQ(acture[2].get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(acture[2].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // health check ok running or starting or error
    manager.report_instance_status("0.test.baidu.jx", 
                                   meta._meta_version,
                                   InstanceInfo::HealthCheckStatus::RUNNING);
    manager.report_instance_status("1.test.baidu.jx", 
                                   meta._meta_version,
                                   InstanceInfo::HealthCheckStatus::STARTING);
    manager.report_instance_status("2.test.baidu.jx", 
                                   meta._meta_version,
                                   InstanceInfo::HealthCheckStatus::ERROR);
    manager.report_instance_status("3.test.baidu.jx", 
                                   meta._meta_version,
                                   InstanceInfo::HealthCheckStatus::ERROR);; // not exist.
    
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)3);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_EQ(acture[1].get_instance_name(), "1.test.baidu.jx");
    ASSERT_EQ(acture[1].get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(acture[1].get_error_code(), InstanceInfo::ErrorCode::DEPLOY_FAIL);
    
    ASSERT_EQ(acture[2].get_instance_name(), "2.test.baidu.jx");
    ASSERT_EQ(acture[2].get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(acture[2].get_error_code(), InstanceInfo::ErrorCode::HEALCHECK_FAIL);
    
    // check get_monitor_instance_list(std::map*)
    std::map<uint64_t, InstanceInfo> monitor_instances;
    manager.get_monitor_instance_list(&monitor_instances);
    ASSERT_EQ(monitor_instances.size(), (size_t)2);
    ASSERT_EQ(monitor_instances[0ul].get_instance_name(), acture[0].get_instance_name());
    ASSERT_TRUE(monitor_instances.find(1ul) == monitor_instances.end());
    
    
    // clear expect_list, do remove
    expect_list.clear();
    manager.push_instance_expect(expect_list);
    sleep(1);
    
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)3);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_EQ(acture[1].get_instance_name(), "1.test.baidu.jx");
    ASSERT_EQ(acture[1].get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(acture[1].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_EQ(acture[2].get_instance_name(), "2.test.baidu.jx");
    ASSERT_EQ(acture[2].get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(acture[2].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // on remove success or fail
    callback.on_remove(0, true);
    callback.on_remove(1, true);
    callback.on_remove(2, false);
    callback.on_remove(3, false);  // not exist.
    
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    
    ASSERT_EQ(acture[0].get_instance_name(), "2.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::REMOVE_FAIL);
    
    manager.stop();
}


TEST_F(MatrixManagerTest, agent_control)
{
    MockExecutorCallback callback;
    MockExecutor executor(callback);
    MatrixManager manager(executor);
    callback.set_manager(&manager);
    
    ResourceTuple resource(1000, 1000, 1000);
    ResourceTuple revert_resource(10, 10, 10);
    manager.report_machine_resource(resource);
    manager.report_machine_status(HostInfo::HostState::AVAILABLE);
    manager.set_reserved_resource(revert_resource);
    manager.start();
    
    ASSERT_FALSE(manager.get_freeze());
    
    // prepare expect list.
    InstanceMeta meta;
    get_instance_meta(&meta);
    std::vector<InstanceInfo> expect_list;
    InstanceInfo info("test.baidu.jx", 0);
    info.set_instance_meta(meta);
    expect_list.push_back(info);
    
    manager.push_instance_expect(expect_list);
    sleep(3);
    std::vector<InstanceInfo> acture;
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // deploy fail
    callback.on_deploy(0, false);
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::DEPLOY_FAIL);
    
    // reset to running
    ASSERT_FALSE(manager.reset_instance("0.test.baidu.jx"));
    manager.set_freeze(true);
    ASSERT_TRUE(manager.reset_instance("0.test.baidu.jx"));
    ASSERT_FALSE(manager.reset_instance("1.test.baidu.jx"));
    
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // test for freeze mode.
    InstanceInfo info1("test.baidu.jx", 1);
    InstanceInfo info2("test.baidu.jx", 2);
    info1.set_instance_meta(meta);
    info2.set_instance_meta(meta);
    expect_list.push_back(info1);
    expect_list.push_back(info2);
    manager.push_instance_expect(expect_list);
    sleep(2);
    
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
}

TEST_F(MatrixManagerTest, test_resource)
{
    MockExecutorCallback callback;
    MockExecutor executor(callback);
    MatrixManager manager(executor);
    callback.set_manager(&manager);
    
    HostInfo host_info;
    ASSERT_FALSE(manager.get_host_info(&host_info));
    
    ResourceTuple resource(1000, 1000, 1000);
    ResourceTuple revert_resource(10, 10, 10);
    manager.report_machine_resource(resource);
    ASSERT_FALSE(manager.get_host_info(&host_info));
    manager.report_machine_status(HostInfo::HostState::AVAILABLE);
    ASSERT_TRUE(manager.get_host_info(&host_info));
    manager.set_reserved_resource(revert_resource);
    manager.start();
    
    // prepare expect list.
    InstanceMeta meta;
    get_instance_meta(&meta);
    std::vector<InstanceInfo> expect_list;
    InstanceInfo info0("test.baidu.jx", 0);
    InstanceInfo info1("test.baidu.jx", 1);
    info0.set_instance_meta(meta);
    expect_list.push_back(info0);
    meta._resource._cpu_num = 995;
    info1.set_instance_meta(meta);
    expect_list.push_back(info1);
    
    manager.push_instance_expect(expect_list);
    sleep(2);
    std::vector<InstanceInfo> acture;
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // on deploy success
    callback.on_deploy(0, true);
    
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // health check ok running or starting or error
    manager.report_instance_status("0.test.baidu.jx", 
                                   meta._meta_version,
                                   InstanceInfo::HealthCheckStatus::RUNNING);
    
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // clear expect_list, do remove
    expect_list.clear();
    manager.push_instance_expect(expect_list);
    sleep(1);
    
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    
    // on remove success
    callback.on_remove(0, true);
    
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)0);
    
    manager.stop();
}

TEST_F(MatrixManagerTest, UpdateWhenMonitor)
{
    MockExecutorCallback callback;
    MockExecutor executor(callback);
    MatrixManager manager(executor);
    callback.set_manager(&manager);
    
    HostInfo host_info;
    ASSERT_FALSE(manager.get_host_info(&host_info));
    ResourceTuple resource(1000, 1000, 1000);
    ResourceTuple revert_resource(10, 10, 10);
    manager.report_machine_resource(resource);
    ASSERT_FALSE(manager.get_host_info(&host_info));
    manager.report_machine_status(HostInfo::HostState::AVAILABLE);
    ASSERT_TRUE(manager.get_host_info(&host_info));
    manager.set_reserved_resource(revert_resource);
    manager.start();
    
    // prepare expect list.
    InstanceMeta meta0;
    get_instance_meta(&meta0);
    std::vector<InstanceInfo> expect_list;
    InstanceInfo info0("test.baidu.jx", 0);
    info0.set_instance_meta(meta0);
    expect_list.push_back(info0);
    manager.push_instance_expect(expect_list);

    sleep(2);
    std::vector<InstanceInfo> acture;
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // on deploy success
    callback.on_deploy(0, true);
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // health check error
    manager.report_instance_status("0.test.baidu.jx", 
                                   meta0._meta_version,
                                   InstanceInfo::HealthCheckStatus::ERROR);
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::HEALCHECK_FAIL);

    // Update
    expect_list.clear();
    InstanceMeta meta1;
    get_instance_meta(&meta1);
    meta1._meta_version = "XXXXXXX";
    InstanceInfo info1("test.baidu.jx", 0);
    info1.set_instance_meta(meta1);
    expect_list.push_back(info1);
    manager.push_instance_expect(expect_list);

    sleep(2);
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // on deploy success
    callback.on_deploy(0, true);
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);

    // monitor report old status, manager should ignore it.
    manager.report_instance_status("0.test.baidu.jx", 
                                   meta0._meta_version,
                                   InstanceInfo::HealthCheckStatus::ERROR);
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // monotor report new status
    manager.report_instance_status("0.test.baidu.jx", 
                                   meta1._meta_version,
                                   InstanceInfo::HealthCheckStatus::RUNNING);
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);

    // clear expect_list, do remove
    expect_list.clear();
    manager.push_instance_expect(expect_list);
    sleep(1);
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)1);
    ASSERT_EQ(acture[0].get_instance_name(), "0.test.baidu.jx");
    ASSERT_EQ(acture[0].get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(acture[0].get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    // on remove success
    callback.on_remove(0, true);
    manager.get_all_instance_info(&acture);
    ASSERT_EQ(acture.size(), (size_t)0);

    manager.stop();
}
