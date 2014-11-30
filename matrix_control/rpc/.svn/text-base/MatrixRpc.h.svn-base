#ifndef __MATRIX_RPC_BASE_H__
#define __MATRIX_RPC_BASE_H__

#include <stdexcept>

#include "inf/computing/matrix/matrix-agent/reporter/Heartbeat.h"

namespace matrix {


class MatrixRpcException : public std::runtime_error
{
public:
    MatrixRpcException(const std::string& what) : runtime_error(what) {}
    ~MatrixRpcException() throw () {}
};

class MatrixMasterRpc
{
public:
    MatrixMasterRpc() {}

    virtual ~MatrixMasterRpc() {}

    /**
     * Heartbeat to matrix master.
     *
     * @throw MatrixRpcException if rpc failed.
     */
    virtual HeartbeatResponse heartbeat(const HeartbeatMessage& msg) throw (MatrixRpcException) = 0;
};

}

#endif
