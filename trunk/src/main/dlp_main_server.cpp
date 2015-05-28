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
#include <algorithm>
using namespace std;

#include <dlp_core_srs.hpp>
#include <dlp_core_proxy.hpp>

void dlp_help(char** argv)
{
    printf("Usage: %s -p <proxy ports> -w <worker process> -s <service ports> -b <srs binary> -c <srs_config>\n"
        "       -p proxy ports, the ports for dolphin to listen at.\n"
        "       -w worker process, the number of woker process to fork.\n"
        "       -s service ports, the port for srs to listen at, to service the dolphin.\n"
        "       -b srs binary, the binary file path of srs.\n"
        "       -c srs config, the config file for srs.\n"
        "For example, use srs linked to current dir:\n"
        "       %s -p 19350 -w 1 -s 2935 -b srs/objs/srs -c conf/dolphin.conf\n"
        "       %s -p 19350 -w 4 -s 2935,2936,2937,2938 -b srs/objs/srs -c conf/dolphin.conf\n"
        "       %s -p 19350 -w 4 -s 2935,2936,2937,2938 -b srs/objs/srs -c conf/dolphin.conf\n"
        "Or, use srs at your home dir:\n"
        "       %s -p 19350 -w 1 -s 2935 -b ~/srs/objs/srs -c conf/dolphin.conf\n"
        "       %s -p 19350 -w 4 -s 2935,2936,2937,2938 -b ~/srs/objs/srs -c conf/dolphin.conf\n"
        "       %s -p 19350 -w 4 -s 2935,2936,2937,2938 -b ~/srs/objs/srs -c conf/dolphin.conf\n"
        "@remark the conf/dolphin.conf is a edge where its origin is 1935, so dolphin use 19350.\n"
        "       while user can change the port of origin, edge and dolphin.\n",
        argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
}

void dlp_parse_options(
    int argc, char** argv,
    bool& show_version, bool& show_help, string& dlp_proxy_ports, int& dlp_worker_process,
    string& srs_service_ports, string& srs_binary, string& srs_config_file
) {
    
    for (int i = 0; i < argc; i++) {
        char* p = argv[i];
        if (p[0] != '-') {
            continue;
        }
        if (p[1] != 'v' && p[1] != 'V' && p[1] != 'h'
            && p[1] != 'p' && p[1] != 'w' && p[1] != 's' && p[1] != 'b' && p[1] != 'c'
        ) {
            continue;
        }
        
        if (p[1] == 'v' || p[1] == 'V') {
            show_version = true;
            break;
        }
        
        if (p[1] == 'h') {
            show_help = true;
            break;
        }
        
        if (p[1] == 'p') {
            if (i + 1 >= argc) {
                printf("invalid options, params required, -p <proxy ports>");
                exit(-1);
            }
            dlp_proxy_ports = argv[++i];
            continue;
        }
        
        if (p[1] == 'w') {
            if (i + 1 >= argc) {
                printf("invalid options, params required, -w <worker process>");
                exit(-1);
            }
            dlp_worker_process = ::atoi(argv[++i]);
            continue;
        }
        
        if (p[1] == 's') {
            if (i + 1 >= argc) {
                printf("invalid options, params required, -s <service ports>");
                exit(-1);
            }
            srs_service_ports = argv[++i];
            continue;
        }
        
        if (p[1] == 'b') {
            if (i + 1 >= argc) {
                printf("invalid options, params required, -b <srs binary>");
                exit(-1);
            }
            srs_binary = argv[++i];
            continue;
        }
        
        if (p[1] == 'c') {
            if (i + 1 >= argc) {
                printf("invalid options, params required, -c <srs config>");
                exit(-1);
            }
            srs_config_file = argv[++i];
            continue;
        }
    }
    
    if (show_version) {
        printf("%s\n", DLP_VERSION);
        exit(0);
    }
    
    if (show_help || dlp_proxy_ports.empty() || dlp_worker_process <= 0 || srs_service_ports.empty() || srs_binary.empty() || srs_config_file.empty()) {
        dlp_help(argv);
        if (show_help) {
            exit(0);
        } else {
            exit(-1);
        }
    }
}

int dlp_listen_rtmp(vector<int> ports, vector<int>& fds)
{
    int ret = ERROR_SUCCESS;
    
    for (int i = 0; i < (int)ports.size(); i++) {
        int port = ports.at(i);
        int fd = -1;
        
        if ((ret = dlp_listen_tcp(port, fd)) != ERROR_SUCCESS) {
            return ret;
        }
        
        dlp_info("dolphin serve at tcp://%d", port);
        fds.push_back(fd);
    }
    
    return ret;
}

int dlp_fork_workers(vector<int> ports, vector<int> fds, int workers, vector<int>& pids, vector<int> sports)
{
    int ret = ERROR_SUCCESS;
    
    for (int i = 0; i < workers; i++) {
        pid_t pid = -1;
        
        // TODO: fork or vfork?
        if ((pid = fork()) < 0) {
            ret = ERROR_FORK_WORKER;
            dlp_error("vfork process failed. ret=%d", ret);
            return ret;
        }
        
        // child process: worker proxy engine.
        if (pid == 0) {
            ret = dlp_run_proxyer(ports, fds, sports);
            exit(ret);
        }
        
        pids.push_back(pid);
        dlp_trace("dolphin fork worker pid=%d", pid);
    }
    
    return ret;
}

int dlp_fork_srs(vector<int> rtmp_ports, string binary, string conf, vector<int>& pids)
{
    int ret = ERROR_SUCCESS;
    
    for (int i = 0; i < (int)rtmp_ports.size(); i++) {
        pid_t pid = -1;
        int rtmp_port = rtmp_ports.at(i);
        
        // TODO: fork or vfork?
        if ((pid = fork()) < 0) {
            ret = ERROR_FORK_WORKER;
            dlp_error("vfork process failed. ret=%d", ret);
            return ret;
        }
        
        // child process: worker SRS engine.
        if (pid == 0) {
            ret = dlp_run_srs(rtmp_port, binary, conf);
            exit(ret);
        }
        
        pids.push_back(pid);
        dlp_trace("dolphin fork srs pid=%d, listen=%d, binary=%s, conf=%s", pid, rtmp_port, binary.c_str(), conf.c_str());
    }
    
    return ret;
}

int main(int argc, char** argv)
{
    int ret = ERROR_SUCCESS;
    
    printf("srs-dolphin %s is a MultipleProcess for SRS, copyright (c) 2015 %s\n", DLP_VERSION, DLP_AUTHORS);
    
    // default params.
    bool show_version = false; // -vV
    bool show_help = false; // -h
    std::string dlp_proxy_ports; // -p 1935
    int dlp_worker_process = 0; // -w 1
    std::string srs_service_ports; // -s 1936
    std::string srs_binary; // -b srs/objs/srs
    std::string srs_config_file; // -c srs/conf/srs.conf
    dlp_parse_options(
        argc, argv,
        show_version, show_help, dlp_proxy_ports, dlp_worker_process,
        srs_service_ports, srs_binary, srs_config_file
    );
    
    dlp_trace("dolphin listen at %s", dlp_proxy_ports.c_str());
    dlp_trace("dolphin will fork %d worker process", dlp_worker_process);
    dlp_trace("dolphin start srs to listen at %s", srs_service_ports.c_str());
    dlp_trace("dolphin use srs binary at %s", srs_binary.c_str());
    dlp_trace("dolphin use config file %s for srs", srs_config_file.c_str());
    
    std::vector<int> rtmp_service_ports = dlp_list_to_ints(srs_service_ports);
    std::vector<int> rtmp_proxy_ports = dlp_list_to_ints(dlp_proxy_ports);
    
    // listen the serve socket for workers.
    std:vector<int> rtmp_fds;
    if ((ret = dlp_listen_rtmp(rtmp_proxy_ports, rtmp_fds)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // fork all srs servers.
    std::vector<int> srs_pids;
    if ((ret = dlp_fork_srs(rtmp_service_ports, srs_binary, srs_config_file, srs_pids)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // fork all workers.
    std::vector<int> worker_pids;
    if ((ret = dlp_fork_workers(rtmp_proxy_ports, rtmp_fds, dlp_worker_process, worker_pids, rtmp_service_ports)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // master wait all child terminated.
    dlp_trace("dolphin forked %d workers, %d srs.", (int)worker_pids.size(), (int)srs_pids.size());
    while (!worker_pids.empty() || !srs_pids.empty()){
        int status = 0;
        pid_t pid = -1;
        if ((pid = waitpid(-1, &status, 0)) < 0) {
            dlp_error("wait child process failed. pid=%d", pid);
            break;
        }
        
        vector<int>::iterator it = std::find(worker_pids.begin(), worker_pids.end(), pid);
        if (it != worker_pids.end()) {
            worker_pids.erase(it);
        } else if ((it = std::find(srs_pids.begin(), srs_pids.end(), pid)) != srs_pids.end()) {
            srs_pids.erase(it);
        }
        
        dlp_trace("dolphin child process %d terminated, status=%d. there are running %d workers, %d srs.",
            pid, status, (int)worker_pids.size(), (int)srs_pids.size());
    }
    
    dlp_trace("dolphin terminated.");
    return 0;
}

