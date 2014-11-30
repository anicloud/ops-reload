#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <cstdlib>
#include <string>

#include <gtest/gtest.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <boost/make_shared.hpp>

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/rpc/MatrixThriftRpc.h"


using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace matrix;

const static int32_t MOCK_MASTER_PORT = 28528;

static struct Configure g_mutable_config;

class MasterAgentProtocolHandler : virtual public MasterAgentProtocolIf {
public:
    MasterAgentProtocolHandler(bool hang) : _hang(hang) {}

    void heartbeat(ThriftHeartbeatResponse& _return, const ThriftHeartbeatMessage& msg)
    {
        if(_hang) {
            sleep(1024);
        }
    }

    bool _hang;
};

class MockMaster
{
public:

    MockMaster() : _child(0) , _hang(false) {}
    virtual ~MockMaster() {}

    bool start()
    {
        _child = fork();

        if(_child == 0) {
            LOG.info("start mock master at port %d", MOCK_MASTER_PORT);
            boost::shared_ptr<MasterAgentProtocolHandler> _handler = boost::make_shared<MasterAgentProtocolHandler>(_hang);
            boost::shared_ptr<TProcessor> processor(new MasterAgentProtocolProcessor(_handler));
            boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(MOCK_MASTER_PORT));
            boost::shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory());
            boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

            TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
            server.serve();

        } else if(_child > 0) {
            // wait child to startup.
            sleep(2);
            return true;
        }

        LOG.error("mock master fork failed");
        return false;
    }

    void stop()
    {
        if (_child != 0) {
            kill(_child, 9);
            sleep(2);
        }
        _child = 0;
    }

    void set_hang()
    {
        _hang = true;
    }

    pid_t _child;
    bool _hang;
};

class MatrixThriftRpcTest : public testing::Test 
{
protected:
    virtual void SetUp()
    {
        g_mutable_config.MASTER_ADDRESS = "127.0.0.1"; 
        g_mutable_config.MASTER_PORT = MOCK_MASTER_PORT;
        g_mutable_config.MASTER_CONNECT_TIMEOUT = 5000;
        g_mutable_config.MASTER_READ_TIMEOUT = 5000;
        g_mutable_config.MASTER_WRITE_TIMEOUT = 5000;
        g_config = &g_mutable_config;
        //
        _master = new MockMaster();
        _rpc = new MatrixMasterThriftRpc();
    }

    virtual void TearDown()
    {
        if (_master != NULL) {
            _master->stop();
        }
        delete _rpc;
        _rpc = NULL;
        delete _master;
        _master = NULL;
    }

    MockMaster* _master;
    MatrixMasterThriftRpc* _rpc;
};


TEST_F(MatrixThriftRpcTest, Normal)
{
    EXPECT_TRUE(_master->start());
    //
    HostInfo host_info;
    ResourceTuple resource;
    resource._cpu_num = 80;
    resource._memory_mb = 10240;
    resource._disk_total_mb = 102400;
    host_info.set_total_resource(resource);
    host_info.set_host_state(HostInfo::HostState::AVAILABLE);
    HeartbeatMessage msg(host_info, std::vector<InstanceInfo>(), true);
    HeartbeatResponse response = _rpc->heartbeat(msg);
}

TEST_F(MatrixThriftRpcTest, RpcAutoReconnect)
{
    EXPECT_TRUE(_master->start());
    //
    HostInfo host_info;
    ResourceTuple resource;
    resource._cpu_num = 80;
    resource._memory_mb = 10240;
    resource._disk_total_mb = 102400;
    host_info.set_total_resource(resource);
    host_info.set_host_state(HostInfo::HostState::AVAILABLE);
    HeartbeatMessage msg(host_info, std::vector<InstanceInfo>(), true);
    _rpc->heartbeat(msg);
    // stop master
    _master->stop();
    try {
        _rpc->heartbeat(msg);
        // should not reach heare.
        EXPECT_TRUE(false);
    } catch (MatrixRpcException& ex) {
    }
    // restart master
    EXPECT_TRUE(_master->start());
    _rpc->heartbeat(msg);
}

TEST_F(MatrixThriftRpcTest, Timeout)
{
    _master->set_hang();
    EXPECT_TRUE(_master->start());
    //
    HostInfo host_info;
    ResourceTuple resource;
    resource._cpu_num = 80;
    resource._memory_mb = 10240;
    resource._disk_total_mb = 102400;
    host_info.set_total_resource(resource);
    host_info.set_host_state(HostInfo::HostState::AVAILABLE);
    HeartbeatMessage msg(host_info, std::vector<InstanceInfo>(), true);
    try {
        _rpc->heartbeat(msg);
        // should not reach heare.
        EXPECT_TRUE(false);
    } catch (MatrixRpcException& ex) {
    }
}

TEST_F(MatrixThriftRpcTest, Convert)
{
    HostInfo host_info;
    ResourceTuple resource;
    resource._cpu_num = 80;
    resource._memory_mb = 10240;
    resource._disk_total_mb = 102400;
    host_info.set_total_resource(resource);
    host_info.set_host_state(HostInfo::HostState::AVAILABLE);
    //
    InstanceMeta meta;
    meta._deploy_dir = "/home/mapred/ark";
    meta._deploy_timeout_sec = 600;
    meta._health_check_timeout_sec = 5;
    meta._group = "ark";
    meta._min_port_include = 40000;
    meta._max_port_include = 50000;
    meta._meta_version = "1.1.21";
    meta._package_source = "ftp://ark.baidu.com/package/ark_1-1-21.tgz";
    meta._package_version = "1.1.21";
    meta._package_type = InstanceMeta::PackageType::IMCI;
    meta._port.insert(std::make_pair("nc", 45678));
    meta._process_name.insert(std::make_pair("main", "nc"));
    meta._tag.insert(std::make_pair("resource_node", "abaci.appmaster"));
    meta._resource._cpu_num = 10;
    meta._resource._memory_mb = 512;
    meta._resource._disk_total_mb = 1024;
    //
    InstanceInfo instance("www.baidu.com", 0);
    instance.set_error_code(InstanceInfo::ErrorCode::SUCCESS);
    instance.set_instance_state(InstanceInfo::InstanceState::RUNNING);
    instance.set_instance_meta(meta);
    //
    std::vector<InstanceInfo> instances;
    instances.push_back(instance);
    HeartbeatMessage msg(host_info, instances, true);
    //
    ThriftHeartbeatMessage t_msg = MatrixMasterThriftRpc::to_thrift(msg, "127.0.0.1", "localhost");
    EXPECT_TRUE(t_msg.firstHeartbeat);
    EXPECT_EQ(t_msg.hostInfo.hostIp, "127.0.0.1");
    EXPECT_EQ(t_msg.hostInfo.hostName, "localhost");
    EXPECT_EQ(t_msg.hostInfo.totalResource.cpuNum, 80);
    EXPECT_EQ(t_msg.hostInfo.totalResource.memoryMB, 10240);
    EXPECT_EQ(t_msg.hostInfo.totalResource.diskTotalMB, 102400);
    EXPECT_EQ(t_msg.hostInfo.hostState, ThriftHostState::AVAILABLE);
    EXPECT_EQ(t_msg.instanceInfo.size(), (size_t)1);
    InstanceInfo converted_instance = MatrixMasterThriftRpc::to_matrix(t_msg.instanceInfo[0]);
    EXPECT_EQ(instance.to_json(), converted_instance.to_json());
    ThriftHeartbeatResponse t_res;
    t_res.agentConf.heartbeatIntervalSec = 100;
    t_res.agentConf.safeMode = true;
    t_res.agentConf.agentHttpPort = 8080;
    HeartbeatResponse res = MatrixMasterThriftRpc::to_matrix(t_res);
    EXPECT_EQ(res._agent_conf._heartbeat_interval_sec, 100);
    EXPECT_EQ(res._agent_conf._master_safe_mode, true);
    EXPECT_EQ(res._agent_conf._agent_http_port, 8080);
}


