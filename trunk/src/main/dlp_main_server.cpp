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

#include <string>
using namespace std;

void parse_options(
    int argc, char** argv,
    bool& show_version, bool& show_help, string& dlp_proxy_ports, int& dlp_worker_process,
    string& srs_service_ports, string& srs_binary, string& srs_config_file
) {
    
    for (int i = 0; i < argc; i++) {
        char* p = argv[i];
        if (p[0] != '-' || (p[1] != 'v' && p[1] != 'V' && p[1] != 'h' && p[1] != 'p' && p[1] != 'w' && p[1] != 's' && p[1] != 'b' && p[1] != 'c')) {
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
        printf("Usage: %s -p <proxy ports> -w <worker process> -s <service ports> -b <srs binary> -c <srs_config>\n"
               "       -p proxy ports, the ports for dolphin to listen at.\n"
               "       -w worker process, the number of woker process to fork.\n"
               "       -s service ports, the port for srs to listen at, to service the dolphin.\n"
               "       -b srs binary, the binary file path of srs.\n"
               "       -c srs config, the config file for srs.\n"
               "For example:\n"
               "       %s -p 1935 -w 1 -s 1936 -b srs/objs/srs -c srs/conf/srs.conf\n"
               "       %s -p 1935 -w 1 -s 1936 -b ~/srs/objs/srs -c ~/srs/conf/srs.conf\n"
               "       %s -p 1935 -w 4 -s 1936,1937,1938,1939 -b srs/objs/srs -c srs/conf/srs.conf\n"
               "       %s -p 1935 -w 4 -s 1936,1937,1938,1939 -b ~/srs/objs/srs -c ~/srs/conf/srs.conf\n",
               argv[0], argv[0], argv[0], argv[0], argv[0]);
        if (show_help) {
            exit(0);
        } else {
            exit(-1);
        }
    }
}

int main(int argc, char** argv)
{
    printf("srs-dolphin %s is a MultipleProcess for SRS, copyright (c) 2015 %s\n", DLP_VERSION, DLP_AUTHORS);
    
    // default params.
    bool show_version = false; // -vV
    bool show_help = false; // -h
    std::string dlp_proxy_ports; // -p 1935
    int dlp_worker_process = 0; // -w 1
    std::string srs_service_ports; // -s 1936
    std::string srs_binary; // -b srs/objs/srs
    std::string srs_config_file; // -c srs/conf/srs.conf
    parse_options(
        argc, argv,
        show_version, show_help, dlp_proxy_ports, dlp_worker_process,
        srs_service_ports, srs_binary, srs_config_file
    );
    
    dlp_trace("dolphin listen at %s", dlp_proxy_ports.c_str());
    dlp_trace("dolphin will fork %d worker process", dlp_worker_process);
    dlp_trace("dolphin start srs to listen at %s", srs_service_ports.c_str());
    dlp_trace("dolphin use srs binary at %s", srs_binary.c_str());
    dlp_trace("dolphin use config file %s for srs", srs_config_file.c_str());
    
    dlp_trace("dolphin terminated.");
    return 0;
}
