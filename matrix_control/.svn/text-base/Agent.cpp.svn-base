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

#include <assert.h>
#include <errno.h>
#include <cstring>

#include <iostream>
#include <sstream>
#include <string>


#include "op/oped/noah/sailor/utility/linkage/LinkagePeer.h"
#include "op/oped/noah/sailor/utility/linkage/Listener.h"
#include "op/oped/noah/sailor/utility/linkage/SimpleServer.h"
#include "op/oped/noah/sailor/utility/thread/MutexLocker.h"
#include "op/oped/noah/sailor/utility/Logger.h"
#include "op/oped/noah/sailor/utility/msleep.h"
#include "op/oped/noah/sailor/utility/revision.h"
#include "op/oped/noah/sailor/utility/safeio.h"
#include "op/oped/noah/sailor/utility/system.h"
#include "op/oped/noah/sailor/utility/cmdline.h"

#include "inf/computing/matrix/matrix-agent/configure.h"

#include "inf/computing/matrix/matrix-agent/Agent.h"

using sailor::LinkagePeer;
using sailor::Listener;
using sailor::MutexLocker;
using sailor::SimpleServer;
using sailor::ThreadPool;

namespace matrix {

Agent::Agent() try : _executor(_callback),
                     _manager(_executor),
                     _reporter(_manager, _rpc, _httpd),
                     _control_server(_manager),
                     _instance_monitor(_manager, _executor),
                     _machine_monitor(_manager),
                     _running(false), 
                     _restart(false), 
                     _run(false)
{
    _callback.set_manager(&_manager);
    _callback.set_monitor(&_instance_monitor);
} catch (std::exception& ex) {
    LOG.error("Agent: construct agent failed with msg [%s]", ex.what());
    // Construct agent failed means un-recoverable error happens(such as load meta failed)
    // So restart has no meaning. Just stop outself.
    // Do not let babysitter to restart.
    exit(EXIT_SUCCESS);
}

Agent::~Agent()
{
    do_shutdown();
}

int Agent::initialize()
{
    MutexLocker locker(&_mutex);
    assert(!_running);

    if (_executor.start() && 
        _manager.start() &&
        _reporter.start() &&
        _control_server.start() &&
        _machine_monitor.start() &&
        _instance_monitor.start()) {

        _httpd.start();
        LOG.info("Agent: Agent initialized ok.");
        _run = true;
        return 0;

    } else {
        LOG.error("Agent: Agent initialized failed!");
        do_shutdown();
        return -1;
    }

}

bool Agent::shutdown()
{
    _run = false;
    return true;
}

void Agent::do_shutdown()
{ 
    _httpd.stop();
    _instance_monitor.stop();
    _machine_monitor.stop();
    _control_server.stop();
    _reporter.stop();
    _manager.stop();
    _executor.stop();
}

void Agent::do_sleep()
{
    int remain = g_config->DEFAULT_SERVER_INTERVAL * 1000;
    while (remain) {
        remain = msleep_ex(remain, 1);
        if (!_run) {
            return;
        }
    }
}

bool Agent::run()
{
    MutexLocker locker(&_mutex);
    LOG.info("Agent: STARTED");
    _running = true;

    while (_run) {
        locker.unlock();
        LOG.trace("Agent: SLEEP OUT");
        do_sleep();
        locker.relock();
    }

    do_shutdown();

    LOG.info("Agent: STOPPED");
    _running = false;
    return true;
}

} // namespace matrix
