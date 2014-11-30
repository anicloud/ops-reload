#!/bin/sh

if [ -h "$0" ]; then
    real_path=`readlink "$0"`
    cd `dirname "${real_path}"`
else
    cd `dirname "$0"`
fi

ECHO="echo"

error()
{
    ${ECHO} "$@" >&2
}
usage()
{
    error "usage: matrix_jail <instance_id> [cmd]"
    error "       by default, cmd is bash"
}

main()
{
    if [ $# -eq 0 -o -z "$1" ]; then
        usage
        return 1
    fi

    for i in "/home/matrix/containers/"${1}; do
        local container_path="$i"
        break
    done

    #local container_path="/home/matrix/containers/${1}"
    if [ ! -d "${container_path}" ]; then
        error "container \"${1}\" not found."
        return 2
    fi

    if [ ! -d "${container_path}/home" -o ! -d "${container_path}/tmp" ]; then
        error "container \"${1}\" is not a jail container."
        return 3
    fi

    # add this to cgroup. but it's not root
    #if [ -d "/cgroups/cpu/${1}" && -d "/cgroups/memory/${1}" ]; then
    #    for i in "/cgroups"/*/"${1}"/tasks; do
    #        if [ "${i}" = "/cgroups/*/${1}/tasks" ]; then
    #            error "something error with cgroups."
    #            return 4
    #        fi
    #        ${ECHO} $$ >> $i || return 5
    #    done
    #else
    #    ${ECHO} "no cgroup found. continue..."
    #fi

    shift
    local cmd="bash"
    if [ -n "$*" ]; then
        cmd="$*"
    fi

    ./jail "${container_path}" work "$cmd"
    local ret=$?
    if [ $ret -ne 0 ]; then
        error "jump to jail error."
        return $ret
    fi
}

main "$@"
exit $?
