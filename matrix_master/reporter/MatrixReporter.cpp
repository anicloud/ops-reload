#include "op/oped/noah/sailor/utility/Logger.h"
#include "inf/computing/matrix/matrix-agent/json-cpp/include/json/json.h"

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/rpc/MatrixRpc.h"
#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/httpd/MatrixHttpd.h"

#include "inf/computing/matrix/matrix-agent/reporter/MatrixReporter.h"

namespace matrix {


MatrixReporter::MatrixReporter(MatrixManager& manager, 
                               MatrixMasterRpc& rpc,
                               MatrixHttpd& httpd) :
    MatrixThread("MatrixReporter", g_config->DEFAULT_HEARTBEAT_INTERVAL * 1000),
    _manager(manager),
    _rpc(rpc),
    _httpd(httpd),
    _init_heartbeat(true)
{
}

MatrixReporter::~MatrixReporter()
{
}

void MatrixReporter::dump_hb_msg(const HeartbeatMessage& msg) const
{
    LOG.trace("-----------------------------dump_hb_msg begin--------------------------");
    LOG.trace("%s", msg.to_json().toStyledString().c_str());
    LOG.trace("-----------------------------dump_hb_msg end--------------------------");
}

void MatrixReporter::dump_hb_res(const HeartbeatResponse& res) const
{
    LOG.trace("-----------------------------dump_hb_res begin--------------------------");
    LOG.trace("%s", res.to_json().toStyledString().c_str());
    LOG.trace("-----------------------------dump_hb_res begin--------------------------");
}

void MatrixReporter::run()
{
    LOG.info("MatrixReporter: begin heartbeat to master.");

    std::vector<InstanceInfo> instances;
    HostInfo host_info;

    // get all instances from manager.
    // always return true.
    if(!_manager.get_all_instance_info(&instances)) {
        // should not reach here.
        return;
    }

    // get host info.
    if(!_manager.get_host_info(&host_info)) {
        LOG.info("MatrixReporter: manager is not ready, try later.");
        return;
    }

    HeartbeatMessage msg(host_info, instances, _init_heartbeat);
    _init_heartbeat = false;

    /* dump msg*/ 
    dump_hb_msg(msg);
    
    try {

        HeartbeatResponse res = _rpc.heartbeat(msg);

        dump_hb_res(res);

        // validate heartbeat response.
        if(!res.is_valid()) {
            LOG.warn("MatrixReporter: rpc response is invalid!");
            return;
        }

        if(res._agent_conf._master_safe_mode) { // skip if master is in safe mode.
            LOG.info("MatrixReporter: master in safe mode, ignore expect instance list and agent conf");
        } else {

            LOG.trace("MatrixReporter: notify manager about heartbeat response.");
            _manager.push_instance_expect(res._expect_instances);
            _manager.set_reserved_resource(res._agent_conf._reserved_resource);

            if(res._agent_conf._heartbeat_interval_sec * 1000 != (int32_t)get_interval()) {
                LOG.info("MatrixReporter: reset heartbeat interval from %d ms to %d ms by master order", 
                         get_interval(), res._agent_conf._heartbeat_interval_sec * 1000);
                set_interval(res._agent_conf._heartbeat_interval_sec * 1000); /* ms */
            }

            // if httpd port change, restart httpd.
            int32_t listen_port = _httpd.get_listen_port();
            if(listen_port != res._agent_conf._agent_http_port) {

                LOG.info("MatrixReporter: reset Httpd listen port from %d to %d by master order", 
                         listen_port, res._agent_conf._agent_http_port);
                listen_port = res._agent_conf._agent_http_port;

                _httpd.stop();
                if(_httpd.start(listen_port)) {
                    LOG.info("MatrixReporter: Httpd started with port %d OK.", 
                             listen_port); 
                } else {
                    LOG.warn("MatrixReporter: Httpd started with port %d FAILED!.", 
                             listen_port);
                }
            }
        }
        LOG.info("MatrixReporter: finish heartbeat to master.");
    } catch (MatrixRpcException& ex) {
        LOG.warn("MatrixReporter: heartbeat failed %s", ex.what());
    }
}

}
