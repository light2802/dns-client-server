#include "dns.h"
#include "url_parse.h"
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// For now sending requests to 8.8.8.8
// Later will be sent to own server
char *dns_servers = "127.0.0.53";

int main(int argc, char **argv) {
    char host[100];

    for (int i = 1; i < argc; i++) {
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
        get_info(host, dns_servers);
    }
    return 0;
}
