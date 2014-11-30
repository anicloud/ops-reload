#include <sys/types.h>
#include <stdint.h>

#include "op/oped/noah/sailor/utility/Logger.h"
#include "op/oped/noah/sailor/utility/thread/MutexLocker.h"

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/httpd/MongooseHttpd.h"

namespace matrix {

MongooseHttpd::MongooseHttpd() : _ctx(NULL),
                                 _doc_root(g_config->MATRIX_DEPLOY_LOG_PATH), 
                                 _listen_port(g_config->AGENT_HTTPD_PORT)
{
}

MongooseHttpd::~MongooseHttpd()
{
    stop();
}

bool MongooseHttpd::start(int32_t listen_port)
{
    ::sailor::MutexLocker locker(&_mutex);
    
    if (_ctx != NULL) {
        // already started. stop it first.
        return false;
    }

    // update port if necessary.
    if (listen_port > 0) {
        _listen_port = listen_port;
    }
    //
    const char * options[5];

    char port_buffer[6];
    snprintf(port_buffer, sizeof(port_buffer),"%u", _listen_port);

    options[0] = "document_root";
    options[1] = _doc_root.c_str();
    options[2] = "listening_ports";
    options[3] = port_buffer;
    options[4] = NULL;

    _ctx  = mg_start(NULL, NULL, options);
    return _ctx != NULL;
}

void MongooseHttpd::stop()
{
    ::sailor::MutexLocker locker(&_mutex);

    if(NULL != _ctx) {
        mg_stop((struct mg_context *)_ctx);
        _ctx = NULL;
    }
}

int32_t MongooseHttpd::get_listen_port() const
{
    return _listen_port;
}

}
