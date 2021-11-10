#include "asciilogo.h"
#include "dns.h"
#include "embed/xgetopt.h"
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct xoption options[] = {
        {'v', "version", xargument_no, NULL, -1},
        {'h', "help", xargument_no, NULL, -1},
        {'r', "recurse", xargument_no, NULL, -1},
        {'q', "query-type", xargument_required, NULL, -1},
        {'d', "domain", xargument_required, NULL, -1},
        {0, NULL, xargument_no, NULL, 0},
};
int  recurse = 0;
void conf_dns();
void display_help();
int  main(int argc, const char **argv) {
    conf_dns();

    int         opt, optind;
    const char *optarg;
    char        query_type[10] = "A";
    char        host[100];

    optind = 0;
    opt    = xgetopt(argc, argv, options, &optind, &optarg);
    while (opt != -1) {
        switch (opt) {
        case 0: break;
        case 'r': recurse = 1; break;
        case 'q': strcpy(query_type, optarg); break;
        case 'v': printf("%s\n", ascii_logo); return 0;
        case 'h': display_help(); return -1;
        case 'd':
        default:
            strcpy(host, optarg);
            printf("Host : %s\n", host);
            get_info(host, _res.nsaddr_list[0], query_type);
        }
        opt = xgetopt(argc, argv, options, &optind, &optarg);
    }
}

void display_help() {
    printf("Usage: dns_server [options]\n"
           "  -r or --recurse\n"
           "                       (tell server to work recursively, default no)\n"
           "  -q <query type> or --query-type=<query type>\n"
           "                       (make query of type, default A)\n"
           "  -d <domain name> or --domain=<domain name>\n"
           "                       (Send the domain name of to query about)\n"
           "  -h, --help           (print help and exit)\n"
           "  -v, --version        (print version and exit)\n");
};


void conf_dns() {
    res_init();
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(_res.nsaddr_list[0].sin_addr), str, INET_ADDRSTRLEN);
    printf("SERVER : %s\nADDRESS : %s#%u\n",
           str,
           str,
           htons(_res.nsaddr_list[0].sin_port));
}
