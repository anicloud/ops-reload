#ifndef __MATRIX_HTTPD_H__
#define __MATRIX_HTTPD_H__

#include <string>
#include <stdint.h>

namespace matrix {

class MatrixHttpd
{
public:
    virtual ~MatrixHttpd() {}

public:
    /**
     * if listen_port != 0, use new port to start.
     */
    virtual bool start(int32_t listen_port = 0) = 0;
    virtual void stop() = 0;
    virtual int32_t get_listen_port() const = 0;
};

}
#endif
