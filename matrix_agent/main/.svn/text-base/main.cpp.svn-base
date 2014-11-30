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

#ifdef __unix__
#include <sys/stat.h>
#include <grp.h>
#include <unistd.h>
#endif // __unix__

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <sstream>
#include <string>

#include "inf/computing/matrix/matrix-agent/Agent.h"
#include "inf/computing/matrix/matrix-agent/configure.h"
#include "inf/computing/matrix/matrix-agent/common/Utility.h"

#include "op/oped/noah/sailor/utility/linkage/Interface.h"
#include "op/oped/noah/sailor/utility/Logger.h"
#include "op/oped/noah/sailor/utility/babysitter.h"
#include "op/oped/noah/sailor/utility/cmdline.h"
#include "op/oped/noah/sailor/utility/common.h"
#include "op/oped/noah/sailor/utility/daemon.h"
#include "op/oped/noah/sailor/utility/mkdirs.h"
#include "op/oped/noah/sailor/utility/msleep.h"
#include "op/oped/noah/sailor/utility/revision.h"
#include "op/oped/noah/sailor/utility/system.h"
#include "op/oped/noah/sailor/utility/utility.h"

using matrix::Agent;
using sailor::Interface;
using sailor::Logger;

extern char **environ;

/// Global Agent pointer.
static Agent *g_agent = NULL;
static bool g_restart = false;

/// Magic number indicating we want to be restarted.
static const int RESTART_MAGIC_NUMBER = 0xAA;

/// Print help.
static void print_help()
{
    printf("Usage: %s [OPTION]...\n", cmdline_arg_name());
    printf("\n");
    printf("  -h, --help    display this help and exit\n");
    printf("  -c, --console run the daemon in the foreground\n");
    printf("  -v, --version output version information and exit\n");
    printf("\n");
    printf("Without any OPTION, start the daemon normally.\n");
}

/// Print version.
static void print_version()
{
    printf("Project Matrix    : Agent\n\n");

    print_revision();
}

/// Check if Sailor is on sail.
static bool is_running()
{
    Interface i;
    int ret = i.connect_tcp("localhost", g_config->AGENT_CONTROL_PORT);
    if (ret < 0) {
        return false;
    } else if (ret == 0) { // Not likely.
        return true;
    }

    return i.wait_until_connected(1000);
    return true;
}

/// Quiting signals handler.
static void signal_handler(int /*sig*/)
{
    g_agent->shutdown();
}

static void restart_handler(int /*sig*/)
{
    g_agent->shutdown();
    g_restart = true;
}

/// Babysitter callback.
static int babysitter(int times, int retcode)
{
#ifdef __unix__
    if (WIFEXITED(retcode) && WEXITSTATUS(retcode) == RESTART_MAGIC_NUMBER) {
        LOG.info("Process terminated expectedly, happened %d time(s), restart...", times);

        if (system_timed(cmdline_module_path(), 5000) == 0) {
            for (int i = 0; i < 50; ++i) {
                msleep(100);
                if (is_running()) {
                    Logger::process_detach();
                    return -1; // Very well.
                }
            }
        }

        // Oops, restart with in memory binary.
        LOG.error("Process failed to load new binary, happened %d time(s).", times);

        return 0;
    }
#endif // __unix__

    LOG.error("Process terminated unexpectedly, happened %d time(s), restarting...", times);
    msleep(5000);

    // Check again to prevent someone started another instance.
    if (is_running()) {
        Logger::process_detach();
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    bool console = false;
    int ret;

    // Deal with early signals, won't touch argv[] and environ[].
    signal(SIGHUP,  SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    argv = cmdline_setup(argc, argv);
    if (!argv) {
        fprintf(stderr, "Failed to initialize environment variables table.\n");
        return EXIT_FAILURE;
    }

    static const char *OPTSTRING = "vch";
    static const struct option LONGOPTS[] = {
        {"version", no_argument,    NULL, 'v'},
        {"console", no_argument,    NULL, 'c'},
        {   "help", no_argument,    NULL, 'h'},
        {     NULL,           0,    NULL,  0 },
    };

    while (true) {
        int longindex = 0;
        int opt = getopt_long(argc, argv, OPTSTRING, LONGOPTS, &longindex);
        if (opt < 0) {
            break;
        }

        switch (opt) {
        case 'v':
            print_version();
            return EXIT_SUCCESS;

        case 'h':
            print_help();
            return EXIT_SUCCESS;

        case 'c':
            console = true;
            break;

        case '?':
        default:
            printf("Try `%s --help' for more information.\n", cmdline_arg_name());
            return EXIT_FAILURE;
        }
    }

#ifdef __unix__
    const gid_t root_primary_gid = 0;
    if (setuid(0) || setgid(0) || setgroups(1, &root_primary_gid)) {
#ifdef NDEBUG
        fprintf(stderr, "Root privilege is required to launch this program.\n");
        return EXIT_FAILURE;
#endif // NDEBUG
    }
    umask(S_IWGRP | S_IWOTH); // 0022
#endif // __unix__

    Logger::set_attached_only(true);
    if (load_global_configure()) {
        fprintf(stderr, "Failed to load configure file.\n");
        return EXIT_FAILURE;
    }

    if (is_running()) {
        printf("Daemon is already running.\n");
        return EXIT_SUCCESS;
    }

    if (mkdirs(g_config->LOG_PATH, S_IRWXU | S_IRWXG | S_IRWXO)) { // 0755
        fprintf(stderr, "Failed to create logging directory.\n");
        // Don't quit here even if we encountered an error.
    }


    if (mkdirs(g_config->META_PATH, S_IRWXU | S_IRWXG | S_IRWXO)) { // 0755
        fprintf(stderr, "Failed to create meta directory.\n");
        // Don't quit here even if we encountered an error.
    }

    if (mkdirs(g_config->MATRIX_CONTAINERS_PATH, S_IRWXU | S_IRWXG | S_IRWXO)) { // 0755
        fprintf(stderr, "Failed to create containers directory.\n");
        return EXIT_FAILURE;
    }
    if (mkdirs(g_config->MATRIX_TMP_PATH, S_IRWXU | S_IRWXG | S_IRWXO)) { // 0755
        fprintf(stderr, "Failed to create tmp directory.\n");
        return EXIT_FAILURE;
    }
    if (mkdirs(g_config->MATRIX_DEPLOY_LOG_PATH, S_IRWXU | S_IRWXG | S_IRWXO)) { // 0755
        fprintf(stderr, "Failed to create deploy_log directory.\n");
        return EXIT_FAILURE;
    }
    if (mkdirs("/home/coresave", S_IRWXU | S_IRWXG | S_IRWXO)) { // 0755
        fprintf(stderr, "Failed to create /home/coresave directory.\n");
        return EXIT_FAILURE;
    }

#ifdef __unix__
    // Force reset attributes like g+s.
    if (chmod(g_config->LOG_PATH, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)  || // 0755
        chmod(g_config->META_PATH, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) ||
        chmod("/home/coresave", S_ISVTX | S_IRWXU | S_IRWXG | S_IRWXO)              || // 1777
        chown(g_config->META_PATH, 0, 0)                                            ||
        chown(g_config->LOG_PATH, 0, 0)                                             ){

        fprintf(stderr, "Incorrect permission for logs, still start...\n");
    }

    uid_t uid;
    gid_t gid;
    if (!matrix::get_uid_gid_of_specified_user("work", &uid, &gid)) {
        fprintf(stderr, "No user work found");
        return EXIT_FAILURE;
    }
    if (chown(g_config->MATRIX_CONTAINERS_PATH,   0,   0) || 
        chown(g_config->MATRIX_TMP_PATH,        uid, gid) ||
        chown(g_config->MATRIX_DEPLOY_LOG_PATH, uid, gid) ){
        
        fprintf(stderr, "Incorrect permission for MATRIX_ROOT/containers or MATRIX_ROOT/tmp\n");
        return EXIT_FAILURE;
    }
#endif // __unix__

#ifdef __unix__
    // check adapter_scripts exist
    if (access(g_config->ADAPTER_SCRIPTS_PATH, F_OK)) {
        fprintf(stderr, "No adapter_scripts found.\n");
        return EXIT_FAILURE;
    }
#endif // __unix__

    if (!console) {
        ret = daemon_spawn(0, 0);
        if (ret < 0) {
            fprintf(stderr, "Failed to initialize.\n");
            return EXIT_FAILURE;

        } else if (ret > 0) {
            for (int i = 0; i < 50; ++i) {
                msleep(100);
                if (is_running()) {
                    printf("Daemon is now running.\n");
                    return EXIT_SUCCESS;
                }
            }

            fprintf(stderr, "Daemon failed to run.\n");
            return EXIT_FAILURE;
        }

#ifdef __unix__
        // Daemonizer will restore umask to 0077.
        umask(S_IWGRP | S_IWOTH); // 0022
#endif // __unix__
    }

    char version[128];
    if (get_project_version(version, sizeof(version))) {
        strcpy(version, "N/A");
    }

    std::stringstream s;
    s << g_config->LOG_PATH << '/' << g_config->LOG_FILE;

    // Logger might fail, but we keep going.
    bool colorful = g_config->LOG_COLOR ? true : false;
    Logger::set_colorful(colorful);
    Logger::process_attach(s.str().c_str(),
                           static_cast<Logger::LogLevel>(g_config->LOG_LEVEL),
                           "%L\t%T\t%A %R",
                           Logger::Truncate,
                           g_config->LOG_SIZE);

    if (!console) {
        cmdline_set_process_name("%s [%s]", g_config->PROCESS_BABYSITTER_NAME, version);

        // No delay between each restart which we'll do it in callback.
        ret = install_babysitter(BS_AUTO_RESTART, 0, 0, babysitter);
        if (ret < 0) {
            LOG.error("Babysitter failed to initialize.");
            Logger::process_detach();
            return EXIT_FAILURE;
        }
        assert(ret == 0);
    }

    LOG.info("%s activating...", g_config->PROCESS_CENTRAL_NAME);
    g_agent = new Agent();

    signal(SIGHUP,  restart_handler);
    signal(SIGINT,  signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    bool result = false;
    do {
        ret = g_agent->initialize();
        if (ret < 0) {
            LOG.error("Agent failed to initialize.");
            break;

        } else if (ret > 0) {
            LOG.warn("Agent failed to initialize, it's trivial so I just quit.");
            result = true;
            break;
        }

        cmdline_set_process_name("%s [%s]", g_config->PROCESS_CENTRAL_NAME, version);

        if (!g_agent->run()) {
            LOG.error("Agent failed to run.");
            break;
        }

        result = true;

    } while (false);

    signal(SIGHUP,  SIG_IGN);
    signal(SIGINT,  SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);

    bool request_restart = g_agent->request_restart() || g_restart;
    delete g_agent;
    g_agent = NULL;

    if (result) {
        LOG.info("%s [%s] quit successfully.", g_config->PROCESS_CENTRAL_NAME, version);
    } else {
        LOG.error("%s [%s] quit with failures.", g_config->PROCESS_CENTRAL_NAME, version);
    }

    if (request_restart) {
        LOG.info("%s has requested a restart, exit with magic number...",
                 g_config->PROCESS_CENTRAL_NAME);
    }

    Logger::process_detach();

    if (!result) {
        return EXIT_FAILURE;
    }

    return request_restart ? RESTART_MAGIC_NUMBER : EXIT_SUCCESS;
}
