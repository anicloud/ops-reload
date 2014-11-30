#include <getopt.h>
#include <netinet/in.h>

#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>

#include "inf/computing/matrix/matrix-agent/protocol/gen-cpp/AgentControlProtocol.h"

using namespace matrix;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

/// Print help.
static void print_help()
{
    printf("Usage: agent-control [agent address] [agent port] [OPTION]\n");
    printf("OPTIONS: \n");
    printf("  -h, --help                    display this help and exit\n");
    printf("  -f, --freeze<=on|off>         set agent freeze mode or leave freeze mode, or show current mode\n");
    printf("  -r, --reset  [instance id]    reset a instance to RUNNING (freeze mode ONLY), instance id = offset.service_name\n");
}


static std::string _command = "";
static std::string _instance_name = "";
static std::string _agent_addr = "";
static uint16_t _agent_port = 0;


int main(int argc, char *argv[])
{
    static const char *OPTSTRING = "hr:s:f::F::";
    static const struct option LONGOPTS[] = {
        {   "help",   0,    0, 'h'},
        {   "reset",  1,    0, 'r'},
        {   "reset",  1,    0, 's'},
        {   "freeze", 2,    0, 'F'},
        {   "freeze", 2,    0, 'f'},
        {   NULL,     0,    0,  0 },
    };

    while (true) {
        int longindex = 0;
        int opt = getopt_long(argc, argv, OPTSTRING, LONGOPTS, &longindex);
        if (opt < 0) {
            break;
        }

        switch (opt) {
        case 'h':
            print_help();
            return EXIT_SUCCESS;
            
        case 'r':
        case 's':
            _command = "reset_instance";
            _instance_name = optarg;
            break;

        case 'F':
        case 'f':
            if(NULL != optarg) {
                if(strcmp(optarg, "on") == 0 || strcmp(optarg, "ON") == 0 ) {
                    _command = "freeze_agent";
                } else if(strcmp(optarg, "off") == 0 || strcmp(optarg, "OFF") == 0 ){
                    _command = "unfreeze_agent";
                } else {
                    fprintf(stderr, "mode is ON|on|OFF|off");
                    return EXIT_FAILURE;
                }
            } else {
                _command = "show_freeze_agent";
            }
            break;

        case '?':
        default:
            fprintf(stderr, "Try `%s --help' for more information.\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if(_command == "") {
        print_help();
        return EXIT_FAILURE;
    }

    // get agent address
    if((argc - optind) != 2) {
        printf("argc %d optind %d\n", argc, optind);
        print_help();
        return EXIT_FAILURE;
    }

    _agent_addr = argv[optind];
    int val = atoi(argv[optind + 1]);
    if(val > 65535 || val <= 0) {
        fprintf(stderr, "port is invalid %d\n", val);
        return EXIT_FAILURE;
    }
    _agent_port = (uint16_t)val;

    boost::shared_ptr<TSocket> socket(new TSocket(_agent_addr, _agent_port));
    boost::shared_ptr<TTransport> transport(new TFramedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    AgentControlProtocolClient _client(protocol);
    try {
        transport->open();
    }
    catch (const apache::thrift::TException & e) {
        fprintf(stderr, "connect agent %s:%u fail\n", _agent_addr.c_str(), _agent_port);
        return EXIT_FAILURE;
    }
    
    fprintf(stderr, "connect agent %s:%u ok\n", _agent_addr.c_str(), _agent_port);

//  do commands
    do {
        if (_command == "reset_instance") {
            if (!_client.getFreeze()) {
                fprintf(stderr, "only in freeze mode can reset instance!\n");
                break;
            }
            
            if (!_client.resetInstance(_instance_name)) {
                fprintf(stderr, "reset_instance to RUNNING fail!\n");
                break;
            }
            fprintf(stderr, "reset_instance %s ok, it should in state RUNNING now.\n", _instance_name.c_str());
            break;
        }
        
        if (_command == "show_freeze_agent") {
            if (_client.getFreeze()) {
                fprintf(stderr, "freeze on\n");
            } else {
                fprintf(stderr, "freeze off\n");
            }
            break;
        }
        
        if (_command == "freeze_agent") {
            _client.setFreeze(true);
            if (_client.getFreeze()) {
                fprintf(stderr, "freeze ok\n");
            } else {
                fprintf(stderr, "freeze fail!\n");
            }
            break;
        }
        
        if (_command == "unfreeze_agent") {
            _client.setFreeze(false);
            
            if (!_client.getFreeze()) {
                fprintf(stderr, "unfreeze ok\n");
            } else {
                fprintf(stderr, "unfreeze fail!\n");
            }
            break;
        }
        
        fprintf(stderr, "unsupport command %s!\n", _command.c_str());

    } while(false);

    transport->close();
    return EXIT_SUCCESS;
}
