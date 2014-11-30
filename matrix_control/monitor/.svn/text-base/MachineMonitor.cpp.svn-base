#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <cerrno>
#include <cassert>
#include <string>
#include <cstring>
#include <cstdlib>


#include "op/oped/noah/sailor/utility/Logger.h"
#include "op/oped/noah/sailor/utility/safeio.h"

#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/manager/MatrixManager.h"
#include "inf/computing/matrix/matrix-agent/manager/HostInfo.h"
#include "inf/computing/matrix/matrix-agent/monitor/MachineMonitor.h"

#define DISK_IO_TIMEOUT_MS 5000

namespace matrix {

MachineMonitor::MachineMonitor(MatrixManager& manager) : 
        MatrixThread("MachineMonitor", 
                     g_config->DEFAULT_MACHINE_MONITOR_INTERVAL * 1000,
                     g_config->DEFAULT_MACHINE_MONITOR_INTERVAL * 1000 * 5), 
        _manager(manager),
        _matrix_root(g_config->MATRIX_ROOT)
{
    LOG.trace("MachineMonitor is constructed.");

    assert(!_matrix_root.empty());
}

MachineMonitor::~MachineMonitor()
{
    stop();
    LOG.trace("MachineMonitor is destroyed.");
}

bool MachineMonitor::monitor_disk()
{
    LOG.trace("%s: start to monitor machine disks.", _name.c_str());

    std::string monitor_file = std::string(_matrix_root) + "/" + "monitor.tmp";

    int fd = open(monitor_file.c_str(), O_CREAT | O_NONBLOCK | O_TRUNC | O_WRONLY, S_IRWXU);
    if (fd == -1) {
        LOG.warn("%s: create monitor tmp file failed with errno [%d]", 
                  _name.c_str(),
                  errno);
        return false;
    }

    const char dump[] = "matrix will control the world!";
    if(safe_timed_write_all(fd, dump, sizeof(dump) - 1, DISK_IO_TIMEOUT_MS) != sizeof(dump) - 1) {
        LOG.warn("%s: write to monitor tmp file failed with errno [%d]", 
                 _name.c_str(),
                 errno);
        safe_close(fd);
        return false;
    }

    if (safe_close(fd) != 0) {
        LOG.warn("%s: close monitor tmp file failed with errno [%d]", 
                 _name.c_str(),
                 errno);
        return false;
    }

    fd = open(monitor_file.c_str(), O_NONBLOCK | O_RDONLY);
    if (fd == -1) {
        LOG.warn("%s: open monitor tmp file failed with errno [%d]", 
                  _name.c_str(),
                  errno);
        return false;
    }

    char buffer[sizeof(dump)];
    if(safe_timed_read_all(fd, buffer, sizeof(buffer), DISK_IO_TIMEOUT_MS) != sizeof(dump) - 1) {
        LOG.warn("%s: read from monitor tmp file failed with errno [%d]", 
                 _name.c_str(),
                 errno);
        safe_close(fd);
        return false;
    }

    if (safe_close(fd) != 0) {
        LOG.warn("%s: close monitor tmp file failed with errno [%d]", 
                 _name.c_str(),
                 errno);
        return false;
    }


    buffer[sizeof(buffer) -1 ] = '\0';
    if (strcmp(buffer, dump) != 0) {
        LOG.warn("%s: monitor tmp file content does not match.", 
                 _name.c_str());
        return false;
    }

    return true;
}

void MachineMonitor::on_timeout()
{
    LOG.error("%s: thread is blocked!!!", _name.c_str());
    _manager.report_machine_status(HostInfo::HostState::FAILED);
    die();
}

void MachineMonitor::die()
{
    // stop ourself, and do not let babysitter restart us.
    exit(EXIT_SUCCESS);
}

void MachineMonitor::run()
{
    LOG.info("%s: start to monitor machine status.", _name.c_str());

    HostInfo::HostState::type machine_state = HostInfo::HostState::AVAILABLE;

    // check machine resource.
    LOG.trace("%s: start to monitor machine resource.", _name.c_str());
    ResourceTuple resource(get_cpu_num(), get_total_memory(), get_total_disk());
    if (resource._cpu_num <= 0L || resource._memory_mb <= 0L || resource._disk_total_mb <= 0L)
    {
        LOG.error("%s: get machine resource error.", _name.c_str());
        machine_state = HostInfo::HostState::FAILED;
    }
    LOG.info("%s: machine resource: %s", _name.c_str(),
                                         resource.to_json().toStyledString().c_str());

    if (!monitor_disk()) {
        LOG.error("%s: monitor disk error.", _name.c_str());
        machine_state = HostInfo::HostState::FAILED;
    }

    LOG.info("%s: machine status: %s", _name.c_str(),
                                       HostInfo::HostState::to_string(machine_state).c_str());
    
    _manager.report_machine_status(machine_state);
    _manager.report_machine_resource(resource);

    if (machine_state == HostInfo::HostState::FAILED) {
        die();
    }

    LOG.info("%s: finish to monitor machine.", _name.c_str());
}

int64_t MachineMonitor::get_cpu_num()
{
    // get available cpu num
    // 10 means 1 physical core.
    return (int64_t)(get_nprocs() * 10);
}

int64_t MachineMonitor::get_total_memory()
{
    // sysino(2)
    //
    // Since Linux 2.3.23 (i386), 2.3.48 (all architectures) the structure is:
    //     struct sysinfo {
    //         long uptime;                            /* Seconds since boot */
    //         unsigned long loads[3];                 /* 1, 5, and 15 minute load averages */
    //         unsigned long totalram;                 /* Total usable main memory size */
    //         unsigned long freeram;                  /* Available memory size */
    //         unsigned long sharedram;                /* Amount of shared memory */
    //         unsigned long bufferram;                /* Memory used by buffers */
    //         unsigned long totalswap;                /* Total swap space size */
    //         unsigned long freeswap;                 /* swap space still available */
    //         unsigned short procs;                   /* Number of current processes */
    //         unsigned long totalhigh;                /* Total high memory size */
    //         unsigned long freehigh;                 /* Available high memory size */
    //         unsigned int mem_unit;                  /* Memory unit size in bytes */
    //         char _f[20-2*sizeof(long)-sizeof(int)]; /* Padding for libc5 */
    //     };
    //
    // and the sizes are given as **multiples** of **mem_unit** bytes.

    struct sysinfo _sysinfo;
    if (sysinfo(&_sysinfo) != 0) {
        LOG.error("%s: get total memory error[%d]", _name.c_str(), errno);
        return -1;
    }
    // convert to mb
    return (int64_t)(_sysinfo.totalram * _sysinfo.mem_unit / 1024 / 1024);
}

int64_t MachineMonitor::get_total_disk()
{
    struct statvfs _vfsinfo;
    if (statvfs(_matrix_root.c_str(), &_vfsinfo) != 0) {
        LOG.error("%s: get total disk error[%d]", _name.c_str(), errno);
        return -1;
    }

    // convert to mb
    return (int64_t)(_vfsinfo.f_frsize * _vfsinfo.f_blocks / 1024 / 1024);
}

} /// namespace matrix
