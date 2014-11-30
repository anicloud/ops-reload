/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

namespace cpp matrix
namespace java com.baidu.inf.matrix.master.rpc.masteragentproto

enum ThriftPackageType {
    ARCHER = 0,
    IMCI = 1,
}

enum ThriftHostState {
    INVALID = 0,
    AVAILABLE = 10,
    FAILED = 20
}

enum ThriftInstanceState {
    DEPLOYING = 10,
    STARTING = 20,
    RUNNING = 30,
    ERROR = 40,
    REMOVING = 50,
}

enum ThriftInstanceErrorCode {
    INSTANCE_OK = 0,
    INSTANCE_DEPLOY_TIMEOUT = 10,
    INSTANCE_DEPLOY_FAILED = 20,
    INSTANCE_HEALTH_CHECK_TIMEOUT = 30,
    INSTANCE_HEALTH_CHECK_FAILED = 40,
    INSTANCE_REMOVE_TIMEOUT = 50,
    INSTANCE_REMOVE_FAILED = 60,
}

struct ThriftResourceTuple {
    1:i64 cpuNum,          // number of 0.1 core, <= 0 is illegal
    2:i64 memoryMB,        // memory in MB, <= 0 is illegal
    3:i64 diskTotalMB      // disk space in MB, <= 0 is illegal
}

struct ThriftAgentInstancePort {
    1:i32 minPortInclude,       // 实例需要使用的端口范围最小值（闭区间）
    2:i32 maxPortInclude,       // 实例需要使用的端口范围最大值（闭区间）
    3:map<string, i32> port,    // 实例使用的端口
}


struct ThriftAgentInstanceMeta {
    1: string metaVersion,                      // meta版本，根据此字段判断meta是否变化，最大长度256字节（不包含末尾\0）
    2: string packageSource,                    // 源包路径，最大长度4096字节（不包含末尾\0）
    3: string packageVersion,                   // 源包版本
    4: ThriftPackageType packageType,           // 部署方式
    5: string deployDir,                        // 工作路径，最大长度4096字节（不包含末尾\0）
    6: ThriftAgentInstancePort port,            // 实例需要使用的端口
    7: map<string, string> processName,         // ps看到的进程（不包含路径），K/V最大长度4096字节（不包含末尾\0）
    8: ThriftResourceTuple resource,            // 资源需求
    9: map<string, string> tag,                 // instance tag，每一个tag的K/V最大长度4096字节（不包含末尾\0）
    10: i32 deployTimeoutSec,                   // 部署超时时间，0表示使用Agent默认值， 负值无效
    11: i32 healthCheckTimeoutSec,              // 健康检查超时时间，0表示使用Agent默认值， <0无效:
    12: string group
}

struct ThriftInstanceInfo {
    1: string serviceName,                          // 服务名，最大长度4096字节（不包含末尾\0）
    2: i32 offset,                                  // offset号
    3: ThriftAgentInstanceMeta instanceMeta,        // 实例元数据
    4: ThriftInstanceState instanceState,           // 实例状态
    5: ThriftInstanceErrorCode errorCode,           // 仅在instanceState是ERROR时使用
}


struct ThriftHostInfo {
    1: string hostIp,                           // 主机IP，最大长度16字节（不包含末尾\0）
    2: string hostName,                         // 主机名，最大长度4096字节（不包含末尾\0）
    3: ThriftResourceTuple totalResource,       //总资源
    4: ThriftHostState hostState,               // 主机状态
}

struct ThriftHeartbeatMessage {
    1: ThriftHostInfo hostInfo,
    2: list<ThriftInstanceInfo> instanceInfo,   // 当前实例列表
    3: bool firstHeartbeat                      // the first heartbeat after restart
}

struct ThriftAgentConf {
    1: i32 heartbeatIntervalSec,                // agent的心跳间隔, 0，负值，> 3600（1小时）都是非法的
    2: bool safeMode,                           // master处于safemode
    3: i32 agentHttpPort,                       // agent 监听HTTP的端口 // 负值, ==0, >= 65536都是非法的
    4: ThriftResourceTuple reservedResource,    //Agent需要保留的资源
}

struct ThriftHeartbeatResponse {
    1: ThriftAgentConf agentConf,
    2: list<ThriftInstanceInfo> expectInstance
}

service MasterAgentProtocol {
    ThriftHeartbeatResponse heartbeat(1: ThriftHeartbeatMessage msg)
}
