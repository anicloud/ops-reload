#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_MACHINE_MONITOR_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_MACHINE_MONITOR_H__

#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/common/MatrixThread.h"

namespace matrix {

class MachineMonitor : public MatrixThread
{
public:
    MachineMonitor(MatrixManager& manager);
    virtual ~MachineMonitor();
    //
    int64_t get_cpu_num();
    int64_t get_total_memory();
    int64_t get_total_disk();

protected:
    virtual void run();
    virtual void on_timeout();
    // UT should override this method to avoid exit.
    virtual void die();
private:
    bool monitor_disk();

private:
    MatrixManager& _manager;
    const std::string _matrix_root;
};

} /// namespace matrix

#endif /// __INF_COMPUTING_MATRIX_MATRIX_AGENT_MACHINE_MONITOR_H__
