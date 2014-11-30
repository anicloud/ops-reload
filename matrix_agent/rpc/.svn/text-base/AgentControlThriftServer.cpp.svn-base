#include "AgentControlThriftServer.h"

#include "inf/computing/matrix/matrix-agent/configure.h"

namespace matrix
{
    
AgentControlThriftServer::AgentControlThriftServer(MatrixManager& manager):_manager(manager), _server(NULL), _thread(NULL)
{
    // nothing
}

AgentControlThriftServer::~AgentControlThriftServer()
{
    stop();
}

bool AgentControlThriftServer::start()
{
    ::boost::mutex::scoped_lock locker(_mutex);
    if (_thread != NULL) {
        return true;
    }
    
    _thread = new ::boost::thread(::boost::bind(&AgentControlThriftServer::serve, this));
    return _thread != NULL;
}

void AgentControlThriftServer::stop()
{
    ::boost::mutex::scoped_lock locker(_mutex);
    if (_thread == NULL) {
        return;
    }
    if (_server == NULL) {
        throw std::runtime_error("Stop server but _server is NULL");
    }
    
    _server->stop();
    _thread->join();
    
    delete _server;
    delete _thread;
    _server = NULL;
    _thread = NULL;
}


void AgentControlThriftServer::serve()
{
    assert(g_config);
    if (_server != NULL) {
        return;
    }
    int32_t port = g_config->AGENT_CONTROL_PORT;
    
    ::boost::shared_ptr<AgentControlHandler> handler(new AgentControlHandler(_manager));
    ::boost::shared_ptr<apache::thrift::TProcessor> processor(new AgentControlProtocolProcessor(handler));
    ::boost::shared_ptr<apache::thrift::transport::TServerTransport> serverTransport(new apache::thrift::transport::TServerSocket(port));
    ::boost::shared_ptr<apache::thrift::transport::TTransportFactory> transportFactory(new apache::thrift::transport::TFramedTransportFactory());
    ::boost::shared_ptr<apache::thrift::protocol::TProtocolFactory> protocolFactory(new apache::thrift::protocol::TBinaryProtocolFactory());
    
    _server = new apache::thrift::server::TSimpleServer(processor, serverTransport, transportFactory, protocolFactory);
    _server->serve();
}


AgentControlThriftServer::AgentControlHandler::AgentControlHandler(MatrixManager& manager):_manager(manager)
{
    // nothing
}

AgentControlThriftServer::AgentControlHandler::~AgentControlHandler()
{
    // nothing
}

bool AgentControlThriftServer::AgentControlHandler::getFreeze()
{
    return _manager.get_freeze();
}

void AgentControlThriftServer::AgentControlHandler::setFreeze(bool freeze)
{
    _manager.set_freeze(freeze);
}

bool AgentControlThriftServer::AgentControlHandler::resetInstance(const std::string& instance_name)
{
    return _manager.reset_instance(instance_name);
}

}

