#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_RPC_AGENT_CONTROL_THRIFT_SERVER_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_RPC_AGENT_CONTROL_THRIFT_SERVER_H__

#include <netinet/in.h>

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <thrift/transport/TSocket.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>

#include "inf/computing/matrix/matrix-agent/protocol/gen-cpp/AgentControlProtocol.h"

#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"

namespace matrix
{
    
class AgentControlThriftServer
{
public:
    //AgentControlThriftServer(){};
    explicit AgentControlThriftServer(MatrixManager& manager);
    virtual ~AgentControlThriftServer();
    
    bool start();
    void stop ();
    
private:
    void serve();
    
    MatrixManager& _manager;
    ::apache::thrift::server::TSimpleServer * _server;
    ::boost::thread * _thread;
    ::boost::mutex _mutex;

private:
    class AgentControlHandler : virtual public AgentControlProtocolIf
    {
    public:
        explicit AgentControlHandler(MatrixManager& manager);
        virtual ~AgentControlHandler();

        bool getFreeze();
        void setFreeze(bool freeze);
        bool resetInstance(const std::string& instance_name);
    private:
        MatrixManager& _manager;
    };
};

}       // namespace matrix

#endif  // __INF_COMPUTING_MATRIX_MATRIX_AGENT_RPC_AGENT_CONTROL_THRIFT_SERVER_H__

