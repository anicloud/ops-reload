#ifndef __MATRIX_MONGOOSE_HTTPD__
#define __MATRIX_MONGOOSE_HTTPD__

#include <string>
#include <sys/types.h>
#include <stdint.h>

#include "op/oped/noah/sailor/utility/thread/Mutex.h"

#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/httpd/MatrixHttpd.h"
#include "inf/computing/matrix/matrix-agent/httpd/mongoose.h"


namespace matrix {

class MongooseHttpd : public MatrixHttpd
{
public:
    MongooseHttpd();
    virtual ~MongooseHttpd();

public:
    /**
     * if listen_port != 0, use new port to start.
     */
    bool start(int32_t listen_port = 0);
    void stop();
    int32_t get_listen_port() const;

private:
    void * _ctx;
    const std::string _doc_root;
    int32_t _listen_port;
    //
    mutable ::sailor::Mutex _mutex;
};

}

#endif
