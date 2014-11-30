#ifndef __MATRIX_THREAD_H__
#define __MATRIX_THREAD_H__

#include <pthread.h>
#include <string>
#include <stdint.h>

#include "op/oped/noah/sailor/utility/thread/Mutex.h"

namespace matrix
{

/*
 * Matrix's wrapper upon pthread to implement loop runner.
 * Subclass should override the *run()* function. 
 * The *run()* function will be called periodically according to *interval_ms*.
 * If *monitor_timeout_ms* is positive, another monitor thread will be initialized.
 * The monitor thread will monitor the main thread to see if it is blocked.
 * If the main thread is blocked for *monitor_timeout_ms*, *the on_timeout()* function will be called.
 */
class MatrixThread
{
public:
    virtual ~MatrixThread()
    {
        // nothing
    }
public:
    MatrixThread() : _name("UNKNOWN"),
                     _tid(0),
                     _quit_flag(true),
                     _running(false), 
                     _interval_ms(0),
                     _monitor_tid(0),
                     _monitor_timeout_ms(0),
                     _last_active_timestamp(0)
    {
        // nothing
    }

    MatrixThread(const std::string & name, 
                 int32_t interval_ms,
                 int32_t monitor_timeout_ms = 0) : _name(name), 
                                         _tid(0),
                                         _quit_flag(true),
                                         _running(false), 
                                         _interval_ms(interval_ms),
                                         _monitor_tid(0),
                                         _monitor_timeout_ms(monitor_timeout_ms),
                                         _last_active_timestamp(0)
    {
        // nothing
    }

    /*
     * start current thread and monitor thread if necessary.
     */
    bool start();
    /*
     * stop current thread and monitor thread if necessary.
     * wait until the thread is stoped.
     */
    void stop();
    
    void set_name(const std::string & name)
    {
        _name = name;
    }

    void set_interval(int32_t intervalMS)
    {
        _interval_ms = intervalMS;
    }

    int32_t get_interval()
    {
        return _interval_ms;
    }
    
    bool get_running()
    {
        return _running;
    }

protected:
    std::string _name;
    static void * fn_thread(void* param);
    // thread function for monitor thread.
    static void* fn_monitor_thread(void* param);
    virtual void run() = 0;
    /*
     * Will be called when timeout happen.
     * Subclass can override this method to receive notification.
     */
    virtual void on_timeout() {}

private:
    /* 
     * stop current thread & monitor thread without require lock.
     * caller should ensure the lock is required.
     */
    void stop_internal();
    //
    pthread_t _tid;
    bool _quit_flag;
    bool _running;
    int32_t _interval_ms;
    ::sailor::Mutex _thread_mutex;
    // for timeout monitor thread.
    pthread_t _monitor_tid;
    int32_t _monitor_timeout_ms;
    // Record the last timestamp that the main thread is active.
    ::sailor::Mutex _timestamp_mutex;
    int64_t _last_active_timestamp;
    int64_t get_last_active_timestamp();
    void set_last_active_timestamp(int64_t timestamp);
};

}

#endif
