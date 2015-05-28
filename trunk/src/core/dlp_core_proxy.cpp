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

int dlp_proxy(st_netfd_t stfd)
{
    int ret = ERROR_SUCCESS;
    
    int fd = st_netfd_fileno(stfd);
    std::string ip = dlp_get_peer_ip(fd);
    dlp_trace("woker serve fd=%d, %s", fd, ip.c_str());
    
    // TODO: FIXME: implements it.
    for (;;) {
        st_sleep(3);
    }
    
    return ret;
}

void* dlp_proxy_pfn(void* arg)
{
    st_netfd_t stfd = (st_netfd_t)arg;
    
    int ret = ERROR_SUCCESS;
    if ((ret = dlp_proxy(stfd)) != ERROR_SUCCESS) {
        dlp_warn("worker proxy failed, ret=%d", ret);
    } else {
        dlp_trace("worker proxy completed.");
    }
    
    return NULL;
}

int dlp_run_proxyer(int port, int fd)
{
    int ret = ERROR_SUCCESS;
    
    if ((ret = dlp_st_init()) != ERROR_SUCCESS) {
        return ret;
    }
    
    dlp_trace("dolphin worker serve port=%d, fd=%d", port, fd);
    
    st_netfd_t stfd = NULL;
    if ((stfd = st_netfd_open_socket(fd)) == NULL) {
        ret = ERROR_ST_OPEN_FD;
        dlp_error("worker open stfd failed. ret=%d", ret);
        return ret;
    }
    dlp_info("worker open fd ok, fd=%d", fd);
    
    for (;;) {
        dlp_verbose("worker proecess serve at port %d", port);
        st_netfd_t cfd = NULL;
        
        if ((cfd = st_accept(stfd, NULL, NULL, ST_UTIME_NO_TIMEOUT)) == NULL) {
            dlp_warn("ignore worker accept client error.");
            continue;
        }
        
        st_thread_t trd = NULL;
        if ((trd = st_thread_create(dlp_proxy_pfn, cfd, 0, 0)) == NULL) {
            dlp_warn("ignore worker thread create error.");
            continue;
        }
    }
    
    return ret;
}
