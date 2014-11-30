#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_EXECUTOR_EXECUTOR_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_EXECUTOR_EXECUTOR_H__

#include <stdint.h>
#include <sys/types.h>

#include "inf/computing/matrix/matrix-agent/manager/InstanceInfo.h"

namespace matrix {

/**
 * All cmd in matrix are executed by Executor.
 * When cmd finished, callback will be called.
 */
class Executor {

public:
    class Callback {
    public:
        virtual ~Callback() {}

        virtual void on_deploy(uint64_t generation, bool succeeded) = 0;

        virtual void on_remove(uint64_t generation, bool succeeded) = 0;

        virtual void on_status(uint64_t generation, 
                               InstanceInfo::HealthCheckStatus::type status) = 0;
    }; // class Callback

public:

    Executor(Callback& callback) : _cb(callback)
    {
    }

    virtual ~Executor() {}

    /**
     * Async interface.
     * Caller will not be blocked.
     */
    virtual void install(const InstanceInfo &info, uint64_t generation) = 0;
    virtual void update(const InstanceInfo &info, uint64_t generation) = 0;
    virtual void remove(const InstanceInfo &info, uint64_t generation) = 0;
    virtual void status(const InstanceInfo& info, uint64_t generation) = 0;
    virtual void stop(uint64_t generation) = 0;

protected:
    Callback& _cb;

};


/**
 * Matrix executor callback.
 * Will redirect cmd's response to MatrixManager & InstanceMonitor.
 */
class MatrixManager;
class InstanceMonitor;
class MatrixExecutorCallback : public Executor::Callback
{
public:
    MatrixExecutorCallback();
    ~MatrixExecutorCallback();
    //
    void set_manager(MatrixManager* manager);
    void set_monitor(InstanceMonitor* monitor);
    //
    virtual void on_deploy(uint64_t generation, bool succeeded);
    virtual void on_remove(uint64_t generation, bool succeeded);
    virtual void on_status(uint64_t generation, 
                           InstanceInfo::HealthCheckStatus::type status);

private:
    MatrixManager* _manager;
    InstanceMonitor* _monitor;
};
 
}

#endif
