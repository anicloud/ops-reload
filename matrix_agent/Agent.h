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

#include <list>
#include <string>

#include "op/oped/noah/sailor/utility/thread/Mutex.h"

#include "inf/computing/matrix/matrix-agent/reporter/MatrixReporter.h"
#include "inf/computing/matrix/matrix-agent/rpc/MatrixThriftRpc.h"
#include "inf/computing/matrix/matrix-agent/rpc/AgentControlThriftServer.h"
#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/executor/NoahExecutor.h"
#include "inf/computing/matrix/matrix-agent/monitor/InstanceMonitor.h"
#include "inf/computing/matrix/matrix-agent/monitor/MachineMonitor.h"
#include "inf/computing/matrix/matrix-agent/httpd/MongooseHttpd.h"

namespace sailor {
class SimpleServer;
} // namespace sailor

namespace matrix {

class Agent {
public:

    Agent();
    virtual ~Agent();

    int initialize();
    bool shutdown();
    bool run();

    bool request_restart() const
    {
        return _restart;
    }

private:
    void do_shutdown();
    void do_sleep();

    MongooseHttpd _httpd;
    MatrixExecutorCallback _callback;
    NoahExecutor _executor;
    MatrixMasterThriftRpc _rpc;
    MatrixManager _manager;
    MatrixReporter _reporter;
    AgentControlThriftServer _control_server;
    InstanceMonitor _instance_monitor;
    MachineMonitor _machine_monitor;

    bool _running;
    bool _restart;
    bool _run;

    mutable ::sailor::Mutex _mutex;

}; // class Agent

} // namespace matrix
