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

class DlpProxyContext
{
private:
    int _port;
    int _fd;
    std::vector<int> sports;
public:
    DlpProxyContext();
    virtual ~DlpProxyContext();
public:
    virtual int initialize(int p, int f, std::vector<int> sps);
    virtual int fd();
    virtual int port();
};

class DlpProxyConnection
{
private:
    DlpProxyContext* context;
    st_netfd_t stfd;
public:
    DlpProxyConnection();
    virtual ~DlpProxyConnection();
public:
    virtual int initilaize(DlpProxyContext* c, st_netfd_t s);
    virtual int fd();
};

extern int dlp_run_proxyer(std::vector<int> ports, std::vector<int> fds, std::vector<int> sports);

#endif
