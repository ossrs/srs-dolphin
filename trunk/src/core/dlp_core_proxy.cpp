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

#include <dlp_core_proxy.hpp>

using namespace std;

#include <st.h>

DlpProxyContext::DlpProxyContext()
{
    _port = -1;
    _fd = -1;
}

DlpProxyContext::~DlpProxyContext()
{
    ::close(_fd);
}

int DlpProxyContext::initialize(int p, int f, vector<int> sps)
{
    int ret = ERROR_SUCCESS;
    
    _port = p;
    _fd = f;
    sports = sps;
    
    return ret;
}

int DlpProxyContext::fd()
{
    return _fd;
}

int DlpProxyContext::port()
{
    return _port;
}

DlpProxyConnection::DlpProxyConnection()
{
    context = NULL;
    stfd = NULL;
}

DlpProxyConnection::~DlpProxyConnection()
{
    dlp_close_stfd(stfd);
}

int DlpProxyConnection::initilaize(DlpProxyContext* c, st_netfd_t s)
{
    int ret = ERROR_SUCCESS;
    
    context = c;
    stfd = s;
    
    return ret;
}

int DlpProxyConnection::fd()
{
    return st_netfd_fileno(stfd);
}

int dlp_connection_proxy(DlpProxyConnection* conn)
{
    int ret = ERROR_SUCCESS;
    
    int fd = conn->fd();
    std::string ip = dlp_get_peer_ip(fd);
    dlp_trace("woker serve fd=%d, %s", fd, ip.c_str());
    
    // TODO: FIXME: implements it.
    for (;;) {
        st_sleep(3);
    }
    
    return ret;
}

void* dlp_connection_pfn(void* arg)
{
    DlpProxyConnection* conn = (DlpProxyConnection*)arg;
    dlp_assert(conn);
    
    int ret = ERROR_SUCCESS;
    if ((ret = dlp_connection_proxy(conn)) != ERROR_SUCCESS) {
        dlp_warn("worker proxy connection failed, ret=%d", ret);
    } else {
        dlp_trace("worker proxy connection completed.");
    }
    
    dlp_freep(conn);
    
    return NULL;
}

int dlp_context_proxy(DlpProxyContext* context)
{
    int ret = ERROR_SUCCESS;
    
    dlp_trace("dolphin worker serve port=%d, fd=%d", context->port(), context->fd());
    
    st_netfd_t stfd = NULL;
    if ((stfd = st_netfd_open_socket(context->fd())) == NULL) {
        ret = ERROR_ST_OPEN_FD;
        dlp_error("worker open stfd failed. ret=%d", ret);
        return ret;
    }
    dlp_info("worker open fd ok, fd=%d", context->fd());
    
    for (;;) {
        dlp_verbose("worker proecess serve at port %d", context->port());
        st_netfd_t cfd = NULL;
        
        if ((cfd = st_accept(stfd, NULL, NULL, ST_UTIME_NO_TIMEOUT)) == NULL) {
            dlp_warn("ignore worker accept client error.");
            continue;
        }
        
        DlpProxyConnection* conn = new DlpProxyConnection();
        if ((ret = conn->initilaize(context, cfd)) != ERROR_SUCCESS) {
            return ret;
        }
        
        st_thread_t trd = NULL;
        if ((trd = st_thread_create(dlp_connection_pfn, conn, 0, 0)) == NULL) {
            dlp_freep(conn);
            
            dlp_warn("ignore worker thread create error.");
            continue;
        }
    }
    
    return ret;
}

void* dlp_context_fpn(void* arg)
{
    DlpProxyContext* context = (DlpProxyContext*)arg;
    dlp_assert(context);
    
    int ret = ERROR_SUCCESS;
    if ((ret = dlp_context_proxy(context)) != ERROR_SUCCESS) {
        dlp_warn("worker proxy context failed, ret=%d", ret);
    } else {
        dlp_trace("worker proxy context completed.");
    }
    
    dlp_freep(context);
    
    return NULL;
}

int dlp_run_proxyer(vector<int> ports, std::vector<int> fds, std::vector<int> sports)
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = dlp_st_init()) != ERROR_SUCCESS) {
        return ret;
    }
    
    dlp_assert(ports.size() == fds.size());
    for (int i = 0; i < (int)ports.size(); i++) {
        int port = ports.at(i);
        int fd = fds.at(i);
        
        DlpProxyContext* context = new DlpProxyContext();
        if ((ret = context->initialize(port, fd, sports)) != ERROR_SUCCESS) {
            dlp_freep(context);
            return ret;
        }
        
        st_thread_t trd = NULL;
        if ((trd = st_thread_create(dlp_context_fpn, context, 0, 0)) == NULL) {
            dlp_freep(context);
            
            ret = ERROR_ST_TRHEAD;
            dlp_warn("worker thread create error. ret=%d", ret);
            return ret;
        }
    }
    
    st_thread_exit(NULL);
    
    return ret;
}
