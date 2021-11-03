#include "dns.h"
#include "url_parse.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char dns_servers[15] = "127.0.0.1";
char query_type[4]   = "A";
int  main(int argc, char **argv) {
    char host[100];

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

        printf("Host : %s\n", host);
        get_info(host, dns_servers, query_type);
    }
    return 0;
}
