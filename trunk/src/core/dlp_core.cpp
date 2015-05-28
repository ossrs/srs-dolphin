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
#include <netdb.h>
#include <arpa/inet.h>

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

void dlp_close_stfd(st_netfd_t& stfd)
{
    if (stfd) {
        int fd = st_netfd_fileno(stfd);
        st_netfd_close(stfd);
        stfd = NULL;
        
        // st does not close it sometimes,
        // close it manually.
        close(fd);
    }
}

string dlp_dns_resolve(string host)
{
    if (inet_addr(host.c_str()) != INADDR_NONE) {
        return host;
    }
    
    hostent* answer = gethostbyname(host.c_str());
    if (answer == NULL) {
        return "";
    }
    
    char ipv4[16];
    memset(ipv4, 0, sizeof(ipv4));
    if (answer->h_length > 0) {
        inet_ntop(AF_INET, answer->h_addr_list[0], ipv4, sizeof(ipv4));
    }
    
    return ipv4;
}

int dlp_socket_connect(string server, int port, st_utime_t timeout, st_netfd_t* pstfd)
{
    int ret = ERROR_SUCCESS;
    
    *pstfd = NULL;
    st_netfd_t stfd = NULL;
    sockaddr_in addr;
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        ret = ERROR_ST_SOCKET;
        dlp_error("create socket error. ret=%d", ret);
        return ret;
    }
    
    dlp_assert(!stfd);
    stfd = st_netfd_open_socket(sock);
    if(stfd == NULL){
        ret = ERROR_ST_SOCKET;
        dlp_error("st_netfd_open_socket failed. ret=%d", ret);
        return ret;
    }
    
    // connect to server.
    std::string ip = dlp_dns_resolve(server);
    if (ip.empty()) {
        ret = ERROR_ST_SOCKET;
        dlp_error("dns resolve server error, ip empty. ret=%d", ret);
        goto failed;
    }
    
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    if (st_connect(stfd, (const struct sockaddr*)&addr, sizeof(sockaddr_in), timeout) == -1){
        ret = ERROR_ST_SOCKET;
        dlp_error("connect to server error. ip=%s, port=%d, ret=%d", ip.c_str(), port, ret);
        goto failed;
    }
    dlp_info("connect ok. server=%s, ip=%s, port=%d", server.c_str(), ip.c_str(), port);
    
    *pstfd = stfd;
    return ret;
    
failed:
    if (stfd) {
        dlp_close_stfd(stfd);
    }
    return ret;
}

DlpStSocket::DlpStSocket(st_netfd_t client_stfd)
{
    stfd = client_stfd;
    send_timeout = recv_timeout = ST_UTIME_NO_TIMEOUT;
    recv_bytes = send_bytes = 0;
}

DlpStSocket::~DlpStSocket()
{
}

bool DlpStSocket::is_never_timeout(int64_t timeout_us)
{
    return timeout_us == (int64_t)ST_UTIME_NO_TIMEOUT;
}

void DlpStSocket::set_recv_timeout(int64_t timeout_us)
{
    recv_timeout = timeout_us;
}

int64_t DlpStSocket::get_recv_timeout()
{
    return recv_timeout;
}

void DlpStSocket::set_send_timeout(int64_t timeout_us)
{
    send_timeout = timeout_us;
}

int64_t DlpStSocket::get_send_timeout()
{
    return send_timeout;
}

int64_t DlpStSocket::get_recv_bytes()
{
    return recv_bytes;
}

int64_t DlpStSocket::get_send_bytes()
{
    return send_bytes;
}

int DlpStSocket::read(void* buf, size_t size, ssize_t* nread)
{
    int ret = ERROR_SUCCESS;
    
    ssize_t nb_read = st_read(stfd, buf, size, recv_timeout);
    if (nread) {
        *nread = nb_read;
    }
    
    // On success a non-negative integer indicating the number of bytes actually read is returned
    // (a value of 0 means the network connection is closed or end of file is reached).
    // Otherwise, a value of -1 is returned and errno is set to indicate the error.
    if (nb_read <= 0) {
        // @see https://github.com/simple-rtmp-server/srs/issues/200
        if (nb_read < 0 && errno == ETIME) {
            return ERROR_SOCKET_TIMEOUT;
        }
        
        if (nb_read == 0) {
            errno = ECONNRESET;
        }
        
        return ERROR_SOCKET_READ;
    }
    
    recv_bytes += nb_read;
    
    return ret;
}

int DlpStSocket::read_fully(void* buf, size_t size, ssize_t* nread)
{
    int ret = ERROR_SUCCESS;
    
    ssize_t nb_read = st_read_fully(stfd, buf, size, recv_timeout);
    if (nread) {
        *nread = nb_read;
    }
    
    // On success a non-negative integer indicating the number of bytes actually read is returned
    // (a value less than nbyte means the network connection is closed or end of file is reached)
    // Otherwise, a value of -1 is returned and errno is set to indicate the error.
    if (nb_read != (ssize_t)size) {
        // @see https://github.com/simple-rtmp-server/srs/issues/200
        if (nb_read < 0 && errno == ETIME) {
            return ERROR_SOCKET_TIMEOUT;
        }
        
        if (nb_read >= 0) {
            errno = ECONNRESET;
        }
        
        return ERROR_SOCKET_READ_FULLY;
    }
    
    recv_bytes += nb_read;
    
    return ret;
}

int DlpStSocket::write(void* buf, size_t size, ssize_t* nwrite)
{
    int ret = ERROR_SUCCESS;
    
    ssize_t nb_write = st_write(stfd, buf, size, send_timeout);
    if (nwrite) {
        *nwrite = nb_write;
    }
    
    // On success a non-negative integer equal to nbyte is returned.
    // Otherwise, a value of -1 is returned and errno is set to indicate the error.
    if (nb_write <= 0) {
        // @see https://github.com/simple-rtmp-server/srs/issues/200
        if (nb_write < 0 && errno == ETIME) {
            return ERROR_SOCKET_TIMEOUT;
        }
        
        return ERROR_SOCKET_WRITE;
    }
    
    send_bytes += nb_write;
    
    return ret;
}

int DlpStSocket::writev(const iovec *iov, int iov_size, ssize_t* nwrite)
{
    int ret = ERROR_SUCCESS;
    
    ssize_t nb_write = st_writev(stfd, iov, iov_size, send_timeout);
    if (nwrite) {
        *nwrite = nb_write;
    }
    
    // On success a non-negative integer equal to nbyte is returned.
    // Otherwise, a value of -1 is returned and errno is set to indicate the error.
    if (nb_write <= 0) {
        // @see https://github.com/simple-rtmp-server/srs/issues/200
        if (nb_write < 0 && errno == ETIME) {
            return ERROR_SOCKET_TIMEOUT;
        }
        
        return ERROR_SOCKET_WRITE;
    }
    
    send_bytes += nb_write;
    
    return ret;
}

