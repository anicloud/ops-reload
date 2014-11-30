#include <iostream>

#include <gtest/gtest.h>

#include "inf/computing/matrix/matrix-agent/reporter/Heartbeat.h"

using namespace matrix;

const char* normal_jsons[] = {
"{\n"
"    \"agent_conf\" : {\n"
"        \"agent_http_port\" : 80,\n"
"        \"heartbeat_interval_sec\" : 10,\n"
"        \"master_safe_mode\" : 0,\n"
"        \"reserved_resource\" : {\n"
"            \"cpu_num\" : 10,\n"
"            \"disk_total_mb\" : 1024,\n"
"            \"memory_mb\" : 1024\n"
"        }\n"
"    },\n"
"    \"expect_instances\" : [\n"
"        {\n"
"            \"error_code\" : 0,\n"
"            \"instance_meta\" : {\n"
"                \"deploy_dir\" : \"/home/work\",\n"
"                \"deploy_timeout_sec\" : 300,\n"
"                \"group\" : \"cos\",\n"
"                \"health_check_timeout_sec\" : 10,\n"
"                \"max_port_include\" : 10250,\n"
"                \"meta_version\" : \"1.1.1\",\n"
"                \"min_port_include\" : 10230,\n"
"                \"package_source\" : \"ftp://tc-dpf-dev.tc.baidu.com/home/lidejia/matrix/bw.tgz\",\n"
"                \"package_type\" : 0,\n"
"                \"package_version\" : \"1.1.1\",\n"
"                \"port\" : {\n"
"                    \"nc_main\" : 10240\n"
"                },\n"
"                \"process_name\" : {\n"
"                    \"main\" : \"nc\"\n"
"                },\n"
"                \"resource\" : {\n"
"                    \"cpu_num\" : 10,\n"
"                    \"disk_total_mb\" : 1024,\n"
"                    \"memory_mb\" : 1024\n"
"                },\n"
"                \"tag\" : {\n"
"                    \"name\" : \"lidejia\"\n"
"                }\n"
"            },\n"
"            \"instance_state\" : 30,\n"
"            \"offset\" : 20,\n"
"            \"service_name\" : \"as.nova.baidu\"\n"
"        }\n"
"    ]\n"
"}\n",
"{\n"
"    \"agent_conf\" : {\n"
"        \"agent_http_port\" : 80,\n"
"        \"heartbeat_interval_sec\" : 10,\n"
"        \"master_safe_mode\" : 0,\n"
"        \"reserved_resource\" : {\n"
"            \"cpu_num\" : 10,\n"
"            \"disk_total_mb\" : 1024,\n"
"            \"memory_mb\" : 1024\n"
"        }\n"
"    },\n"
"    \"expect_instances\" : [\n"
"    ]\n"
"}\n"
};

const char* invalid_jsons[] = {
"{\n"
"    \"agent_conf\" : {\n"
//"        \"agent_http_port\" : 80,\n"
"        \"heartbeat_interval_sec\" : 10,\n"
"        \"master_safe_mode\" : 0,\n"
"        \"reserved_resource\" : {\n"
"            \"cpu_num\" : 10,\n"
"            \"disk_total_mb\" : 1024,\n"
"            \"memory_mb\" : 1024\n"
"        }\n"
"    },\n"
"    \"expect_instances\" : [\n"
"        {\n"
"            \"error_code\" : 0,\n"
"            \"instance_meta\" : {\n"
"                \"deploy_dir\" : \"/home/work\",\n"
"                \"deploy_timeout_sec\" : 300,\n"
"                \"group\" : \"cos\",\n"
"                \"health_check_timeout_sec\" : 10,\n"
"                \"max_port_include\" : 10250,\n"
"                \"meta_version\" : \"1.1.1\",\n"
"                \"min_port_include\" : 10230,\n"
"                \"package_source\" : \"ftp://tc-dpf-dev.tc.baidu.com/home/lidejia/matrix/bw.tgz\",\n"
"                \"package_type\" : 0,\n"
"                \"package_version\" : \"1.1.1\",\n"
"                \"port\" : {\n"
"                    \"nc_main\" : 10240\n"
"                },\n"
"                \"process_name\" : {\n"
"                    \"main\" : \"nc\"\n"
"                },\n"
"                \"resource\" : {\n"
"                    \"cpu_num\" : 10,\n"
"                    \"disk_total_mb\" : 1024,\n"
"                    \"memory_mb\" : 1024\n"
"                },\n"
"                \"tag\" : {\n"
"                    \"name\" : \"lidejia\"\n"
"                }\n"
"            },\n"
"            \"instance_state\" : 30,\n"
"            \"offset\" : 20,\n"
"            \"service_name\" : \"as.nova.baidu\"\n"
"        }\n"
"    ]\n"
"}\n",
"{\n"
"    \"agent_conf\" : {\n"
"        \"agent_http_port\" : 80,\n"
//"        \"heartbeat_interval_sec\" : 10,\n"
"        \"master_safe_mode\" : 0,\n"
"        \"reserved_resource\" : {\n"
"            \"cpu_num\" : 10,\n"
"            \"disk_total_mb\" : 1024,\n"
"            \"memory_mb\" : 1024\n"
"        }\n"
"    },\n"
"    \"expect_instances\" : [\n"
"        {\n"
"            \"error_code\" : 0,\n"
"            \"instance_meta\" : {\n"
"                \"deploy_dir\" : \"/home/work\",\n"
"                \"deploy_timeout_sec\" : 300,\n"
"                \"group\" : \"cos\",\n"
"                \"health_check_timeout_sec\" : 10,\n"
"                \"max_port_include\" : 10250,\n"
"                \"meta_version\" : \"1.1.1\",\n"
"                \"min_port_include\" : 10230,\n"
"                \"package_source\" : \"ftp://tc-dpf-dev.tc.baidu.com/home/lidejia/matrix/bw.tgz\",\n"
"                \"package_type\" : 0,\n"
"                \"package_version\" : \"1.1.1\",\n"
"                \"port\" : {\n"
"                    \"nc_main\" : 10240\n"
"                },\n"
"                \"process_name\" : {\n"
"                    \"main\" : \"nc\"\n"
"                },\n"
"                \"resource\" : {\n"
"                    \"cpu_num\" : 10,\n"
"                    \"disk_total_mb\" : 1024,\n"
"                    \"memory_mb\" : 1024\n"
"                },\n"
"                \"tag\" : {\n"
"                    \"name\" : \"lidejia\"\n"
"                }\n"
"            },\n"
"            \"instance_state\" : 30,\n"
"            \"offset\" : 20,\n"
"            \"service_name\" : \"as.nova.baidu\"\n"
"        }\n"
"    ]\n"
"}\n",
"{\n"
"    \"agent_conf\" : {\n"
"        \"agent_http_port\" : 80,\n"
"        \"heartbeat_interval_sec\" : 10,\n"
//"        \"master_safe_mode\" : 0,\n"
"        \"reserved_resource\" : {\n"
"            \"cpu_num\" : 10,\n"
"            \"disk_total_mb\" : 1024,\n"
"            \"memory_mb\" : 1024\n"
"        }\n"
"    },\n"
"    \"expect_instances\" : [\n"
"        {\n"
"            \"error_code\" : 0,\n"
"            \"instance_meta\" : {\n"
"                \"deploy_dir\" : \"/home/work\",\n"
"                \"deploy_timeout_sec\" : 300,\n"
"                \"group\" : \"cos\",\n"
"                \"health_check_timeout_sec\" : 10,\n"
"                \"max_port_include\" : 10250,\n"
"                \"meta_version\" : \"1.1.1\",\n"
"                \"min_port_include\" : 10230,\n"
"                \"package_source\" : \"ftp://tc-dpf-dev.tc.baidu.com/home/lidejia/matrix/bw.tgz\",\n"
"                \"package_type\" : 0,\n"
"                \"package_version\" : \"1.1.1\",\n"
"                \"port\" : {\n"
"                    \"nc_main\" : 10240\n"
"                },\n"
"                \"process_name\" : {\n"
"                    \"main\" : \"nc\"\n"
"                },\n"
"                \"resource\" : {\n"
"                    \"cpu_num\" : 10,\n"
"                    \"disk_total_mb\" : 1024,\n"
"                    \"memory_mb\" : 1024\n"
"                },\n"
"                \"tag\" : {\n"
"                    \"name\" : \"lidejia\"\n"
"                }\n"
"            },\n"
"            \"instance_state\" : 30,\n"
"            \"offset\" : 20,\n"
"            \"service_name\" : \"as.nova.baidu\"\n"
"        }\n"
"    ]\n"
"}\n",
"{\n"
"    \"agent_conf\" : {\n"
"        \"agent_http_port\" : 80,\n"
"        \"heartbeat_interval_sec\" : 10,\n"
"        \"master_safe_mode\" : 0,\n"
"        \"reserved_resource\" : {\n"
//"            \"cpu_num\" : 10,\n"
"            \"disk_total_mb\" : 1024,\n"
"            \"memory_mb\" : 1024\n"
"        }\n"
"    },\n"
"    \"expect_instances\" : [\n"
"        {\n"
"            \"error_code\" : 0,\n"
"            \"instance_meta\" : {\n"
"                \"deploy_dir\" : \"/home/work\",\n"
"                \"deploy_timeout_sec\" : 300,\n"
"                \"group\" : \"cos\",\n"
"                \"health_check_timeout_sec\" : 10,\n"
"                \"max_port_include\" : 10250,\n"
"                \"meta_version\" : \"1.1.1\",\n"
"                \"min_port_include\" : 10230,\n"
"                \"package_source\" : \"ftp://tc-dpf-dev.tc.baidu.com/home/lidejia/matrix/bw.tgz\",\n"
"                \"package_type\" : 0,\n"
"                \"package_version\" : \"1.1.1\",\n"
"                \"port\" : {\n"
"                    \"nc_main\" : 10240\n"
"                },\n"
"                \"process_name\" : {\n"
"                    \"main\" : \"nc\"\n"
"                },\n"
"                \"resource\" : {\n"
"                    \"cpu_num\" : 10,\n"
"                    \"disk_total_mb\" : 1024,\n"
"                    \"memory_mb\" : 1024\n"
"                },\n"
"                \"tag\" : {\n"
"                    \"name\" : \"lidejia\"\n"
"                }\n"
"            },\n"
"            \"instance_state\" : 30,\n"
"            \"offset\" : 20,\n"
"            \"service_name\" : \"as.nova.baidu\"\n"
"        }\n"
"    ]\n"
"}\n",
"{\n"
"    \"agent_conf\" : {\n"
"        \"agent_http_port\" : 80,\n"
"        \"heartbeat_interval_sec\" : 10,\n"
"        \"master_safe_mode\" : 0,\n"
"        \"reserved_resource\" : {\n"
"            \"cpu_num\" : 10,\n"
"            \"disk_total_mb\" : 1024,\n"
"            \"memory_mb\" : 1024\n"
"        }\n"
"    },\n"
"    \"expect_instances\" : [\n"
"        {\n"
"            \"error_code\" : 0,\n"
"            \"instance_meta\" : {\n"
//"                \"deploy_dir\" : \"/home/work\",\n"
"                \"deploy_timeout_sec\" : 300,\n"
"                \"group\" : \"cos\",\n"
"                \"health_check_timeout_sec\" : 10,\n"
"                \"max_port_include\" : 10250,\n"
"                \"meta_version\" : \"1.1.1\",\n"
"                \"min_port_include\" : 10230,\n"
"                \"package_source\" : \"ftp://tc-dpf-dev.tc.baidu.com/home/lidejia/matrix/bw.tgz\",\n"
"                \"package_type\" : 0,\n"
"                \"package_version\" : \"1.1.1\",\n"
"                \"port\" : {\n"
"                    \"nc_main\" : 10240\n"
"                },\n"
"                \"process_name\" : {\n"
"                    \"main\" : \"nc\"\n"
"                },\n"
"                \"resource\" : {\n"
"                    \"cpu_num\" : 10,\n"
"                    \"disk_total_mb\" : 1024,\n"
"                    \"memory_mb\" : 1024\n"
"                },\n"
"                \"tag\" : {\n"
"                    \"name\" : \"lidejia\"\n"
"                }\n"
"            },\n"
"            \"instance_state\" : 30,\n"
"            \"offset\" : 20,\n"
"            \"service_name\" : \"as.nova.baidu\"\n"
"        }\n"
"    ]\n"
"}\n",
};


TEST(HeartbeatTest, FromJsonNormal)
{
    for(int i = 0; i < sizeof(normal_jsons) / sizeof(const char*); i++) {
        Json::Reader reader;
        Json::Value json;
        EXPECT_TRUE(reader.parse(normal_jsons[i], json));
        HeartbeatResponse response = HeartbeatResponse::from_json(json);
        EXPECT_TRUE(response.is_valid());
    }
}

TEST(HeartbeatTest, FromJsonInvalid)
{
    for(int i = 0; i < sizeof(invalid_jsons) / sizeof(const char*); i++) {
        Json::Reader reader;
        Json::Value json;
        EXPECT_TRUE(reader.parse(invalid_jsons[i], json));
        try {
            HeartbeatResponse response = HeartbeatResponse::from_json(json);
            EXPECT_TRUE(false);
        } catch (MatrixJsonException& ex) {

        }
    }
}