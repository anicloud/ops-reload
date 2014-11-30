#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <limits.h>
#include <errno.h>

#include "inf/computing/matrix/matrix-agent/rpc/MatrixThriftRpc.h"
#include "op/oped/noah/sailor/utility/thread/MutexLocker.h"

namespace matrix {

/**
 * Convert between matrix and thrift
 */

void MatrixMasterThriftRpc::to_thrift(const ResourceTuple& resource,
                                      ThriftResourceTuple& t_resource)
{
    t_resource.cpuNum = resource._cpu_num;
    t_resource.memoryMB = resource._memory_mb;
    t_resource.diskTotalMB = resource._disk_total_mb;
}

void MatrixMasterThriftRpc::to_thrift(const HostInfo& info,
                                      const std::string& host_ip,
                                      const std::string& host_name,
                                      ThriftHostInfo& t_info)
{
    t_info.hostIp = host_ip;
    t_info.hostName = host_name;
    t_info.hostState = static_cast<ThriftHostState::type>(info.get_host_state());
    to_thrift(info.get_total_resource(), t_info.totalResource);
}

void MatrixMasterThriftRpc::to_thrift(const InstanceMeta& meta,
                                      ThriftAgentInstanceMeta& t_meta)
{
    t_meta.metaVersion           = meta._meta_version;
    t_meta.packageSource         = meta._package_source;
    t_meta.packageVersion        = meta._package_version;
    t_meta.packageType           = static_cast<ThriftPackageType::type>(meta._package_type);
    t_meta.deployDir             = meta._deploy_dir;
    t_meta.processName           = meta._process_name;
    t_meta.tag                   = meta._tag;
    t_meta.deployTimeoutSec      = meta._deploy_timeout_sec;
    t_meta.healthCheckTimeoutSec = meta._health_check_timeout_sec;
    t_meta.port.port             = meta._port;
    t_meta.port.minPortInclude   = meta._min_port_include;
    t_meta.port.maxPortInclude   = meta._max_port_include;
    t_meta.group                 = meta._group;
    to_thrift(meta._resource, t_meta.resource);
}


void MatrixMasterThriftRpc::to_thrift(const InstanceInfo& instance,
                                      ThriftInstanceInfo& t_instance)
{
    t_instance.serviceName = instance.get_service_name();
    t_instance.offset = instance.get_offset();
    t_instance.errorCode  = static_cast<ThriftInstanceErrorCode::type>(instance.get_error_code());
    t_instance.instanceState = static_cast<ThriftInstanceState::type>(instance.get_instance_state());
    to_thrift(instance.get_instance_meta(), t_instance.instanceMeta);
}

ThriftHeartbeatMessage MatrixMasterThriftRpc::to_thrift(const HeartbeatMessage& msg,
                                                        const std::string& host_ip,
                                                        const std::string& host_name)
{
    ThriftHeartbeatMessage t_msg;

    to_thrift(msg._host_info, host_ip, host_name, t_msg.hostInfo);

    for (std::vector<InstanceInfo>::const_iterator itr = msg._instances.begin();
         itr != msg._instances.end(); ++itr) {
        ThriftInstanceInfo t_info;
        to_thrift(*itr, t_info);
        t_msg.instanceInfo.push_back(t_info);
    }

    t_msg.firstHeartbeat = msg._first_heartbeat;
    return t_msg;
}

ResourceTuple MatrixMasterThriftRpc::to_matrix(const ThriftResourceTuple& t_resource)
{
    ResourceTuple resource;
    resource._cpu_num = t_resource.cpuNum;
    resource._memory_mb = t_resource.memoryMB;
    resource._disk_total_mb = t_resource.diskTotalMB;
    return resource;
}

AgentConf MatrixMasterThriftRpc::to_matrix(const ThriftAgentConf& t_conf)
{
    return AgentConf(t_conf.safeMode,
                     t_conf.heartbeatIntervalSec,
                     t_conf.agentHttpPort,
                     to_matrix(t_conf.reservedResource));
}

InstanceMeta MatrixMasterThriftRpc::to_matrix(const ThriftAgentInstanceMeta& t_meta)
{
    InstanceMeta meta;
    meta._meta_version           = t_meta.metaVersion;
    meta._package_source        = t_meta.packageSource;
    meta._package_version       = t_meta.packageVersion;
    meta._package_type          = static_cast<InstanceMeta::PackageType::type>(t_meta.packageType);
    meta._deploy_dir            = t_meta.deployDir;
    meta._process_name          = t_meta.processName;
    meta._tag                   = t_meta.tag;
    meta._deploy_timeout_sec    = t_meta.deployTimeoutSec;
    meta._health_check_timeout_sec = t_meta.healthCheckTimeoutSec;
    meta._port                  = t_meta.port.port;
    meta._group                 = t_meta.group;
    meta._min_port_include      = t_meta.port.minPortInclude;
    meta._max_port_include      = t_meta.port.maxPortInclude;
    meta._resource = to_matrix(t_meta.resource);
    return meta;
}

InstanceInfo MatrixMasterThriftRpc::to_matrix(const ThriftInstanceInfo& t_instance)
{
    InstanceInfo instance(t_instance.serviceName, t_instance.offset);
    instance.set_error_code(static_cast<InstanceInfo::ErrorCode::type>(t_instance.errorCode));
    instance.set_instance_state(static_cast<InstanceInfo::InstanceState::type>(t_instance.instanceState));
    instance.set_instance_meta(to_matrix(t_instance.instanceMeta));
    return instance;
}

HeartbeatResponse MatrixMasterThriftRpc::to_matrix(const ThriftHeartbeatResponse& t_res)
{
    AgentConf conf = to_matrix(t_res.agentConf);
    std::vector<InstanceInfo> expect_instances;
    for (std::vector<ThriftInstanceInfo>::const_iterator itr = t_res.expectInstance.begin();
         itr != t_res.expectInstance.end(); ++itr) {
        expect_instances.push_back(to_matrix(*itr));
    }
    return HeartbeatResponse(conf, expect_instances);

} // END OF CONVERT

MatrixMasterThriftRpc::MatrixMasterThriftRpc(): 
    _client(NULL), 
    _master_host_name(g_config->MASTER_ADDRESS), 
    _master_port(g_config->MASTER_PORT),
    _conn_timeo_ms(g_config->MASTER_CONNECT_TIMEOUT),
    _read_timeo_ms(g_config->MASTER_READ_TIMEOUT),
    _write_timeo_ms(g_config->MASTER_WRITE_TIMEOUT),
    _connected(false)
{
}

MatrixMasterThriftRpc::~MatrixMasterThriftRpc()
{
    ::sailor::MutexLocker locker(&_mutex);
    close_connection();
}

HeartbeatResponse MatrixMasterThriftRpc::heartbeat(const HeartbeatMessage& msg) throw (MatrixRpcException)
{

    ::sailor::MutexLocker locker(&_mutex);

    // check connection.
    if (!_connected) {
        // close old connection.
        close_connection();

        // init new connection.
        init_connection();
    }

    // connection is not ready, try next time.
    if (!_connected) {
        throw MatrixRpcException("connection is not ready.");
    }

    try {

        ThriftHeartbeatMessage t_msg = to_thrift(msg, _host_ip, _host_name);
        ThriftHeartbeatResponse t_res;

        LOG.trace("MatrixMasterThriftRpc: heartbeat called");
        _client->heartbeat(t_res, t_msg);
        LOG.trace("MatrixMasterThriftRpc: heartbeat finished.");
        return to_matrix(t_res);

    } catch (std::exception& ex) {

        LOG.warn("MatrixMasterThriftRpc: heartbeat error %s", ex.what());
        _connected = false;
        throw MatrixRpcException(ex.what());

    }
}

bool MatrixMasterThriftRpc::get_host_info(int32_t socket_fd)
{
    struct sockaddr_in sockaddr;
    socklen_t sockaddr_len = sizeof(sockaddr);
    if (getsockname(socket_fd, (struct sockaddr*)&sockaddr, &sockaddr_len) != 0) {
        LOG.error("MatrixMasterThriftRpc: getsockname error %d", errno);
        return false;
    }

    char ip_str[INET_ADDRSTRLEN + 1];
    if (inet_ntop(AF_INET, &sockaddr.sin_addr, ip_str, sizeof(ip_str)) != NULL) {
        _host_ip = ip_str;
        LOG.info("MatrixMasterThriftRpc: local ip is %s", _host_ip.c_str());
    } else {
        LOG.error("MatrixMasterThriftRpc: inet_ntoa error %d", errno);
        return false;
    }

    char hostname_str[HOST_NAME_MAX + 1];
    if (gethostname(hostname_str, sizeof(hostname_str)) == 0) {
        _host_name = hostname_str;
        LOG.info("MatrixMasterThriftRpc: local hostname is %s", _host_name.c_str());
    } else {
        LOG.error("MatrixMasterThriftRpc: gethostname error %d", errno);
        return false;
    }
    return true;
    
}

bool MatrixMasterThriftRpc::init_connection()
{

    // no need to init connection.
    if (_client != NULL) {
        return true;
    }

    try {
        ::boost::shared_ptr<apache::thrift::transport::TSocket> 
            socket(new apache::thrift::transport::TSocket(_master_host_name.c_str(), 
                                                          _master_port));
        socket->setConnTimeout(_conn_timeo_ms);
        socket->setRecvTimeout(_read_timeo_ms);
        socket->setSendTimeout(_write_timeo_ms);

        ::boost::shared_ptr<apache::thrift::transport::TTransport> 
            transport(new apache::thrift::transport::TFramedTransport(socket));
        ::boost::shared_ptr<apache::thrift::protocol::TProtocol> 
            protocol(new apache::thrift::protocol::TBinaryProtocol(transport));

        _client = new MasterAgentProtocolClient(protocol);

        transport->open();

        if (get_host_info(socket->getSocketFD())) {
            _connected = true;

            LOG.info("MatrixMasterThriftRpc: open connection ok to %s(%s):%u",
                     _master_host_name.c_str(), 
                     socket->getPeerAddress().c_str(), 
                     _master_port);
            return true;
        } else {
            LOG.warn("MatrixMasterThriftRpc: failed to get host ip or name");
        }
    } catch (const apache::thrift::TException & e) {
        LOG.warn("MatrixMasterThriftRpc: open rpc error %s", e.what());
        _connected = false;
    } catch (const std::exception & e) {
        LOG.warn("MatrixMasterThriftRpc: open rpc error %s", e.what());
        _connected = false;
    }
    return false;
}

void MatrixMasterThriftRpc::close_connection()
{
    if(NULL != _client) {
        try {

            boost::shared_ptr<apache::thrift::protocol::TProtocol> protocol = _client->getInputProtocol();
            boost::shared_ptr<apache::thrift::transport::TTransport> transport = protocol->getTransport();
            transport->close();

        } catch (const std::exception &e) {
            LOG.warn("MatrixMasterThriftRpc: close rpc error %s", e.what());
        }

        delete _client;
        _client = NULL;
    }

    _connected = false;
}

}
