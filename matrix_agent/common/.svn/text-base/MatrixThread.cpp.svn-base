#include <sys/time.h>

#include <cerrno>

#include "op/oped/noah/sailor/utility/Logger.h"
#include "op/oped/noah/sailor/utility/msleep.h"
#include "op/oped/noah/sailor/utility/thread/MutexLocker.h"

#include "inf/computing/matrix/matrix-agent/common/Utility.h"
#include "inf/computing/matrix/matrix-agent/common/MatrixThread.h"

namespace matrix
{

void* MatrixThread::fn_monitor_thread(void* param)
{
    MatrixThread* p = (MatrixThread*) param;
    ::sailor::Logger::thread_attach();
    while(!p->_quit_flag) {

        LOG.info("<%s[Monitor]> start monitor timeout.", p->_name.c_str());

        int64_t non_active_time_ms = get_current_timestamp_ms() - p->get_last_active_timestamp();
        if (non_active_time_ms > p->_monitor_timeout_ms) {
            LOG.warn("<%s[Monitor]> non_active time [%ld], timeout[%d]", 
                     p->_name.c_str(),
                     non_active_time_ms,
                     p->_monitor_timeout_ms);
            p->on_timeout();
        }

        LOG.info("<%s[Monitor]> finish monitor timeout.", p->_name.c_str());

        int delay = p->_interval_ms;
        while (!p->_quit_flag && delay > 0) {
            int now = delay >= 1000 ? 1000 : delay;
            msleep(now);
            delay -= now;
        }
    }
    LOG.info("<%s[Monitor]> quit, tid is %lx", p->_name.c_str(), p->_tid);
    ::sailor::Logger::thread_detach();
    return NULL;
}

void * MatrixThread::fn_thread(void* param)
{
    MatrixThread * p = (MatrixThread *) param;
    ::sailor::Logger::thread_attach();
    p->_running = true;

    while(!p->_quit_flag) {

        // update active time.
        p->set_last_active_timestamp(get_current_timestamp_ms());

        p->run();

        int delay = p->_interval_ms;
        while (!p->_quit_flag && delay > 0) {
            int now = delay >= 1000 ? 1000 : delay;
            msleep(now);
            delay -= now;
        }
    }
    LOG.info("<%s>  quit, tid is %lx", p->_name.c_str(), p->_tid);
    p->_running = false;
    ::sailor::Logger::thread_detach();
    return NULL;
}

bool MatrixThread::start()
{
    ::sailor::MutexLocker locker(&_thread_mutex);

    if(_tid == 0) {

        _quit_flag = false;
        // set last active timestamp to current time when start.
        set_last_active_timestamp(get_current_timestamp_ms());

        if (pthread_create(&_tid, NULL, fn_thread, this) != 0) {
            LOG.error("<%s> pthread_create failed [%d]", _name.c_str(), errno);
            return false;
        } else {
            LOG.info("<%s> pthread_create ok, tid is %lx", _name.c_str(), _tid);
        }
        // init monitor thread
        if (_monitor_timeout_ms > 0) {
            if (pthread_create(&_monitor_tid, NULL, fn_monitor_thread, this) != 0) {
                LOG.error("<%s[Monitor]> pthread_create failed [%d]", _name.c_str(), errno);
                stop_internal();
                return false;
            } else {
                LOG.info("<%s[Monitor]> pthread_create ok, tid is %lx", _name.c_str(), _tid);
            }
        }
        
    } else {
        LOG.warn("<%s> already running", _name.c_str());
    }

    return true;
}

void MatrixThread::stop_internal()
{
    _quit_flag = true;
    void* retptr = NULL;

    if(_monitor_tid != 0) {
        pthread_join(_monitor_tid, &retptr);
        _monitor_tid = 0;
    }

    if(_tid != 0) {
        pthread_join(_tid, &retptr);
        _tid = 0;
    }

}

void MatrixThread::stop()
{
    ::sailor::MutexLocker locker(&_thread_mutex);
    stop_internal();
}

void MatrixThread::set_last_active_timestamp(int64_t timestamp)
{
    ::sailor::MutexLocker locker(&_timestamp_mutex);
    _last_active_timestamp = timestamp;
}

int64_t MatrixThread::get_last_active_timestamp()
{
    ::sailor::MutexLocker locker(&_timestamp_mutex);
    return _last_active_timestamp;
}

}
