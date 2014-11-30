#!/bin/sh

# 返回值说明：
# 0 成功
# 1 参数不合泄17
# 2 配置文件路径为空
# 3 读取CT任务配置文件错误
# 4 部署任务失败
# 5 启用模块CT任务失败

cd `dirname $0`

CURL='curl -s'
KEY=423468798
HOST="http://noah.baidu.com"
DEPLOY_URL="${HOST}/ctweb/api/taskPostXml/${KEY}/deployCtTask/"
ENABLE_TASK_URL="${HOST}/ctweb/api/taskPostXml/${KEY}/enableModuleTask/"

function usage()
{
    echo "usage: $0 -m modulename -p modulepath -c configFile"
    echo "  -m modulename : The name of the deployed module."
    echo "  -p modulepath : The deploy path of the module."
    echo "  -c configFile : The path of the ct config file."
    echo "  -j            : run ct job in jail."
}

# 读取参数
while getopts m:p:c:j OPTION
do
    case $OPTION in
        m)
        moduleName=$OPTARG
        ;;
        p)
        modulePath=$OPTARG
        ;;
        c)
        confPath=$OPTARG
        ;;
        j)
        jail="True"
        ;;
        *)
        usage
        exit 1
        ;;
    esac
done

# CT配置文件路径为空
if [ X"$confPath" = X ]
then
    echo 'error: config file path is needed.'
    exit 2
fi

hostname=`hostname | sed s/\.baidu\.com$//g`

#读取CT配置文件内容
conf=`cat ${confPath}`
if [ "${jail}" = "True" ]; then
    containerId=`basename ${modulePath}`
    conf=`sed "s/command\s*:\s*\(.\+\)$/command: matrix_jail ${containerId} \"\1\"/g" ${confPath}`
fi
replace_conf=`sh ./transfer.sh "${conf}"`
if [ $? -ne 0 ]
then
    echo "error: read config file fail."
    exit 3
fi

statusTag='status'
msgTag='msg'

# 部署CT任务
deployXml=`${CURL} -d "_a=1" -d "hostname=${hostname}" -d "moduleName=${moduleName}" -d "modulePath=${modulePath}" -d "config=${replace_conf}" ${DEPLOY_URL}`
ret=''
for row in $deployXml
do
    ret="$ret $row"
done
status=`echo "$ret" | grep "<$statusTag>" | sed "s/^.*<$statusTag>\(.*\)<\/$statusTag>.*$/\1/g"`
msg=`echo "$ret" | grep "<$msgTag>" | sed "s/^.*<$msgTag>\(.*\)<\/$msgTag>.*$/\1/g"`
if [ $status != 'success' ]
then
    msgEcho=`echo ${msg} | iconv -f utf8 -t gbk`;
    echo "deploy failed: $msgEcho"
    exit 4
fi

# 启用CT任务
enableTasksXml=`${CURL} -d "_a=1" -d "hostname=${hostname}" -d "moduleName=${moduleName}" -d "modulePath=${modulePath}" ${ENABLE_TASK_URL}`
ret=''
for row in $enableTasksXml
do
    ret="$ret $row"
done
status=`echo "$ret" | grep "<$statusTag>" | sed "s/^.*<$statusTag>\(.*\)<\/$statusTag>.*$/\1/g"`
msg=`echo "$ret" | grep "<$msgTag>" | sed "s/^.*<$msgTag>\(.*\)<\/$msgTag>.*$/\1/g"`

if [ $status != 'success' ]
then
    echo "Enable tasks failed: ${msg}"
    exit 5
fi

exit 0
