#ifndef __MATRIX_THRIFT_RPC_H__
#define __MATRIX_THRIFT_RPC_H__

#include <netinet/in.h>
#include <stdint.h>


#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>

#include "op/oped/noah/sailor/utility/thread/Mutex.h"

#include "inf/computing/matrix/matrix-agent/protocol/gen-cpp/MasterAgentProtocol.h" 
#include "inf/computing/matrix/matrix-agent/rpc/MatrixRpc.h"
#include "inf/computing/matrix/matrix-agent/reporter/Heartbeat.h"
#include "inf/computing/matrix/matrix-agent/manager/HostInfo.h"
#include "inf/computing/matrix/matrix-agent/manager/InstanceInfo.h"
#include "inf/computing/matrix/matrix-agent/configure.h"


namespace matrix {

// thrift rpc implement.
class MatrixMasterThriftRpc : public MatrixMasterRpc
{
public:
    MatrixMasterThriftRpc();
    virtual ~MatrixMasterThriftRpc();

    HeartbeatResponse heartbeat(const HeartbeatMessage& msg) throw (MatrixRpcException);

private:
    /**
     * @warning Caller should aquire _mutex
     */
    bool init_connection();
    void close_connection();
    //
    bool get_host_info(int32_t socket_fd);

public:
    /**
     * Convert between Matrix and Thrift data structure.
     */
    //
    static ThriftHeartbeatMessage to_thrift(const HeartbeatMessage& msg,
                                            const std::string& host_ip,
                                            const std::string& host_name);
    static void to_thrift(const HostInfo& info,
                          const std::string& host_ip,
                          const std::string& host_name,
                          ThriftHostInfo& t_info);
    //
    static void to_thrift(const ResourceTuple& resource, ThriftResourceTuple& t_resource);
    static void to_thrift(const InstanceInfo& instance, ThriftInstanceInfo& t_info);
    static void to_thrift(const InstanceMeta& meta, ThriftAgentInstanceMeta& t_meta);
    //
    static HeartbeatResponse to_matrix(const ThriftHeartbeatResponse& t_res);
    static AgentConf to_matrix(const ThriftAgentConf& t_conf);
    static InstanceInfo to_matrix(const ThriftInstanceInfo& t_instance);
    static ResourceTuple to_matrix(const ThriftResourceTuple& t_resource);
    static InstanceMeta to_matrix(const ThriftAgentInstanceMeta& t_meta);

private:
    MasterAgentProtocolClient* _client;
    std::string _master_host_name;
    int32_t  _master_port;
    int32_t _conn_timeo_ms;
    int32_t _read_timeo_ms;
    int32_t _write_timeo_ms;
    //
    bool _connected;
    std::string _host_ip;
    std::string _host_name;
    //
    mutable ::sailor::Mutex _mutex;
};

}

#endif
