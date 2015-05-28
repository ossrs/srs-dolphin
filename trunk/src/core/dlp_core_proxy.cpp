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

#include <algorithm>
using namespace std;

#include <st.h>

DlpProxyServer::DlpProxyServer()
{
    load = 0;
}

DlpProxyServer::~DlpProxyServer()
{
}

DlpProxyContext::DlpProxyContext(DlpProxyServer* s)
{
    _port = -1;
    _fd = -1;
    server = s;
}

DlpProxyContext::~DlpProxyContext()
{
    ::close(_fd);
    
    std::vector<DlpProxySrs*>::iterator it;
    for (it = sports.begin(); it != sports.end(); ++it) {
        DlpProxySrs* srs = *it;
        dlp_freep(srs);
    }
    sports.clear();
}

int DlpProxyContext::initialize(int p, int f, vector<int> sps)
{
    int ret = ERROR_SUCCESS;
    
    _port = p;
    _fd = f;
    
    for (int i = 0; i < (int)sps.size(); i++) {
        DlpProxySrs* srs = new DlpProxySrs();
        srs->port = sps.at(i);
        srs->load = 0;
        sports.push_back(srs);
    }
    
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

DlpProxySrs* DlpProxyContext::choose_srs()
{
    DlpProxySrs* match = NULL;
    
    std::vector<DlpProxySrs*>::iterator it;
    for (it = sports.begin(); it != sports.end(); ++it) {
        DlpProxySrs* srs = *it;
        if (!match || match->load > srs->load) {
            match = srs;
        }
    }
    
    if (match) {
        match->load++;
        server->load++;
    }
    
    return match;
}

void DlpProxyContext::release_srs(DlpProxySrs* srs)
{
    std::vector<DlpProxySrs*>::iterator it;
    it = std::find(sports.begin(), sports.end(), srs);
    
    if (it != sports.end()) {
        srs->load--;
        server->load--;
    }
}

DlpProxyConnection::DlpProxyConnection()
{
    _context = NULL;
    stfd = NULL;
}

DlpProxyConnection::~DlpProxyConnection()
{
    dlp_close_stfd(stfd);
}

int DlpProxyConnection::initilaize(DlpProxyContext* c, st_netfd_t s)
{
    int ret = ERROR_SUCCESS;
    
    _context = c;
    stfd = s;
    
    return ret;
}

int DlpProxyConnection::fd()
{
    return st_netfd_fileno(stfd);
}

DlpProxyContext* DlpProxyConnection::context()
{
    return _context;
}

int DlpProxyConnection::proxy(st_netfd_t srs)
{
    int ret = ERROR_SUCCESS;
    
    DlpStSocket skt_client(stfd);
    DlpStSocket skt_srs(srs);
    
    //skt_client.set_recv_timeout(0);
    //skt_srs.set_recv_timeout(500 * 1000);
    
    pollfd pds[2];
    pds[0].fd = st_netfd_fileno(stfd);
    pds[0].events = POLLIN;
    pds[1].fd = st_netfd_fileno(srs);
    pds[1].events = POLLIN;
    
    char buf[4096];
    ssize_t nread = 0;
        
    // TODO: FIXME: refine performance, use poll or writev.
    for (;;) {
        // check which is active.
        pds[0].revents = 0;
        pds[1].revents = 0;
        if (st_poll(pds, 2, ST_UTIME_NO_TIMEOUT) <= 0) {
            break;
        }
        
        // proxy client ==> srs.
        if (pds[0].revents & POLLIN) {
            nread = 0;
            
            if ((ret = skt_client.read(buf, 4096, &nread)) != ERROR_SUCCESS) {
                if (ret != ERROR_SOCKET_TIMEOUT) {
                    return ret;
                }
            }
            
            if (nread > 0 && (ret = skt_srs.write(buf, nread, NULL)) != ERROR_SUCCESS) {
                return ret;
            }
        }
        
        // proxy srs ==> client
        if (pds[1].revents & POLLIN) {
            nread = 0;
            
            if ((ret = skt_srs.read(buf, 4096, &nread)) != ERROR_SUCCESS) {
                if (ret != ERROR_SOCKET_TIMEOUT) {
                    return ret;
                }
            }
            
            if (nread > 0 && (ret = skt_client.write(buf, nread, NULL)) != ERROR_SUCCESS) {
                return ret;
            }
        }
    }
    
    return ret;
}

int dlp_connection_proxy(DlpProxyConnection* conn)
{
    int ret = ERROR_SUCCESS;
    
    DlpProxyContext* context = conn->context();
    
    // discovery client information.
    int fd = conn->fd();
    std::string ip = dlp_get_peer_ip(fd);
    
    // choose the best SRS service por
    DlpProxySrs* srs = context->choose_srs();
    dlp_assert(srs);
    dlp_trace("woker serve %s, fd=%d, srs_port=%d", ip.c_str(), fd, srs->port);
    
    // try to connect to srs.
    // TODO: FIXME: use timeout.
    // TODO: FIXME: retry next srs when error.
    st_netfd_t stfd = NULL;
    if ((ret = dlp_socket_connect("127.0.0.1", srs->port, ST_UTIME_NO_TIMEOUT, &stfd)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // do proxy.
    ret = conn->proxy(stfd);
    context->release_srs(srs);
    dlp_close_stfd(stfd);
    
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
    
    // set the title to worker
    dlp_process_title->set_title(DLP_WORKER);
    
    if ((ret = dlp_st_init()) != ERROR_SUCCESS) {
        return ret;
    }
    
    DlpProxyServer server;
    
    dlp_assert(ports.size() == fds.size());
    for (int i = 0; i < (int)ports.size(); i++) {
        int port = ports.at(i);
        int fd = fds.at(i);
        
        DlpProxyContext* context = new DlpProxyContext(&server);
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
    
    for (;;) {
        // update the title with dynamic data.
        char ptitle[256];
        snprintf(ptitle, sizeof(ptitle), "%s(%dc)", DLP_WORKER, server.load);
        dlp_process_title->set_title(ptitle);
        
        // use st sleep.
        st_usleep(DLP_CYCLE_TIEOUT_MS * 1000);
    }
    
    return ret;
}
