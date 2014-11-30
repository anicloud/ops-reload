#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <getopt.h>

#include <cstdio>
#include <string>
#include <fstream>
#include <cstdlib>
#include <cerrno>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "op/oped/noah/sailor/utility/thread/Mutex.h"
#include "op/oped/noah/sailor/utility/thread/MutexLocker.h"

#include "inf/computing/matrix/matrix-agent/rpc/MatrixThriftRpc.h"
#include "inf/computing/matrix/matrix-agent/common/MatrixThread.h"

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace matrix;
using namespace std;

int32_t g_hb_count = -1;
std::string g_input_json("");
std::string g_output_dir("./out");
uint16_t g_master_port = 26726;

time_t g_mtime;

ThriftHeartbeatResponse g_res;

::sailor::Mutex _mutex;

static void to_thrift(const HeartbeatResponse& res,
                      ThriftHeartbeatResponse& t_res)
{
    t_res.agentConf.agentHttpPort = res._agent_conf._agent_http_port;
    t_res.agentConf.heartbeatIntervalSec = res._agent_conf._heartbeat_interval_sec;
    t_res.agentConf.safeMode = res._agent_conf._master_safe_mode;
    t_res.agentConf.reservedResource.cpuNum = 
        res._agent_conf._reserved_resource._cpu_num;
    t_res.agentConf.reservedResource.memoryMB = 
        res._agent_conf._reserved_resource._memory_mb;
    t_res.agentConf.reservedResource.diskTotalMB = 
        res._agent_conf._reserved_resource._disk_total_mb;

    t_res.expectInstance.clear();
    for (std::vector<InstanceInfo>::const_iterator itr = res._expect_instances.begin();
         itr != res._expect_instances.end(); ++itr) {
        ThriftInstanceInfo t_instance;
        MatrixMasterThriftRpc::to_thrift(*itr, t_instance);
        t_res.expectInstance.push_back(t_instance);
    }
}

static HeartbeatMessage to_matrix(const ThriftHeartbeatMessage& t_msg)
{
    HostInfo host_info;
    host_info.set_host_state((HostInfo::HostState::type)t_msg.hostInfo.hostState);
    host_info.set_total_resource(
              MatrixMasterThriftRpc::to_matrix(t_msg.hostInfo.totalResource));
    std::vector<InstanceInfo> instances;
    for (std::vector<ThriftInstanceInfo>::const_iterator itr = t_msg.instanceInfo.begin();
         itr != t_msg.instanceInfo.end(); ++itr) {
        instances.push_back(MatrixMasterThriftRpc::to_matrix(*itr));
    }
    return HeartbeatMessage(host_info, instances, t_msg.firstHeartbeat);
}

bool load_res(ThriftHeartbeatResponse& res)
{
    std::string content;

    try {
        std::ifstream ifs(g_input_json.c_str());
        while(ifs) {
            std::string line;
            ifs >> line;
            content += (line + "\n");
        }
        ifs.close();
    } catch (const std::exception & e) {
        LOG.error("error while read file %s : %s", g_input_json.c_str(), e.what());
        return false;
    }

    Json::Value v;
    Json::Reader reader;
    if(!reader.parse(content, v)) {
        LOG.error("error while parse file %s", g_input_json.c_str());
        return false;
    }

    try {
        HeartbeatResponse res_tmp = HeartbeatResponse::from_json(v);
        to_thrift(res_tmp, res);
    } catch (MatrixJsonException& ex) {
        LOG.error("error while load file %s", g_input_json.c_str());
        return false;
    }

    return true;
}

class MasterAgentProtocolHandler : virtual public MasterAgentProtocolIf {
public:
    MasterAgentProtocolHandler()
    {
        // Your initialization goes here
    }

    void heartbeat(ThriftHeartbeatResponse& _return, const ThriftHeartbeatMessage& msg)
    {
        ::sailor::MutexLocker locker(&_mutex);
        // save msg
        HeartbeatMessage matrix_msg = to_matrix(msg);
        std::string out;
        Json::Value v = matrix_msg.to_json();
        Json::StyledWriter writer;
        out = writer.write(v);
        cout << "============================================================";
        cout << out << endl;
        cout << "============================================================";

        // set response
        _return = g_res;

        if(g_hb_count > 0) {
            g_hb_count --;
        }

        if(g_hb_count == 0) {
            LOG.info("HB count reach, quit");
            exit(0);
            return;
        }
    }

};

static const char *OPTSTRING = "c:i:o:p:h";
static const struct option LONGOPTS[] = {
    { "hb-count", required_argument,    NULL, 'c'},
    {    "input", required_argument,    NULL, 'i'},
    {   "output", required_argument,    NULL, 'o'},
    {     "port", required_argument,    NULL, 'p'},
    {     "help",       no_argument,    NULL, 'h'},
    {       NULL,                 0,    NULL,  0 },
};

class MockWatch : public MatrixThread
{
public:
    MockWatch()
    {
        set_name("MockWatch");
        set_interval(1000);
    }
    virtual ~MockWatch()
    {

    }

    void run()
    {
        ::sailor::MutexLocker locker(&_mutex);
        struct stat stat_buf;

        if(stat(g_input_json.c_str(), &stat_buf) < 0) {
            LOG.error("error while get file mtime %s : %d", g_input_json.c_str(), errno);
        } else {
            if(g_mtime != stat_buf.st_mtime) {
                LOG.info("%s modified, reload", g_input_json.c_str());
                load_res(g_res);
            }
            g_mtime = stat_buf.st_mtime;
        }

    }

};

static void usage(const char * prog)
{
    fprintf(stderr, "usage: test_MockMaster options\n");
    fprintf(stderr, "--hb-count|-c [heart beat count], default -1 (always loop)\n");
    fprintf(stderr, "--input   |-i [heart beat response json file]\n");
    fprintf(stderr, "--output  |-o [heart beat message json file output dir], default ./out\n");
    fprintf(stderr, "--port    |-p [master port] : listen port, default 26726\n");
    fprintf(stderr, "--help    |-h print this screen\n");
}

int main(int argc, char *argv[])
{

    while (true) {
        int longindex = 0;
        int opt = getopt_long(argc, argv, OPTSTRING, LONGOPTS, &longindex);
        if (opt < 0) {
            break;
        }

        switch (opt) {
        case 'c':
            g_hb_count = atoi(optarg);
            break;

        case 'i':
            g_input_json = optarg;
            break;

        case 'o':
            g_output_dir = optarg;
            break;

        case 'p':
            g_master_port = atoi(optarg);
            break;

        case 'h':
            usage(argv[0]);
            exit(0);
            break;

        default:
            usage(argv[0]);
            exit(1);
        }
    }

    if(g_input_json.size() == 0) {
        usage(argv[0]);
        exit(0);
    }


    if(!load_res(g_res)) {
        exit(1);
    }

    struct stat stat_buf;

    if(stat(g_input_json.c_str(), &stat_buf) < 0) {
        LOG.error("error while get file mtime %s : %d", g_input_json.c_str(), errno);
        exit(1);
    }

    g_mtime = stat_buf.st_mtime;

    mkdir(g_output_dir.c_str(), 0755);

    MockWatch w;

    w.start();

    boost::shared_ptr<MasterAgentProtocolHandler> handler(new MasterAgentProtocolHandler());
    boost::shared_ptr<TProcessor> processor(new MasterAgentProtocolProcessor(handler));
    boost::shared_ptr<TServerTransport> serverTransport(new TServerSocket(g_master_port));
    boost::shared_ptr<TTransportFactory> transportFactory(new TFramedTransportFactory());
    boost::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    server.serve();

    return 0;
}
