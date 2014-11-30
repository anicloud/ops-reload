#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

#include <gtest/gtest.h>

#include "op/oped/noah/sailor/utility/Logger.h"

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/manager/Instance.h"

using namespace matrix;

static struct Configure g_mutable_config;

class InstanceTest : public testing::Test
{
protected:
    virtual void SetUp()
    {
        g_mutable_config.META_PATH = "/tmp/matrix_meta";
        g_mutable_config.DEFAULT_MANAGER_REMOVE_TIMEO = 3;
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

// test install ok -> health check starting -> health check ok -> remove ok
TEST_F(InstanceTest, state_machine_1)
{
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_EQ(0lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_install(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::STARTING);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_TRUE(instance.do_remove());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_remove(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::INVALID);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
}


// test install fail -> remove ok
TEST_F(InstanceTest, state_machine_2)
{
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_EQ(0lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_install(InstanceInfo::ErrorCode::DEPLOY_FAIL);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::DEPLOY_FAIL);
    
    ASSERT_TRUE(instance.do_remove());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_remove(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::INVALID);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
}

// test install fail -> health check ok -> remove ok
TEST_F(InstanceTest, state_machine_3)
{
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_EQ(0lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_install(InstanceInfo::ErrorCode::DEPLOY_FAIL);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::DEPLOY_FAIL);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::DEPLOY_FAIL);
    
    ASSERT_TRUE(instance.do_remove());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_remove(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::INVALID);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
}

// test install ok -> health check fail ->remove ok
TEST_F(InstanceTest, state_machine_4)
{
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_EQ(0lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_install(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::HEALCHECK_FAIL);
    
    ASSERT_TRUE(instance.do_remove());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_remove(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::INVALID);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
}


// test install ok -> health check timeo ->remove ok
TEST_F(InstanceTest, state_machine_5)
{
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_EQ(0lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_install(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::TIMEOUT);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::HEALCHECK_TIMEOUT);
    
    ASSERT_TRUE(instance.do_remove());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_remove(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::INVALID);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
}

// test install ok -> health check fail -> health check ok -> remove ok
TEST_F(InstanceTest, state_machine_6)
{
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_EQ(0lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_install(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::HEALCHECK_FAIL);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_TRUE(instance.do_remove());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_remove(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::INVALID);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
}

// test install ok -> health check timeout -> health check ok -> remove ok
TEST_F(InstanceTest, state_machine_7)
{
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_EQ(0lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_install(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::TIMEOUT);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::HEALCHECK_TIMEOUT);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_TRUE(instance.do_remove());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_remove(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::INVALID);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
}

// test install timeo
TEST_F(InstanceTest, state_machine_8)
{
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_EQ(0lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    meta._deploy_timeout_sec = 3;
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    sleep(1);
    ASSERT_FALSE(instance.check_timeo());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    sleep(2);
    ASSERT_TRUE(instance.check_timeo());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::DEPLOY_TIMEOUT);
}

// test remove timeo
TEST_F(InstanceTest, state_machine_9)
{
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_EQ(0lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_FALSE(instance.do_remove());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_install(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_TRUE(instance.do_remove());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    sleep(1);
    ASSERT_FALSE(instance.check_timeo());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    sleep(2);
    ASSERT_TRUE(instance.check_timeo());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::ERROR);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::REMOVE_TIMEOUT);
}

// test load from file
TEST_F(InstanceTest, state_machine_10)
{
    Instance instance(1, "test2.baidu.jx", 0);
    ASSERT_EQ(1lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    meta._deploy_dir = "/asdfasdf";
    meta._deploy_timeout_sec = 12345;
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    Instance instance2("1.0.test2.baidu.jx");
    ASSERT_TRUE(instance.get_generation() == instance2.get_generation());
    ASSERT_TRUE(instance.get_instance_info().get_instance_state() == instance2.get_instance_info().get_instance_state());
    ASSERT_TRUE(instance.get_instance_info().get_error_code() == instance2.get_instance_info().get_error_code());
    ASSERT_TRUE(instance.get_instance_info().get_instance_name() == instance2.get_instance_info().get_instance_name());
    ASSERT_TRUE(meta._deploy_dir == instance2.get_instance_info().get_instance_meta()._deploy_dir);
    ASSERT_TRUE(meta._deploy_timeout_sec == instance2.get_instance_info().get_instance_meta()._deploy_timeout_sec);
}

// test install ok -> health check ok -> update ok -> health check ok -> remove ok
TEST_F(InstanceTest, state_machine_11)
{
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_EQ(0lu, instance.get_generation());
    
    InstanceMeta meta;
    get_instance_meta(&meta);
    
    ASSERT_TRUE(instance.do_install(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_install(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    meta._meta_version = "version_2";
    meta._deploy_dir = "/cccccccc";
    ASSERT_TRUE(instance.do_update(meta));
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::DEPLOYING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_install(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::STARTING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_health_check(InstanceInfo::HealthCheckStatus::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::RUNNING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    ASSERT_TRUE(instance.do_remove());
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::REMOVING);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
    
    instance.on_remove(InstanceInfo::ErrorCode::SUCCESS);
    ASSERT_EQ(instance.get_instance_info().get_instance_state(), InstanceInfo::InstanceState::INVALID);
    ASSERT_EQ(instance.get_instance_info().get_error_code(), InstanceInfo::ErrorCode::SUCCESS);
}


std::string meta_case =
"{"
"   \"event_start_timestamp\" : 1366287423,"
"   \"event_timeout_sec\" : 10,"
"   \"generation\" : 3,"
"   \"instance_info\" : {"
"      \"error_code\" : 0,"
"      \"instance_meta\" : {"
"         \"deploy_dir\" : \"/home/work\","
"         \"deploy_timeout_sec\" : 10,"
"         \"group\" : \"group\","
"         \"health_check_timeout_sec\" : 10,"
"         \"meta_version\" : \"meta_version\","
"         \"max_port_include\" : 1,"
"         \"min_port_include\" : 1,"
"         \"package_source\" : \"http://package.source\","
"         \"package_version\" : \"\","
"         \"package_type\" : 0, "
"         \"port\" : null,"
"         \"process_name\" : null,"
"         \"resource\" : {"
"            \"cpu_num\" : 10,"
"            \"disk_total_mb\" : 100,"
"            \"memory_mb\" : 100"
"         },"
"         \"tag\" : null"
"      },"
"      \"instance_state\" : 10,"
"      \"offset\" : 0,"
"      \"service_name\" : \"test.baidu.jx\""
"   }"
"}";

std::string meta_case_invalid_string =
"{"
"   \"event_start_timestamp\" : -1,"
"   \"event_timeout_sec\" : 10,"
"   \"generation\" : 3,"
"   \"instance_info\" : {"
"      \"error_code\" : 0,"
"      \"instance_meta\" : {"
"         \"deploy_dir\" : \"/home/work\","
"         \"deploy_timeout_sec\" : 10,"
"         \"group\" : \"group\","
"         \"health_check_timeout_sec\" : 10,"
"         \"meta_version\" : \"meta$version\","
"         \"max_port_include\" : 1,"
"         \"min_port_include\" : 1,"
"         \"package_source\" : \"http://package.source\","
"         \"package_version\" : \"\","
"         \"package_type\" : 0, "
"         \"port\" : null,"
"         \"process_name\" : null,"
"         \"resource\" : {"
"            \"cpu_num\" : 10,"
"            \"disk_total_mb\" : 100,"
"            \"memory_mb\" : 100"
"         },"
"         \"tag\" : null"
"      },"
"      \"instance_state\" : 10,"
"      \"offset\" : 0,"
"      \"service_name\" : \"test$$baidu.jx\""
"   }"
"}";

std::string meta_case_invalid_path =
"{"
"   \"event_start_timestamp\" : -1,"
"   \"event_timeout_sec\" : 10,"
"   \"generation\" : 3,"
"   \"instance_info\" : {"
"      \"error_code\" : 0,"
"      \"instance_meta\" : {"
"         \"deploy_dir\" : \"home/work\","
"         \"deploy_timeout_sec\" : 10,"
"         \"group\" : \"group\","
"         \"health_check_timeout_sec\" : 10,"
"         \"meta_version\" : \"meta-version\","
"         \"max_port_include\" : 1,"
"         \"min_port_include\" : 1,"
"         \"package_source\" : \"http://package.source\","
"         \"package_version\" : \"\","
"         \"package_type\" : 0, "
"         \"port\" : null,"
"         \"process_name\" : null,"
"         \"resource\" : {"
"            \"cpu_num\" : 10,"
"            \"disk_total_mb\" : 100,"
"            \"memory_mb\" : 100"
"         },"
"         \"tag\" : null"
"      },"
"      \"instance_state\" : 10,"
"      \"offset\" : 0,"
"      \"service_name\" : \"test-baidu.jx\""
"   }"
"}";

std::string meta_case_invalid_num =
"{"
"   \"event_start_timestamp\" : 1,"
"   \"event_timeout_sec\" : 10,"
"   \"generation\" : 3,"
"   \"instance_info\" : {"
"      \"error_code\" : 0,"
"      \"instance_meta\" : {"
"         \"deploy_dir\" : \"home/work\","
"         \"deploy_timeout_sec\" : \"asd\","
"         \"group\" : \"group\","
"         \"health_check_timeout_sec\" : 10,"
"         \"meta_version\" : \"meta-version\","
"         \"max_port_include\" : 1,"
"         \"min_port_include\" : 1,"
"         \"package_source\" : \"http://package.source\","
"         \"package_version\" : \"\","
"         \"package_type\" : 0, "
"         \"port\" : null,"
"         \"process_name\" : null,"
"         \"resource\" : {"
"            \"cpu_num\" : 10,"
"            \"disk_total_mb\" : 100,"
"            \"memory_mb\" : 100"
"         },"
"         \"tag\" : null"
"      },"
"      \"instance_state\" : 10,"
"      \"offset\" : 0,"
"      \"service_name\" : \"test-baidu.jx\""
"   }"
"}";

std::string meta_case_invalid_errorcode =
"{"
"   \"event_start_timestamp\" : 1,"
"   \"event_timeout_sec\" : 10,"
"   \"generation\" : 3,"
"   \"instance_info\" : {"
"      \"error_code\" : 110,"
"      \"instance_meta\" : {"
"         \"deploy_dir\" : \"home/work\","
"         \"deploy_timeout_sec\" : 10,"
"         \"group\" : \"group\","
"         \"health_check_timeout_sec\" : 10,"
"         \"meta_version\" : \"meta-version\","
"         \"max_port_include\" : 1,"
"         \"min_port_include\" : 1,"
"         \"package_source\" : \"http://package.source\","
"         \"package_version\" : \"\","
"         \"package_type\" : 0, "
"         \"port\" : null,"
"         \"process_name\" : null,"
"         \"resource\" : {"
"            \"cpu_num\" : 10,"
"            \"disk_total_mb\" : 100,"
"            \"memory_mb\" : 100"
"         },"
"         \"tag\" : null"
"      },"
"      \"instance_state\" : 10,"
"      \"offset\" : 0,"
"      \"service_name\" : \"test-baidu.jx\""
"   }"
"}";

std::string meta_case_invalid_json =
"{"
"   \"event_start_timestamp\" : 1,"
"   \"event_timeout_sec\" : 10,"
"   \"generation\" : 3,"
"   \"instance_info\" : {"
"      \"error_code\" : 0"
"      \"instance_meta\" : {"
"         \"deploy_dir\" : \"/home/work\","
"         \"deploy_timeout_sec\" : 10,"
"         \"group\" : \"group\","
"         \"health_check_timeout_sec\" : 10,"
"         \"meta_version\" : \"meta-version\","
"         \"max_port_include\" : 1,"
"         \"min_port_include\" : 1,"
"         \"package_source\" : \"http://package.source\","
"         \"package_version\" : \"\","
"         \"package_type\" : 0, "
"         \"port\" : null,"
"         \"process_name\" : null,"
"         \"resource\" : {"
"            \"cpu_num\" : 10,"
"            \"disk_total_mb\" : 100,"
"            \"memory_mb\" : 100"
"         },"
"         \"tag\" : null"
"      },"
"      \"instance_state\" : 10,"
"      \"offset\" : 0,"
"      \"service_name\" : \"test-baidu.jx\""
"   }"
"}";

TEST_F(InstanceTest, from_string)
{
    std::string content = "";
    Instance instance(0, "test.baidu.jx", 0);
    ASSERT_TRUE(instance.from_string(meta_case));
    ASSERT_TRUE(instance.get_instance_info().is_valid());
    
    ASSERT_TRUE(instance.from_string(meta_case_invalid_string));
    ASSERT_FALSE(instance.get_instance_info().is_valid());
    
    ASSERT_TRUE(instance.from_string(meta_case_invalid_path));
    ASSERT_FALSE(instance.get_instance_info().is_valid());
    
    ASSERT_TRUE(instance.from_string(meta_case_invalid_errorcode));
    ASSERT_FALSE(instance.get_instance_info().is_valid());
    
    ASSERT_FALSE(instance.from_string(meta_case_invalid_num));
    //ASSERT_FALSE(instance.get_instance_info().is_valid());
    
    ASSERT_FALSE(instance.from_string(meta_case_invalid_json));
}
