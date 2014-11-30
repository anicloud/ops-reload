/************************************************************************
 * Automated Operation Platform - Matrix Agent                          *
 * Copyright (C) 2012 yiyuanzhong@gmail.com (Yiyuan Zhong)              *
 *                                                                      *
 * This program is free software; you can redistribute it and/or        *
 * modify it under the terms of the GNU General Public License          *
 * as published by the Free Software Foundation; either version 2       *
 * of the License, or (at your option) any later version.               *
 *                                                                      *
 * This program is distributed in the hope that it will be useful,      *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of       *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        *
 * GNU General Public License for more details.                         *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, write to the Free Software          *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,           *
 * MA  02110-1301, USA.                                                 *
 ************************************************************************/

#ifndef __INF_COMPUTING_MATRIX_MATRIX_AGENT_EXECUTOR_NOAH_EXECUTOR_H__
#define __INF_COMPUTING_MATRIX_MATRIX_AGENT_EXECUTOR_NOAH_EXECUTOR_H__

#include <stdint.h>

#include <list>
#include <string>

#include "op/oped/noah/sailor/utility/thread/Mutex.h"

#include "inf/computing/matrix/matrix-agent/executor/Executor.h"

namespace sailor {
class SimpleClient;
class ThreadPool;
} // namespace sailor

namespace matrix {

class InstanceInfo;

class NoahExecutor : public Executor {
public:
    NoahExecutor(Executor::Callback& cb);
    virtual ~NoahExecutor();

    bool start();
    bool stop();

    void install(const InstanceInfo& info, uint64_t generation);
    void update(const InstanceInfo& info, uint64_t generation);
    void remove(const InstanceInfo& info, uint64_t generation);
    void status(const InstanceInfo& info, uint64_t generation);
    void stop(uint64_t generation);

private:
    class SimpleHandshaker;
    class SimpleCallback;
    class SimpleWorker;
    class SimpleNotice;
    class Order;

    static bool save_configuration(const InstanceInfo& info, 
                                   const std::string& conf_file);
    static std::string prepare_cmd(const InstanceInfo& info,
                                   const std::string& conf_file,
                                   const std::string& action);

    static bool parse_uuid(const std::string &uuid, uint64_t *generation);
    static void make_uuid(uint64_t generation, std::string *uuid);

    void notify_order_processer(Order* order);

    bool process_message(const std::string &message);
    void process_errors(bool critical);

    bool process_order_status(const InstanceInfo& info, 
                              uint64_t generation);
    bool process_order_install(const InstanceInfo &info, 
                               uint64_t generation);
    bool process_order_update(const InstanceInfo &info, 
                              uint64_t generation);
    bool process_order_deploy(const InstanceInfo& info, 
                              uint64_t generation, 
                              bool is_update);
    bool process_order_remove(const InstanceInfo &info, 
                              uint64_t generation);
    bool process_order_stop(uint64_t generation);

    bool process_order(Order *order);
    bool process_orders();

    SimpleNotice *_notice;
    SimpleWorker *_worker;
    SimpleCallback *_callback;
    ::sailor::SimpleClient *_client;
    ::sailor::ThreadPool* _pool;

    std::list<Order*> _orders;
    ::sailor::Mutex _mutex;

}; // class NoahExecutor

} // namespace matrix

#endif // __INF_COMPUTING_MATRIX_MATRIX_AGENT_EXECUTOR_NOAH_EXECUTOR_H__
