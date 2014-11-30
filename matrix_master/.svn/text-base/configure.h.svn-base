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

#ifndef __OP_OPED_MATRIX_AGENT_CONFIGURE_H__
#define __OP_OPED_MATRIX_AGENT_CONFIGURE_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The configuration object. */
struct Configure {
    /** Babysitter */
    const char *PROCESS_BABYSITTER_NAME;

    /** Agent */
    const char *PROCESS_CENTRAL_NAME;

    /** Matrix root dir */
    const char *MATRIX_ROOT;

    /** Matrix containers path */
    const char *MATRIX_CONTAINERS_PATH;

    /** Matrix tmp path */
    const char *MATRIX_TMP_PATH;

    /** Matrix deploy log path */
    const char *MATRIX_DEPLOY_LOG_PATH;

    /** cgroup mount dir */
    const char *CGROUP_ROOT;

    /** Default reporter heartbeat interval in seconds. */
    int32_t     DEFAULT_HEARTBEAT_INTERVAL;

    /** Default server interval in seconds. */
    int32_t     DEFAULT_SERVER_INTERVAL;

    /** Default machine monitor interval in seconds. */
    int32_t     DEFAULT_MACHINE_MONITOR_INTERVAL;

    /** Default instance monitor interval in seconds. */
    int32_t     DEFAULT_INSTANCE_MONITOR_INTERVAL;

    /** Default manager gc interval in seconds. */
    int32_t     DEFAULT_MANAGER_GC_INTERVAL;

    /** Default manager install timeo in seconds. */
    int32_t     DEFAULT_MANAGER_INSTALL_TIMEO;

    /** Default manager remove timeo in seconds. */
    int32_t     DEFAULT_MANAGER_REMOVE_TIMEO;

    /** Master server address. */
    const char *MASTER_ADDRESS;

    /** Master server port. */
    uint16_t    MASTER_PORT;

    /** Agent control port. */
    uint16_t    AGENT_CONTROL_PORT;

    /** Agent httpd port. */
    uint16_t    AGENT_HTTPD_PORT;

    /** How many milliseconds will a connect(2) fail. */
    int32_t     MASTER_CONNECT_TIMEOUT;

    /** How many milliseconds will a write(2) fail. */
    int32_t     MASTER_WRITE_TIMEOUT;

    /** How many milliseconds will a read(2) fail. */
    int32_t     MASTER_READ_TIMEOUT;

    /** the socket name of executor */
    const char *EXECUTOR_SOCKET_NAME;

    /** Where to put intermediate files. */
    const char *TEMPORARY_PATH;

    /** Instance table 
        all instance save to 
        META_PATH/generation.offset.service_name
    */
    const char *META_PATH;

    /** Log filtering level. */
    int32_t     LOG_LEVEL;

    /** Enable log color. */
    int32_t     LOG_COLOR;

    /** Log files' directory name, no trailing slash, absolute path. */
    const char *LOG_PATH;

    /** Uses this string as its log file names. */
    const char *LOG_FILE;

    /** Uses this value as its log file size. */
    int32_t     LOG_SIZE;

    /** Adapter scripts path. */
    const char *ADAPTER_SCRIPTS_PATH;
};

/** The global constant configuration object. */
extern const struct Configure *g_config;

/** @warning Not reentrant. */
extern int load_global_configure(void);

#ifdef __cplusplus
}
#endif

#endif // __OP_OPED_MATRIX_AGENT_CONFIGURE_H__
