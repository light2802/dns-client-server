#ifndef URL_PARSE_H
#define URL_PARSE_H
#include <stdbool.h>

//A simple C URL parser made for study purposes
typedef struct url_parser_url {
    char *protocol;
    char *host;
    int   port;
    char *path;
    char *query_string;
    int   host_exists;
    char *host_ip;
} url_parser_url_t;

void free_parsed_url(url_parser_url_t *url_parsed);
int  parse_url(char *url, bool verify_host, url_parser_url_t *parsed_url);

#endif
