/*
 The MIT License (MIT)
 
 Copyright (c) 2015 winlin
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <dlp_core.hpp>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <map>
using namespace std;

#include <st.h>

// nginx also set to 512
#define SERVER_LISTEN_BACKLOG 512

bool st_initilaized = false;
int dlp_master_id = getpid();
std::map<st_thread_t, int> cache;

int dlp_generate_id()
{
    if (!st_initilaized) {
        return 0;
    }
    
    static int id = 100;
    
    int gid = id++;
    cache[st_thread_self()] = gid;
    return gid;
}

int dlp_get_id()
{
    if (st_initilaized) {
        return cache[st_thread_self()];
    }
    return 0;
}

vector<int> dlp_list_to_ints(string str_list)
{
    vector<int> ints;
    
    std::string p = str_list;
    dlp_verbose("parse string list: %s", p.c_str());
    
    size_t pos = string::npos;
    while ((pos = p.find(",")) != string::npos) {
        std::string value = p.substr(0, pos);
        if (!value.empty()) {
            dlp_verbose("int value: %s", value.c_str());
            ints.push_back(::atoi(value.c_str()));
        }
        p = p.substr(pos + 1);
    }
    
    if (!p.empty()) {
        dlp_verbose("int value: %s", p.c_str());
        ints.push_back(::atoi(p.c_str()));
    }
    
    dlp_verbose("string list parsed.");
    
    return ints;
}

int dlp_listen_tcp(int port, int& fd)
{
    int ret = ERROR_SUCCESS;
    
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        ret = ERROR_SOCKET_LISTEN;
        dlp_error("create linux socket error. port=%d, ret=%d", port, ret);
        return ret;
    }
    dlp_verbose("create linux socket success. port=%d, fd=%d", port, fd);
    
    int reuse_socket = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_socket, sizeof(int)) == -1) {
        ret = ERROR_SOCKET_LISTEN;
        dlp_error("setsockopt reuse-addr error. port=%d, ret=%d", port, ret);
        return ret;
    }
    dlp_verbose("setsockopt reuse-addr success. port=%d, fd=%d", port, fd);
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    //addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(fd, (const sockaddr*)&addr, sizeof(sockaddr_in)) == -1) {
        ret = ERROR_SOCKET_LISTEN;
        dlp_error("bind socket error. port=%d, ret=%d", port, ret);
        return ret;
    }
    dlp_verbose("bind socket success. port=%d, fd=%d", port, fd);
    
    if (::listen(fd, SERVER_LISTEN_BACKLOG) == -1) {
        ret = ERROR_SOCKET_LISTEN;
        dlp_error("listen socket error. port=%d, ret=%d", port, ret);
        return ret;
    }
    dlp_verbose("listen socket success. port=%d, fd=%d", port, fd);
    
    return ret;
}

int dlp_st_init()
{
    int ret = ERROR_SUCCESS;
    
    // Select the best event system available on the OS. In Linux this is
    // epoll(). On BSD it will be kqueue.
    if (st_set_eventsys(ST_EVENTSYS_ALT) == -1) {
        ret = ERROR_ST_INITIALIZE;
        dlp_error("st_set_eventsys use %s failed. ret=%d", st_get_eventsys_name(), ret);
        return ret;
    }
    dlp_trace("st_set_eventsys to %s", st_get_eventsys_name());
    
    if(st_init() != 0){
        ret = ERROR_ST_INITIALIZE;
        dlp_error("st_init failed. ret=%d", ret);
        return ret;
    }
    dlp_trace("st_init success, use %s", st_get_eventsys_name());
    
    st_initilaized = true;
    dlp_generate_id();
    dlp_trace("st main thread, cid=%d", dlp_get_id());
    
    return ret;
}

string dlp_get_peer_ip(int fd)
{
    std::string ip;
    
    // discovery client information
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    if (getpeername(fd, (sockaddr*)&addr, &addrlen) == -1) {
        return ip;
    }
    dlp_verbose("get peer name success.");
    
    // ip v4 or v6
    char buf[INET6_ADDRSTRLEN];
    memset(buf, 0, sizeof(buf));
    
    if ((inet_ntop(addr.sin_family, &addr.sin_addr, buf, sizeof(buf))) == NULL) {
        return ip;
    }
    dlp_verbose("get peer ip of client ip=%s, fd=%d", buf, fd);
    
    ip = buf;
    
    dlp_verbose("get peer ip success. ip=%s, fd=%d", ip.c_str(), fd);
    
    return ip;
}
