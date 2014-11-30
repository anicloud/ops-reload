#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_REPORTER_MATRIX_REPORTER_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_REPORTER_MATRIX_REPORTER_H__

#include "inf/computing/matrix/matrix-agent/common/MatrixThread.h"

namespace matrix {

class MatrixManager;
class MatrixMasterRpc;
class MatrixHttpd;
class HeartbeatMessage;
class HeartbeatResponse;

/**
 * Report agent's actual instances to master and fecth expected instances from master.
 * Update agent's conf as ordered by master.
 * 
 * @param manager: 
 * @param rpc
 * @param httpd
 */
class MatrixReporter : public MatrixThread
{ 
public:
    MatrixReporter(MatrixManager& manager, 
                   MatrixMasterRpc& rpc,
                   MatrixHttpd& httpd);
    virtual ~MatrixReporter();


private:
    void run();
    //
    void dump_hb_msg(const HeartbeatMessage& msg) const;
    void dump_hb_res(const HeartbeatResponse& res) const;

private:
    MatrixManager& _manager;
    MatrixMasterRpc& _rpc;
    MatrixHttpd& _httpd;

    bool _init_heartbeat;
};

}

#endif // __INF_COMPUTING_MATRIX_MATRIX_AGENT_REPORTER_MATRIX_REPORTER_H__
