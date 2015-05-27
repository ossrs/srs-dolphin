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
using namespace std;

#include <st.h>

int dlp_get_id()
{
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
    
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
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
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    if (bind(fd, (const sockaddr*)&addr, sizeof(sockaddr_in)) == -1) {
        ret = ERROR_SOCKET_LISTEN;
        dlp_error("bind socket error. port=%d, ret=%d", port, ret);
        return ret;
    }
    dlp_verbose("bind socket success. port=%d, fd=%d", port, fd);
    
    return ret;
}
