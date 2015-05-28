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

#include <dlp_core_srs.hpp>

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
using namespace std;

#include <st.h>

int dlp_fork_srs(int rtmp_port, string binary, string conf)
{
    int ret = ERROR_SUCCESS;
    
    // close other fds
    // TODO: do in right way.
    for (int i = 3; i < 1024; i++) {
        ::close(i);
    }
    
    // params for srs.
    std::string argv0 = binary;
    std::string argv1 = "-c";
    std::string argv2 = conf;
    std::string argv3 = "-p";
    char argv4[10];
    snprintf(argv4, sizeof(argv4), "%d", rtmp_port);
    // TODO: FIXME: should specifies the log file and tank.
    
    char** argv = new char*[5 + 1];
    argv[0] = (char*)argv0.data();
    argv[1] = (char*)argv1.data();
    argv[2] = (char*)argv2.data();
    argv[3] = (char*)argv3.data();
    argv[4] = (char*)argv4;
    argv[5] = NULL;
    
    dlp_trace("exec srs: %s %s %s %s %s", argv[0], argv[1], argv[2], argv[3], argv[4]);
    
    // TODO: execv or execvp
    ret = execv(binary.data(), argv);
    
    return ret;
}

int dlp_run_srs(int rtmp_port, string binary, string conf)
{
    int ret = ERROR_SUCCESS;
    
    // set the title to srs
    dlp_process_title->set_title(DLP_SRS);
    
    dlp_trace("dolphin srs serve port %d", rtmp_port);
    
    pid_t pid = -1;
    if ((pid = fork()) < 0) {
        ret = ERROR_FORK_SRS;
        dlp_error("fork srs failed. ret=%d", ret);
        return ret;
    }
    
    // child process: ffmpeg encoder engine.
    if (pid == 0) {
        ret = dlp_fork_srs(rtmp_port, binary, conf);
        exit(ret);
    }
    
    // parent.
    dlp_trace("fork srs pid=%d, port=%d, binary=%s, conf=%s", pid, rtmp_port, binary.c_str(), conf.c_str());
    
    for (;;) {
        int status = 0;
        if (waitpid(pid, &status, WNOHANG) > 0) {
            dlp_warn("srs quit, status=%d, pid=%d", status, pid);
        }
        
        // update the title with dynamic data.
        char ptitle[256];
        snprintf(ptitle, sizeof(ptitle), "%s(%dr)", DLP_SRS, rtmp_port);
        dlp_process_title->set_title(ptitle);
        
        // use system sleep.
        usleep(DLP_CYCLE_TIEOUT_MS * 1000);
    }
    
    return ret;
}
