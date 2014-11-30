#include <pwd.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <list>
#include <cerrno>
#include <cassert>
#include <cstdlib>
#include <stdexcept>


#include "op/oped/noah/sailor/utility/Logger.h"

#include "Utility.h"

namespace matrix {

using namespace std;
    
int get_process_name_from_pid(const pid_t pid, string* process_name)
{
    assert(process_name);
    process_name->clear();

    char buf[PATH_MAX];
    char path[64];
    FILE *file;
    int ret;

    //read process name from /proc/%d/stat
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (!file) {
        return -1;
    }
    ret = fscanf(file, "%*d (%15[^)]", buf);
    fclose(file);
    if (ret != 1) {
        return -2;
    }
    string name1(buf);

    //read process name by readlink of /proc/%d/exe
    snprintf(path, sizeof(path), "/proc/%d/exe", pid);
    ret = readlink(path, buf, sizeof(buf)-1);
    if (ret <= 0) {
        return -3;
    }
    buf[ret] = '\0';
    string name2(buf);

    //only keep file name
    string::size_type pos = name2.rfind('/');
    if (pos == string::npos) {
        return -4;
    }
    name2.erase(name2.begin(), name2.begin() + pos + 1);

    //erase '(deleted)'
    if (name2.length() > 10 && name2.compare(name2.length() - 10, 10, " (deleted)") == 0) {
        name2.erase(name2.length() - 10);
    }

    //if same, then use name from /proc/%d/exe
    //else, use name from /proc/%d/stat
    if (name2.compare(0, name1.size(), name1) == 0) {
        process_name->assign(name2);
    } else {
        process_name->assign(name1);
    }

    return 0;
}

void get_process_names(const list<pid_t>& pids, list<string>* processes)
{
    assert(processes);
    processes->clear();
    errno = 0;

    for (list<pid_t>::const_iterator iter = pids.begin(); iter != pids.end(); ++iter)
    {
        string process_name;
        if (get_process_name_from_pid(*iter, &process_name) == 0) {
            processes->push_back(process_name);
            LOG.debug("Utility: pid %d is: %s", *iter, process_name.c_str());
        }
    }
}

void get_port_and_fd_list(map<int32_t, uint64_t>* port_fd_list)
{
    assert(port_fd_list);
    port_fd_list->clear();

    ifstream stream;
    string line;
    string tmp;

    const string path[] = {"/proc/net/tcp", "/proc/net/tcp6"};
    for (int i=0; i<2; i++) {
        stream.open(path[i].c_str(), ifstream::in);
        while (getline(stream, line).good())
        {
            istringstream ss(line);
            string address, st, inode;
            ss >> tmp >> address >> tmp >> st;
            if (st == "st" || st == "03") {
                continue;
            }
            else if (st != "0A") {
                break;
            }
            ss >> tmp >> tmp >> tmp >> tmp >> tmp >> inode;

            int32_t port;
            uint64_t fd;
            if (sscanf(address.c_str(), "%*x:%x", &port) == 1 &&
                    sscanf(inode.c_str(), "%lu", &fd) ==1) {
                port_fd_list->insert(pair<int32_t, uint64_t>(port, fd));
            }
            else {
                LOG.warn("Utility: get port and fd from address %s and inode %s error", 
                        address.c_str(), inode.c_str());
            }
        }

        stream.close();
    }
}

void get_socket_fd(const list<pid_t>& pids, list<uint64_t>* fds)
{
    assert(fds);
    fds->clear();

    char path[64];
    char buf[64];
    DIR *dir;
    struct dirent *dp;

    for (list<pid_t>::const_iterator iter = pids.begin();
            iter != pids.end(); ++iter)
    {
        errno = 0;
        snprintf(path, sizeof(path), "/proc/%u/fd/", *iter);
        dir = opendir(path);
        if (dir == NULL) {
            continue;
        }
        while ((dp = readdir(dir)) != NULL) {
            if ((dp->d_type) != DT_LNK) {
                continue;
            }
            snprintf(path, sizeof(path), "/proc/%u/fd/%s", *iter, dp->d_name);
            int size = readlink(path, buf, sizeof(buf)-1);
            if (size <= 0) {
                continue;
            }
            buf[size] = '\0';

            uint64_t fd;
            if (sscanf(buf, "socket:[%lu]", &fd) == 1) {
                fds->push_back(fd);
            }
        }
        closedir(dir);
    }
}

bool get_uid_gid_of_specified_user(const string& user, uid_t* const uid, gid_t* const gid)
{
    assert(uid);
    assert(gid);
    errno = 0;

    struct passwd pwd;
    struct passwd *result;
    char *buf;
    size_t bufsize;

    bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (errno == EINVAL) {
        LOG.warn("Utility: get bufsize faild, errno: %d", errno);
        return false;
    }

    if (bufsize <= 0 || bufsize > 1048576 /*1m*/) {
        LOG.warn("Utility: bufsize is not valid, bufsize is %lu", bufsize);
        return false;
    }

    buf = (char *)malloc(bufsize);
    if (buf == NULL) {
        LOG.warn("Utility: malloc error!");
        return false;
    }

    int ret = getpwnam_r(user.c_str(), &pwd, buf, bufsize, &result);
    if (ret == 0) {
        *uid = pwd.pw_uid;
        *gid = pwd.pw_gid;
    } else {
        LOG.warn("Utility: getpwnam_r faild, errno %d", ret);
    }
    free(buf);
    return ret == 0;

}

//  数字 | 字母 | 下划线 | 中划线 | .
bool string_check_base(const std::string& str) 
{
    for(size_t i = 0 ; i < str.size() ; i++) {
        if((str[i] < 'a' || str[i] > 'z') && (str[i] < 'A' || str[i] > 'Z') && (str[i] < '0' || str[i] > '9')
            && str[i] != '-' && str[i] != '_' && str[i] != '.')
            return false;
    }
    return true;
}

//数字 | 字母 | 下划线 | 中划线 | : | / | . | @
bool string_check_url(const std::string& str) 
{
    for(size_t i = 0 ; i < str.size() ; i++) {
        if((str[i] < 'a' || str[i] > 'z') && (str[i] < 'A' || str[i] > 'Z') && (str[i] < '0' || str[i] > '9')
            && str[i] != '-' && str[i] != '_' && str[i] != ':' && str[i] != '/' && str[i] != '.' && str[i] != '@')
            return false;
    }
    return true;
}

//数字 | 字母 | 下划线 | 中划线 | . | /
bool string_check_path(const std::string& str) 
{
    for(size_t i = 0 ; i < str.size() ; i++) {
        if((str[i] < 'a' || str[i] > 'z') && (str[i] < 'A' || str[i] > 'Z') && (str[i] < '0' || str[i] > '9')
            && str[i] != '-' && str[i] != '_' && str[i] != '/' && str[i] != '.')
            return false;
    }
    return true;
}

// 数字 | 字母 | 下划线 | 中划线 | . | / 且必须以/打头
bool string_check_abspath(const std::string& str) 
{
    if (str.size() > 0 && str[0] != '/') {
        return false;
    }
    for(size_t i = 0 ; i < str.size() ; i++) {
        if((str[i] < 'a' || str[i] > 'z') && (str[i] < 'A' || str[i] > 'Z') && (str[i] < '0' || str[i] > '9')
            && str[i] != '-' && str[i] != '_' && str[i] != '/' && str[i] != '.')
            return false;
    }
    return true;
}

//数字 | 字母 | 下划线 | 中划线 | . 
bool string_check_version(const std::string& str) 
{
    for(size_t i = 0 ; i < str.size() ; i++) {
        if((str[i] < 'a' || str[i] > 'z') && (str[i] < 'A' || str[i] > 'Z') && (str[i] < '0' || str[i] > '9')
            && str[i] != '-' && str[i] != '_' && str[i] != '.')
            return false;
    }
    return true;
}

bool string_check_name(const std::string& str) 
{
    return string_check_version(str);
}

std::string time_to_str(struct timeval& val)
{
    char buffer[1024];
    struct tm tm_ret;
    time_t _tm = val.tv_sec;

    if(val.tv_sec || val.tv_usec) {
        localtime_r(&_tm, &tm_ret);

        snprintf(buffer, sizeof(buffer), "%d-%d-%d %d:%d:%d.%ld", 
            tm_ret.tm_year + 1900,
            tm_ret.tm_mon + 1,
            tm_ret.tm_mday,
            tm_ret.tm_hour,
            tm_ret.tm_min,
            tm_ret.tm_sec,
            val.tv_usec);
    } else {
        snprintf(buffer, sizeof(buffer), "N/A");
    }

    return std::string(buffer);
}

int64_t get_current_timestamp_ms()
{
    struct timeval current;
    if (gettimeofday(&current, NULL) == 0) {
        return current.tv_sec * 1000 + current.tv_usec / 1000;
    } else {
        throw std::runtime_error("get system time failed.");
    }
}

} /// namespace matrix
