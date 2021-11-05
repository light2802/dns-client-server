#include "dns.h"
#include "url_parse.h"
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <netinet/in.h>
#include <resolv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char query_type[4] = "A";
char host[100];
void conf_dns();
int  main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            strcpy(query_type, &argv[i][1]);
            continue;
        }
        // parse url
        int               err;
        url_parser_url_t *parsed_url;
        parsed_url = (url_parser_url_t *)malloc(sizeof(url_parser_url_t));
        err        = parse_url(argv[i], false, parsed_url);
        if (err != 0) {
            printf("Invalid URL \"%s\".\n", argv[i]);
            continue;
        }
        strcpy(host, parsed_url->host);
        free_parsed_url(parsed_url);

        conf_dns();
        printf("Host : %s\n", host);
        get_info(host, _res.nsaddr_list[0], query_type);
    }
    return 0;
}

void conf_dns() {
    res_init();
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(_res.nsaddr_list[0].sin_addr), str, INET_ADDRSTRLEN);
    printf("SERVER : %s\nADDRESS : %s#%u\n",
           str,
           str,
           htons(_res.nsaddr_list[0].sin_port));
}
