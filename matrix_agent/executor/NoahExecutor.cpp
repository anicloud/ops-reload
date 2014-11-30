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

#include "inf/computing/matrix/matrix-agent/executor/Executor.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#include <stdexcept>
#include <string>
#include <sstream>
#include <cerrno>


#include "op/oped/noah/sailor/protocol/executor.pb.h"

#include "op/oped/noah/sailor/utility/linkage/EnvelopeHandshaker.h"
#include "op/oped/noah/sailor/utility/linkage/FileEvent.h"
#include "op/oped/noah/sailor/utility/linkage/Linkage.h"
#include "op/oped/noah/sailor/utility/linkage/LinkageAdapter.h"
#include "op/oped/noah/sailor/utility/linkage/SimpleClient.h"
#include "op/oped/noah/sailor/utility/linkage/UuidEnvelope.h"
#include "op/oped/noah/sailor/utility/thread/MutexLocker.h"
#include "op/oped/noah/sailor/utility/File.h"
#include "op/oped/noah/sailor/utility/Logger.h"
#include "op/oped/noah/sailor/utility/cmdline.h"
#include "op/oped/noah/sailor/utility/safeio.h"
#include "op/oped/noah/sailor/utility/thread/ThreadPool.h"

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/manager/InstanceInfo.h"
#include "inf/computing/matrix/matrix-agent/executor/NoahExecutor.h"

using sailor::EnvelopeHandshaker;
using sailor::File;
using sailor::FileEvent;
using sailor::Linkage;
using sailor::LinkageAdapter;
using sailor::MutexLocker;
using sailor::SimpleClient;
using sailor::ThreadPool;
using sailor::UuidEnvelope;

namespace matrix {

class NoahExecutor::SimpleHandshaker : public EnvelopeHandshaker {
public:
    SimpleHandshaker() : EnvelopeHandshaker(new UuidEnvelope(1024, 8192)) {}
    virtual ~SimpleHandshaker() {}

protected:
    virtual bool process(const std::string &message, Action *next_action);
    virtual void get_initial_action(Action *initial_action);

}; // class NoahExecutor::SimpleHandshaker

class NoahExecutor::SimpleCallback : public SimpleClient::Callback {
public:
    SimpleCallback(NoahExecutor *executor) : _executor(executor) {}
    virtual ~SimpleCallback() {}

    virtual bool connect(Linkage *linkage);
    virtual void on_error(bool critical);

private:
    NoahExecutor *_executor;

}; // class NoahExecutor::SimpleCallback

class NoahExecutor::SimpleWorker : public LinkageAdapter {
public:
    SimpleWorker(NoahExecutor *executor) : LinkageAdapter(new UuidEnvelope(1024, 8192),
                                                      new SimpleHandshaker)
                                     , _executor(executor) {}

    virtual bool process_message(const std::string &message);

private:
    NoahExecutor *_executor;

}; // class NoahExecutor::SimpleWorker

class NoahExecutor::SimpleNotice : public FileEvent {
public:
    SimpleNotice(NoahExecutor *executor) : _executor(executor) {}

protected:
    virtual bool on_wake();

private:
    NoahExecutor *_executor;

}; // class NoahExecutor::SimpleNotice

class NoahExecutor::Order {
public:
    enum Type{
        TypeInstall,
        TypeUpdate,
        TypeRemove,
        TypeStop,
        TypeStatus,
    };

    Order(const Type &type, 
          const InstanceInfo* info, 
          uint64_t generation) : _info(NULL), 
                                 _generation(generation), 
                                 _type(type)
    {
        if (info) {
            _info = new InstanceInfo(*info);
        }
    }

    ~Order()
    {
        delete _info;
        _info = NULL;
    }

    InstanceInfo* _info;
    uint64_t _generation;
    Type _type;

}; // class NoahExecutor::Order

bool NoahExecutor::SimpleHandshaker::process(const std::string &message, Action *next_action)
{
    ::sailor::serializable::Protocol protocol;
    if (!protocol.ParseFromString(message)                      ||
        protocol.type() != ::sailor::serializable::ProtocolNone ||
        !protocol.has_buffer()                                  ){

        return false;
    }

    LOG.info("NoahExecutor: HANDSHAKE [%s]", protocol.buffer().c_str());
    return true;
}

void NoahExecutor::SimpleHandshaker::get_initial_action(Action *initial_action)
{
    ::sailor::serializable::Protocol protocol;
    protocol.set_type(::sailor::serializable::ProtocolNone);
    protocol.set_uuid("48f22730-2424-42d3-b1fe-60064fff437d");
    protocol.SerializeToString(&initial_action->_message);
    initial_action->_message_wanted = true;
    initial_action->_need_ack = true;
}

bool NoahExecutor::SimpleCallback::connect(Linkage *linkage)
{
    assert(linkage == _executor->_worker);
    LinkageAdapter *adapter = static_cast<LinkageAdapter *>(linkage);

    if (!adapter->connect_unix(g_config->EXECUTOR_SOCKET_NAME, false)) {
        return false;
    }

    return true;
}

void NoahExecutor::SimpleCallback::on_error(bool critical)
{
    _executor->process_errors(critical);
}

bool NoahExecutor::SimpleWorker::process_message(const std::string &message)
{
    return _executor->process_message(message);
}

bool NoahExecutor::SimpleNotice::on_wake()
{
    return _executor->process_orders();
}

NoahExecutor::NoahExecutor(Executor::Callback& cb) : Executor(cb)
{
    _pool = new ThreadPool();
    _worker = new SimpleWorker(this);
    _notice = new SimpleNotice(this);
    _callback = new SimpleCallback(this);
    _client = new SimpleClient(_pool, _worker, _callback, _notice);
}

NoahExecutor::~NoahExecutor()
{
    stop();
    delete _client;
    delete _callback;
    delete _notice;
    delete _worker;
    delete _pool;
}

bool NoahExecutor::start()
{
    assert(g_config);

    MutexLocker locker(&_mutex);

    if (!_pool->initialize()) {
        return false;
    }

    if (!_client->initialize()) {
        return false;
    }

    return true;
}

bool NoahExecutor::stop()
{

    if (!_client->shutdown()) {
        return false;
    }

    _pool->shutdown();

    return true;
}

bool NoahExecutor::save_configuration(const InstanceInfo& info, const std::string &conf_file)
{
    errno = 0;
    char host_name[HOST_NAME_MAX + 1];
    if (gethostname(host_name, sizeof(host_name)) != 0) {
        LOG.warn("NoahExecutor: could not get host name, errno: %d", errno);
        return false;
    }

    File file(conf_file);
    if (!file.open_for_writing()) {
        LOG.warn("NoahExecutor: failed to [%s] for writing: %d",
                 conf_file.c_str(), errno);

        return false;
    }

    const InstanceMeta& instance_meta = info.get_instance_meta();
    
    std::string container_dir = std::string(g_config->MATRIX_CONTAINERS_PATH) + "/" + 
                               info.get_instance_name() + "/";
    // generate instance.conf
    std::ostringstream buffer;

    buffer << "serviceName : " << info.get_service_name() << std::endl;
    buffer << "offset : " << info.get_offset() << std::endl;
    buffer << "packageSource : " << instance_meta._package_source << std::endl;
    buffer << "packageVersion : " << instance_meta._package_version << std::endl;
    buffer << "containerDir : " << container_dir << std::endl;
    buffer << "deployDir : " << instance_meta._deploy_dir << std::endl;
    buffer << "group : " << instance_meta._group << std::endl;
    buffer << "port :" << std::endl;
    for (std::map<std::string, int32_t>::const_iterator iter = instance_meta._port.begin();
            iter != instance_meta._port.end(); ++iter)
    {
        buffer << "    " << iter->first << " : " << iter->second << std::endl;
    }
    buffer << "minPortInclude : " << instance_meta._min_port_include << std::endl;
    buffer << "maxPortInclude : " << instance_meta._max_port_include << std::endl;
    buffer << "processName :" << std::endl;
    for (std::map<std::string, std::string>::const_iterator 
            iter = instance_meta._process_name.begin();
            iter != instance_meta._process_name.end(); ++iter)
    {
        buffer << "    " << iter->first << " : " << iter->second << std::endl;
    }
    buffer << "tag :" << std::endl;
    for (std::map<std::string, std::string>::const_iterator 
            iter = instance_meta._tag.begin();
            iter != instance_meta._tag.end(); ++iter)
    {
        buffer << "    " << iter->first << " : " << iter->second << std::endl;
    }
    buffer << "hostName : " << host_name << std::endl;
    buffer << "containerId : " << info.get_instance_name() << std::endl;

    // 5s should be enough to write this thing down.
    if (!file.write_all(buffer.str(), 5000)) {
        LOG.warn("NoahExecutor: failed to write to [%s]: %d",
                 conf_file.c_str(), errno);

        file.cancel();
        return false;
    }

    if (!file.close()) {
        LOG.warn("NoahExecutor: failed to commit [%s]: %d",
                 conf_file.c_str(), errno);

        return false;
    }

    return true;
}

std::string NoahExecutor::prepare_cmd(const InstanceInfo& info, 
                               const std::string& conf_file, 
                               const std::string& action)
{
    std::string instance_name = info.get_instance_name();
    std::string instance_path = std::string(g_config->MATRIX_CONTAINERS_PATH) + "/" + instance_name;
    const InstanceMeta& instance_meta = info.get_instance_meta();

    //splice cmd
    std::ostringstream cmd;
    cmd << g_config->ADAPTER_SCRIPTS_PATH << "/main " << action << " ";   // operation UPDATE
    cmd << "-j ";                                                         // always in jail
    cmd << "-c " << '"' << instance_path << "\" ";                        // container path
    cmd << "-i " << '"' << conf_file << "\" ";                            // conf file name
    cmd << "-s " << '"' << info.get_service_name() << "\" ";              // servic name
    cmd << "-w " << '"' << instance_meta._deploy_dir << "\" ";      // working dir
    cmd << "-a " << '"' << InstanceMeta::PackageType::to_string(
                               instance_meta._package_type) << "\" ";     // package type
    cmd << "-l " << '"' << g_config->MATRIX_DEPLOY_LOG_PATH << '"';       // deploy log

    LOG.trace("NoahExecutor: cmd %s", cmd.str().c_str());

    return cmd.str();

}

bool NoahExecutor::process_order_update(const InstanceInfo& info, uint64_t generation) {
    return process_order_deploy(info, generation, true);
}

bool NoahExecutor::process_order_install(const InstanceInfo& info, uint64_t generation) {
    return process_order_deploy(info, generation, false);
}

bool NoahExecutor::process_order_deploy(const InstanceInfo &info, 
                                    uint64_t generation,
                                    bool is_update)
{
    std::string instance_name = info.get_instance_name();

    std::ostringstream conf_file_buffer;
    conf_file_buffer << g_config->MATRIX_TMP_PATH << '/' << instance_name << '_' << generation;
    std::string conf_file = conf_file_buffer.str();

    if (!save_configuration(info, conf_file)) {
        return false;
    }

    std::string cmd = prepare_cmd(info, 
                                  conf_file, 
                                  is_update ? "UPDATE" : "INSTALL");

    std::string uuid;
    make_uuid(generation, &uuid);

    // set cpu and memory in cgroup
    const InstanceMeta& instance_meta = info.get_instance_meta();

    const ResourceTuple& resource = instance_meta._resource;
    std::string cpu, memory;
    std::stringstream buffer;
    buffer.str("");
    buffer.clear();
    buffer << resource._cpu_num * 10;
    buffer >> cpu;
    buffer.str("");
    buffer.clear();
    buffer << resource._memory_mb * 1048576L;
    buffer >> memory;

    ::sailor::serializable::TaskInfo t;
    t.set_uuid(uuid);
    t.set_entry_point(cmd);
    t.set_privileged(true);
    t.set_emulator(::sailor::serializable::EmulatorSsh);

    ::sailor::serializable::Cgroups &c = *t.mutable_cgroups();
    c.set_name(instance_name);
    c.set_auto_release(false);
    c.add_subsystems("cpu");
    c.add_subsystems("memory");
    c.add_subsystems("cpuacct");

    ::sailor::serializable::Cgroups::Parameter *param;
    param = c.add_parameters();
    param->set_key("cpu.shares");
    param->set_value(cpu);
    param = c.add_parameters();
    param->set_key("memory.limit_in_bytes");
    param->set_value(memory);

    ::sailor::serializable::Protocol protocol;
    protocol.set_type(::sailor::serializable::ProtocolStart);
    protocol.set_buffer(t.SerializeAsString());
    protocol.set_instance("DEPLOY");

    if (!_worker->append_outgoing(protocol.SerializeAsString())) {
        return false;
    }

    LOG.info("NoahExecutor: DEPLOY initiated: %s", uuid.c_str());
    return true;
}

bool NoahExecutor::process_order_remove(const InstanceInfo &info, uint64_t generation)
{
    std::string instance_name = info.get_instance_name();

    std::ostringstream conf_file_buffer;
    conf_file_buffer << g_config->MATRIX_TMP_PATH << '/' << instance_name << '_' << generation;
    std::string conf_file = conf_file_buffer.str();

    if (!save_configuration(info, conf_file)) {
        return false;
    }

    std::string cmd = prepare_cmd(info, conf_file, "REMOVE");
    std::string uuid;
    make_uuid(generation, &uuid);

    ::sailor::serializable::TaskInfo t;
    t.set_uuid(uuid);
    t.set_entry_point(cmd);
    t.set_privileged(true);
    t.set_emulator(::sailor::serializable::EmulatorSsh);

    ::sailor::serializable::Cgroups &c = *t.mutable_cgroups();
    c.set_name(instance_name);
    c.set_auto_release(true);

    ::sailor::serializable::Protocol protocol;
    protocol.set_type(::sailor::serializable::ProtocolStart);
    protocol.set_buffer(t.SerializeAsString());
    protocol.set_instance("REMOVE");

    if (!_worker->append_outgoing(protocol.SerializeAsString())) {
        return false;
    }

    LOG.info("NoahExecutor: REMOVE initiated: %s", uuid.c_str());
    return true;
}

bool NoahExecutor::process_order_status(const InstanceInfo &info, uint64_t generation)
{
    std::string instance_name = info.get_instance_name();

    std::ostringstream conf_file_buffer;
    conf_file_buffer << g_config->MATRIX_TMP_PATH << '/' << instance_name << '_' << generation;
    std::string conf_file = conf_file_buffer.str();

    if (!save_configuration(info, conf_file)) {
        return false;
    }

    std::string cmd = prepare_cmd(info, conf_file, "STATUS");

    std::string uuid;
    make_uuid(generation, &uuid);

    ::sailor::serializable::TaskInfo t;
    t.set_uuid(uuid);
    t.set_entry_point(cmd);
    t.set_privileged(true);
    t.set_emulator(::sailor::serializable::EmulatorSsh);

    ::sailor::serializable::Cgroups &c = *t.mutable_cgroups();
    c.set_name(instance_name);
    c.set_auto_release(false);

    ::sailor::serializable::Protocol protocol;
    protocol.set_type(::sailor::serializable::ProtocolStart);
    protocol.set_buffer(t.SerializeAsString());
    protocol.set_instance("STATUS");

    if (!_worker->append_outgoing(protocol.SerializeAsString())) {
        return false;
    }

    LOG.info("NoahExecutor: STATUS initiated: %s", uuid.c_str());
    return true;
}

bool NoahExecutor::process_order_stop(uint64_t generation)
{
    std::string uuid;
    make_uuid(generation, &uuid);

    ::sailor::serializable::Protocol protocol;
    protocol.set_type(::sailor::serializable::ProtocolStop);
    protocol.set_param(::sailor::serializable::NiceSoft);
    protocol.set_uuid(uuid);

    if (!_worker->append_outgoing(protocol.SerializeAsString())) {
        return false;
    }

    LOG.info("NoahExecutor: STOP initiated: %s", uuid.c_str());
    return true;
}

bool NoahExecutor::process_order(Order* order)
{
    switch (order->_type) {
    case Order::TypeInstall:
        return process_order_install(*order->_info, order->_generation);

    case Order::TypeUpdate:
        return process_order_update(*order->_info, order->_generation);

    case Order::TypeRemove:
        return process_order_remove(*order->_info, order->_generation);

    case Order::TypeStop:
        return process_order_stop(order->_generation);

    case Order::TypeStatus:
        return process_order_status(*order->_info, order->_generation);

    default:
        LOG.error("NoahExecutor: unknown order [%d], must be a bug.", order->_type);
        return false;
    };
}

bool NoahExecutor::process_orders()
{
    MutexLocker locker(&_mutex);

    while (!_orders.empty()) {
        Order *order = _orders.front();
        _orders.pop_front();

        // If we return false, FileEvent is down.
        // What can we do if it fails?
        if (!process_order(order)) {
            LOG.warn("NoahExecutor: failed to process order for %lu.",
                     order->_generation);
        }

        delete order;
    }

    return true;
}

bool NoahExecutor::process_message(const std::string& message)
{
    ::sailor::serializable::Protocol protocol;
    if (!protocol.ParseFromString(message)) {
        return false;
    }


    switch (protocol.type()) {
    case ::sailor::serializable::ProtocolNone:
        if (!protocol.has_buffer()) {
            return false;
        }

        LOG.info("NoahExecutor: HANDSHAKE [%s]", protocol.buffer().c_str());
        break;

    case ::sailor::serializable::ProtocolStatus:
        uint64_t generation;
        if (!parse_uuid(protocol.uuid(), &generation)) {
            return false;
        }

        if (protocol.has_param()) {
            LOG.info("NoahExecutor: [%s] [%d]: %lu",
                     protocol.instance().c_str(),
                     protocol.param(),
                     generation);

            if (protocol.param() == 172 << 8) {
                LOG.warn("NoahExecutor: ignore 172 with %s. generation: %lu", 
                         protocol.instance().c_str(),
                         generation);
                break;
            }
            bool result = (protocol.param() == 0);

            if (protocol.instance().compare("DEPLOY") == 0) {
                _cb.on_deploy(generation, result);
            } else if (protocol.instance().compare("REMOVE") == 0) {
                _cb.on_remove(generation, result);
            } else if (protocol.instance().compare("STATUS") == 0) {
                InstanceInfo::HealthCheckStatus::type status = InstanceInfo::HealthCheckStatus::ERROR;
                switch (protocol.param()) {
                case 0:
                    status = InstanceInfo::HealthCheckStatus::RUNNING;
                    break;
                case 256:
                    status = InstanceInfo::HealthCheckStatus::STARTING;
                    break;
                default:
                    status = InstanceInfo::HealthCheckStatus::ERROR;
                    break;
                }
                _cb.on_status(generation, status);
            } else {
                LOG.warn("NoahExecutor: UNKNOWN INSTANCE [%s]", protocol.instance().c_str());
            }
            break;

        } else {
            LOG.trace("NoahExecutor: [%s] [ALIVE]: %lu",
                      protocol.instance().c_str(), generation);
        }
        break;

    case ::sailor::serializable::ProtocolStdOut:
        LOG.trace("NoahExecutor: STDOUT[%lu]", protocol.buffer().length());
        LOG.trace("NoahExecutor: STDOUT: %s", protocol.buffer().c_str());
        break;

    case ::sailor::serializable::ProtocolStdErr:
        LOG.trace("NoahExecutor: STDERR[%lu]", protocol.buffer().length());
        break;

    case ::sailor::serializable::ProtocolReject:
        // TODO(zhongyiyuan): log more detail for better (debug) view.
        LOG.info("NoahExecutor: REJECT [%d], ignore.", protocol.param());
        break;

    default:
        LOG.warn("NoahExecutor: UNKNOWN PROTOCOL [%d]", protocol.type());
        break;

    };

    // Don't return false so easily since it will close the link down.
    return true;
}

void NoahExecutor::process_errors(bool critical)
{
    LOG.error("Executor: error occurred: %s", critical ? "CRITICAL" : "NORMAL");
}

void NoahExecutor::notify_order_processer(Order* order)
{
    MutexLocker locker(&_mutex);

    // instance info should always be valid.
    // check here just for safety.
    if (order->_info != NULL && !order->_info->is_valid()) {
        const char* err_msg = "Order's instance info is not valid";
        LOG.warn(err_msg);
        throw std::runtime_error(err_msg);
    }

    _orders.push_back(order);

    if (!_notice->wake()) {
        _orders.pop_back();
        delete order;
        LOG.error("NoahExecutor: notify order processer failed.");
        // Notify processer use pipe failed. 
        // Restart agent for retry.
        exit(EXIT_FAILURE);
    }

}

void NoahExecutor::update(const InstanceInfo& info, uint64_t generation)
{
    LOG.trace("NoahExecutor: call update, generation: %lu", generation);
    Order* order = new Order(Order::TypeUpdate, &info, generation);
    notify_order_processer(order);
}

void NoahExecutor::install(const InstanceInfo& info, uint64_t generation)
{
    LOG.trace("NoahExecutor: call install, generation: %lu", generation);
    Order *order = new Order(Order::TypeInstall, &info, generation);
    notify_order_processer(order);
}

void NoahExecutor::remove(const InstanceInfo& info, uint64_t generation)
{
    LOG.trace("NoahExecutor: call remove, generation: %lu", generation);
    Order *order = new Order(Order::TypeRemove, &info, generation);
    notify_order_processer(order);
}

void NoahExecutor::status(const InstanceInfo &info, uint64_t generation)
{
    LOG.trace("NoahExecutor: call status, generation: %lu", generation);
    Order* order = new Order(Order::TypeStatus, &info, generation);
    notify_order_processer(order);
}

void NoahExecutor::stop(uint64_t generation)
{
    LOG.trace("NoahExecutor: call stop, generation: %lu", generation);
    Order *order = new Order(Order::TypeStop, NULL, generation);
    notify_order_processer(order);
}

void NoahExecutor::make_uuid(uint64_t generation, std::string *uuid)
{
    assert(uuid);

    char buffer[40];
    uint64_t high = generation >> 48;
    uint64_t low = generation & 0x0000FFFFFFFFFFFF;
    int ret = snprintf(buffer, sizeof(buffer), "0055aa00-0000-0002-%04lx-%012lx", high, low);
    if (ret < 0 || static_cast<size_t>(ret) >= sizeof(buffer)) {
        // Oops, is it possible?
        throw std::runtime_error("internal error");
    }

    uuid->assign(buffer);
}

bool NoahExecutor::parse_uuid(const std::string& uuid, uint64_t* generation)
{
    assert(generation);

    uint64_t high;
    uint64_t low;
    int ret = sscanf(uuid.c_str(), "%*08x-%*04x-%*04x-%04lx-%012lx", &high, &low);
    if (ret != 2) {
        return false;
    }

    *generation = high << 48 | low;
    return true;
}

}; // namespace matrix
