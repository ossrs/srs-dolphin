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

#ifndef DLP_CORE_PROXY_HPP
#define DLP_CORE_PROXY_HPP

/*
 #include <dlp_core_proxy.hpp>
 */

#include <dlp_core.hpp>

#include <vector>

#include <st.h>

class DlpProxyConnection;

struct DlpProxySrs
{
    int port;
    int load;
};

struct DlpProxyServer
{
    int load;
    
    DlpProxyServer();
    virtual ~DlpProxyServer();
};

struct DlpProxyRecvContext
{
    DlpProxyConnection* conn;
    st_netfd_t srs;
    bool cycle;
    bool terminated;
    DlpProxyRecvContext();
    virtual ~DlpProxyRecvContext();
};

class DlpProxyContext
{
private:
    int _port;
    int _fd;
    DlpProxyServer* server;
    std::vector<DlpProxySrs*> sports;
public:
    DlpProxyContext(DlpProxyServer* s);
    virtual ~DlpProxyContext();
public:
    virtual int initialize(int p, int f, std::vector<int> sps);
    virtual int fd();
    virtual int port();
    virtual DlpProxySrs* choose_srs();
    virtual void release_srs(DlpProxySrs* srs);
};

class DlpProxyConnection
{
private:
    DlpProxyContext* _context;
    st_netfd_t stfd;
public:
    DlpProxyConnection();
    virtual ~DlpProxyConnection();
public:
    virtual int initilaize(DlpProxyContext* c, st_netfd_t s);
    virtual DlpProxyContext* context();
    virtual int fd();
    virtual int proxy(st_netfd_t srs);
    virtual int proxy_recv(DlpProxyRecvContext* rc);
};

extern int dlp_run_proxyer(
    std::vector<int> rports, std::vector<int> rfds, std::vector<int> hports, std::vector<int> hfds,
    std::vector<int> sports, std::vector<int> shports
);

#endif
