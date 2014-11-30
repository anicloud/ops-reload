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

#include "inf/computing/matrix/matrix-agent/configure.h"

#include <cstddef>
#include <string.h>
#include <iostream>

// I hate this.
#include <Configure.h>

#include "op/oped/noah/sailor/utility/cmdline.h"
#include "op/oped/noah/sailor/utility/utility.h"

/// Hard code here.
static const char *CONFIGURE_PATH = "../etc";

/// Hard code here.
static const char *CONFIGURE_FILE = "matrix.conf";

/// Expose a constant configuration object.
const struct Configure *g_config;

static struct Configure g_mutable_config;   ///< Internal mutable configuration object.
static comcfg::Configure g_conf;            ///< Global loader.

static std::string g_matrix_root;           ///< Absolute path.
static std::string g_matrix_containers_path;///< Absolute path.
static std::string g_matrix_tmp_path;       ///< Absolute path.
static std::string g_matrix_deploy_log_path;///< Absolute path.
static std::string g_log_path;              ///< Absolute path.
static std::string g_temp_path;             ///< Absolute path.
static std::string g_meta_path;             ///< Absolute path.
static std::string g_cgroup_root;           ///< Absolute path.
static std::string g_adapter_scripts_path;   ///< Absolute path.

static void load_path(bool loaded, const char *name, const char *def, std::string *result)
{
    int err = 0;
    const char *v = NULL;
    if (loaded) {
        v = g_conf[name].to_cstr(&err);
    }

    if (err || !v) {
        v = def;
    }

    char *path = cmdline_get_absolute_path(v, 0);
    if (!path) {
        *result = ".";
        return;
    }

    *result = path;
    free(path);
}

/// Can be called multiple times, but not reentrant.
int load_global_configure()
{
    g_config = NULL;
    memset(&g_mutable_config, 0, sizeof(g_mutable_config));

    bool loaded = false;
    char *path = cmdline_get_absolute_path(CONFIGURE_PATH, 0);
    if (path) {
        if (g_conf.load(path, CONFIGURE_FILE) == 0) {
            loaded = true;
        }
        free(path);
    }

#define GETs(name,def,null) \
        err = 0; \
        if (loaded) { \
            g_mutable_config.name = g_conf[#name].to_cstr(&err); \
        } \
        if (!loaded || err || (!null && !*g_mutable_config.name)) { \
            g_mutable_config.name = def; \
        }

#define GETi(name,type,min,max,def) \
        err = 0; \
        if (loaded) { \
            g_mutable_config.name = g_conf[#name].to_##type(&err); \
        } \
        if (!loaded || err || g_mutable_config.name < min || g_mutable_config.name > max) { \
            g_mutable_config.name = def; \
        }

    int err; // No use, just to silent exceptions.
    GETs(PROCESS_BABYSITTER_NAME, "Matrix Babysitter", 0);
    GETs(PROCESS_CENTRAL_NAME,    "Matrix Agent",      0);

    GETi(DEFAULT_SERVER_INTERVAL,               int32, 1, 3600, 300);
    GETi(DEFAULT_HEARTBEAT_INTERVAL,            int32, 1, 3600,  10);
    GETi(DEFAULT_MACHINE_MONITOR_INTERVAL,      int32, 1, 3600,  10);
    GETi(DEFAULT_INSTANCE_MONITOR_INTERVAL,     int32, 1, 3600,  10);
    GETi(DEFAULT_MANAGER_GC_INTERVAL,           int32, 1, 3600,  10);
    GETi(DEFAULT_MANAGER_INSTALL_TIMEO,         int32, 1, 3600,  30);
    GETi(DEFAULT_MANAGER_REMOVE_TIMEO,          int32, 1, 3600,  30);
    GETi(AGENT_CONTROL_PORT,                   uint16, 1, 1023, 410);
    GETi(AGENT_HTTPD_PORT,                     uint16, 8000, 9000, 80);

    GETs(MASTER_ADDRESS,        "master.matrix.baidu.com", 0);
    GETi(MASTER_CONNECT_TIMEOUT,    int32,  1, 300000, 10000);
    GETi(MASTER_WRITE_TIMEOUT,      int32,  1, 300000, 10000);
    GETi(MASTER_READ_TIMEOUT,       int32,  1, 300000, 10000);
    GETi(MASTER_PORT,               uint16, 1,  49151,   409);

    GETs(EXECUTOR_SOCKET_NAME, "232ce5fg-16d1-48c3-bd86-ae1739a4c4db", 0);

    GETs(LOG_FILE,   "matrix.log",    0);
    GETi(LOG_COLOR,  int32, 0, 1,     1);
    GETi(LOG_LEVEL,  int32, 0, 16,    8);
    GETi(LOG_SIZE,   int32, 1, 10240, 2048);

    load_path(loaded, "MATRIX_ROOT", "/home/matrix", &g_matrix_root);
    g_mutable_config.MATRIX_ROOT = g_matrix_root.c_str();

    g_matrix_containers_path = g_matrix_root + "/containers";
    g_mutable_config.MATRIX_CONTAINERS_PATH = g_matrix_containers_path.c_str();

    g_matrix_tmp_path = g_matrix_root + "/tmp";
    g_mutable_config.MATRIX_TMP_PATH = g_matrix_tmp_path.c_str();

    g_matrix_deploy_log_path = g_matrix_root + "/deploy_logs";
    g_mutable_config.MATRIX_DEPLOY_LOG_PATH = g_matrix_deploy_log_path.c_str();

    load_path(loaded, "CGROUP_ROOT", "/cgroups", &g_cgroup_root);
    g_mutable_config.CGROUP_ROOT = g_cgroup_root.c_str();

    load_path(loaded, "LOG_PATH", "/noah/log", &g_log_path);
    g_mutable_config.LOG_PATH = g_log_path.c_str();

    load_path(loaded, "META_PATH", "/noah/tmp/matrix_meta", &g_meta_path);
    g_mutable_config.META_PATH = g_meta_path.c_str();

    load_path(loaded, "TEMPORARY_PATH", "/noah/tmp", &g_temp_path);
    g_mutable_config.TEMPORARY_PATH = g_temp_path.c_str();

    const char * p = cmdline_module_dirname();
    g_adapter_scripts_path = p == NULL ? "" : p;
    g_adapter_scripts_path.append("/../adapter_scripts");
    g_mutable_config.ADAPTER_SCRIPTS_PATH = g_adapter_scripts_path.c_str();

    g_config = &g_mutable_config;
    return 0;
}
