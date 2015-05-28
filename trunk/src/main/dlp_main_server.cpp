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
    printf("Usage: %s [-vVh] -w <worker process> -p <rtmp ports> -x <http ports> -b <srs binary> -c <srs_config> -s <srs rtmp ports> -y <srs http ports>\n"
        "       -?, -h          : show this help and exit.\n"
        "       -v, -V          : show the version of srs-dolphin and exit.\n"
        "       -w              : worker process, the number of woker process to fork.\n"
        "       -p              : rtmp ports, the ports for dolphin to listen at for rtmp.\n"
        "       -x              : http ports, the port for dolphin to listen at for http.\n"
        "       -b              : srs binary, the binary file path of srs, to serve the dolphin.\n"
        "       -c              : srs config, the config file for srs, to serve the dolphin.\n"
        "       -s              : srs rtmp ports, the port for srs to listen at for rtmp, to service the dolphin.\n"
        "       -y              : srs http ports, the port for srs to listen at for http, to service the dolphin."
        "For example, use srs linked to current dir:\n"
        "       %s -w 1 -p 19350 -x 8088 -b srs/objs/srs -c conf/dolphin.conf -s 2935 -y 8081\n"
        "       %s -w 4 -p 19350 -x 8088 -b srs/objs/srs -c conf/dolphin.conf -s 2935,2936,2937,2938 -y 8081,8082,8083,8084\n"
        "       %s -w 4 -p 19350 -x 8088 -b srs/objs/srs -c conf/dolphin.conf -s 2935,2936,2937,2938 -y 8081,8082,8083,8084\n"
        "Or, use srs at your home dir:\n"
        "       %s -w 1 -p 19350 -x 8088 -b ~/srs/objs/srs -c conf/dolphin.conf -s 2935 -y 8081\n"
        "       %s -w 4 -p 19350 -x 8088 -b ~/srs/objs/srs -c conf/dolphin.conf -s 2935,2936,2937,2938 -y 8081,8082,8083,8084\n"
        "       %s -w 4 -p 19350 -x 8088 -b ~/srs/objs/srs -c conf/dolphin.conf -s 2935,2936,2937,2938 -y 8081,8082,8083,8084\n"
        "@remark the conf/dolphin.conf is a edge where its origin is 1935, so dolphin use 19350.\n"
        "       while user can change the port of origin, edge and dolphin.\n",
        argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
}

void dlp_parse_options(
    int argc, char** argv,
    bool& show_version, bool& show_help, string& dlp_rtmp_ports, string& dlp_http_ports, int& dlp_worker_process,
    string& srs_rtmp_ports, string& srs_http_ports, string& srs_binary, string& srs_config_file
) {
    
    for (int i = 0; i < argc; i++) {
        char* p = argv[i];
        if (p[0] != '-') {
            continue;
        }
        if (p[1] != 'v' && p[1] != 'V' && p[1] != 'h' && p[1] != '?'
            && p[1] != 'p' && p[1] != 'w' && p[1] != 's' && p[1] != 'b' && p[1] != 'c'
            && p[1] != 'x' && p[1] != 'y'
        ) {
            continue;
        }
        
        if (p[1] == 'v' || p[1] == 'V') {
            show_version = true;
            break;
        }
        
        if (p[1] == 'h' || p[1] == '?') {
            show_help = true;
            break;
        }
        
        if (p[1] == 'p') {
            if (i + 1 >= argc) {
                printf("invalid options, params required, -p <rtmp ports>");
                exit(-1);
            }
            dlp_rtmp_ports = argv[++i];
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
                printf("invalid options, params required, -s <srs rtmp ports>");
                exit(-1);
            }
            srs_rtmp_ports = argv[++i];
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
        
        if (p[1] == 'x') {
            if (i + 1 >= argc) {
                printf("invalid options, params required, -x <http ports>");
                exit(-1);
            }
            dlp_http_ports = argv[++i];
            continue;
        }
        
        if (p[1] == 'y') {
            if (i + 1 >= argc) {
                printf("invalid options, params required, -y <srs http ports>");
                exit(-1);
            }
            srs_http_ports = argv[++i];
            continue;
        }
    }
    
    if (show_version) {
        printf("%s\n", DLP_VERSION);
        exit(0);
    }
    
    if (show_help || dlp_rtmp_ports.empty() || dlp_worker_process <= 0
        || srs_rtmp_ports.empty() || srs_binary.empty() || srs_config_file.empty()
        || dlp_http_ports.empty() || srs_http_ports.empty()
    ) {
        dlp_help(argv);
        if (show_help) {
            exit(0);
        } else {
            exit(-1);
        }
    }
}

int dlp_listen_tcp(vector<int> ports, vector<int>& fds)
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

int dlp_fork_workers(
    int workers, vector<int> srports, vector<int> shports,
    vector<int> rports, vector<int> rfds, vector<int> hports, vector<int> hfds,
    vector<int>& pids
) {
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
            ret = dlp_run_proxyer(rports, rfds, hports, hfds, srports, shports);
            exit(ret);
        }
        
        pids.push_back(pid);
        dlp_trace("dolphin fork worker pid=%d", pid);
    }
    
    return ret;
}

int dlp_fork_srs_servers(vector<int> rtmp_ports, vector<int> http_ports, string binary, string conf, vector<int>& pids)
{
    int ret = ERROR_SUCCESS;
    
    dlp_assert(rtmp_ports.size() == http_ports.size());
    for (int i = 0; i < (int)rtmp_ports.size(); i++) {
        pid_t pid = -1;
        int rtmp_port = rtmp_ports.at(i);
        int http_port = http_ports.at(i);
        
        // TODO: fork or vfork?
        if ((pid = fork()) < 0) {
            ret = ERROR_FORK_WORKER;
            dlp_error("vfork process failed. ret=%d", ret);
            return ret;
        }
        
        // child process: worker SRS engine.
        if (pid == 0) {
            ret = dlp_run_srs(rtmp_port, http_port, binary, conf);
            exit(ret);
        }
        
        pids.push_back(pid);
        dlp_trace("dolphin fork srs pid=%d, rtmp=%d, http=%d, binary=%s, conf=%s", pid, rtmp_port, http_port, binary.c_str(), conf.c_str());
    }
    
    return ret;
}

int main(int argc, char** argv)
{
    int ret = ERROR_SUCCESS;
    
    dlp_process_title->set_argcv(argc, argv);
    printf("srs-dolphin %s is a MultipleProcess for SRS, copyright (c) 2015 %s\n", DLP_VERSION, DLP_AUTHORS);
    
    // default params.
    bool show_version = false; // -vV
    bool show_help = false; // -h
    std::string dlp_rtmp_ports; // -p 1935
    std::string dlp_http_ports; // -x 8088
    int dlp_worker_process = 0; // -w 1
    std::string srs_rtmp_ports; // -s 1936
    std::string srs_http_ports; // -y 8081
    std::string srs_binary; // -b srs/objs/srs
    std::string srs_config_file; // -c srs/conf/srs.conf
    dlp_parse_options(
        argc, argv,
        show_version, show_help, dlp_rtmp_ports, dlp_http_ports, dlp_worker_process,
        srs_rtmp_ports, srs_http_ports, srs_binary, srs_config_file
    );
    
    dlp_trace("dolphin rtmp listen at %s", dlp_rtmp_ports.c_str());
    dlp_trace("dolphin http listen at %s", dlp_http_ports.c_str());
    dlp_trace("dolphin will fork %d worker process", dlp_worker_process);
    dlp_trace("dolphin start srs to rtmp listen at %s", srs_rtmp_ports.c_str());
    dlp_trace("dolphin start srs to http listen at %s", srs_http_ports.c_str());
    dlp_trace("dolphin use srs binary at %s", srs_binary.c_str());
    dlp_trace("dolphin use config file %s for srs", srs_config_file.c_str());
    
    std::vector<int> vsrs_rtmp_ports = dlp_list_to_ints(srs_rtmp_ports);
    std::vector<int> vdlp_rtmp_ports = dlp_list_to_ints(dlp_rtmp_ports);
    std::vector<int> vsrs_http_ports = dlp_list_to_ints(srs_http_ports);
    std::vector<int> vdlp_http_ports = dlp_list_to_ints(dlp_http_ports);
    
    // listen the serve socket for workers.
    std::vector<int> rtmp_fds;
    if ((ret = dlp_listen_tcp(vdlp_rtmp_ports, rtmp_fds)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // listen the serve socket for workers.
    std::vector<int> http_fds;
    if ((ret = dlp_listen_tcp(vdlp_http_ports, http_fds)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // fork all srs servers.
    std::vector<int> srs_pids;
    if ((ret = dlp_fork_srs_servers(vsrs_rtmp_ports, vsrs_http_ports, srs_binary, srs_config_file, srs_pids)) != ERROR_SUCCESS) {
        return ret;
    }
    
    // fork all workers.
    std::vector<int> worker_pids;
    if ((ret = dlp_fork_workers(dlp_worker_process, vsrs_rtmp_ports, vsrs_http_ports,
        vdlp_rtmp_ports, rtmp_fds, vdlp_http_ports, http_fds, worker_pids)) != ERROR_SUCCESS
    ) {
        return ret;
    }
    
    // set the title to master
    dlp_process_title->set_title(DLP_MASTER);
    
    // master wait all child terminated.
    int nb_dead = 0;
    dlp_trace("dolphin forked %d workers, %d srs, %d dead.", (int)worker_pids.size(), (int)srs_pids.size(), nb_dead);
    while (!worker_pids.empty() && !srs_pids.empty()){
        int status = 0;
        pid_t pid = -1;
        if ((pid = waitpid(-1, &status, WNOHANG)) < 0) {
            dlp_error("wait child process failed. pid=%d", pid);
            break;
        }
        
        // when got no pid.
        if (pid > 0) {
            vector<int>::iterator it = std::find(worker_pids.begin(), worker_pids.end(), pid);
            if (it != worker_pids.end()) {
                worker_pids.erase(it);
            } else if ((it = std::find(srs_pids.begin(), srs_pids.end(), pid)) != srs_pids.end()) {
                srs_pids.erase(it);
            }
            
            nb_dead++;
            dlp_trace("dolphin child process %d terminated, status=%d. there are running %d workers, %d srs, %d dead.",
                pid, status, (int)worker_pids.size(), (int)srs_pids.size(), nb_dead);
        }
        
        // update the title with dynamic data.
        char ptitle[256];
        snprintf(ptitle, sizeof(ptitle), "%s(%dw+%ds+%dd)", DLP_MASTER, (int)worker_pids.size(), (int)srs_pids.size(), nb_dead);
        dlp_process_title->set_title(ptitle);
        
        // use system sleep.
        usleep(DLP_CYCLE_TIEOUT_MS * 1000);
    }
    
    // killall others.
    for (int i = 0; i < (int)worker_pids.size(); i++) {
        int pid = worker_pids.at(i);
        kill(pid, SIGKILL);
    }
    for (int i = 0; i < (int)srs_pids.size(); i++) {
        int pid = srs_pids.at(i);
        kill(pid, SIGKILL);
    }
    
    dlp_trace("dolphin terminated.");
    return 0;
}

