#include "dns.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
extern int recurse;
// convert www.google.com to 3www6google3com
void change_to_dns_name_format(unsigned char *dns, unsigned char *host) {
    int lock = 0, i;
    strcat((char *)host, ".");
    for (i = 0; i < strlen((char *)host); i++) {
        if (host[i] == '.') {
            *dns++ = i - lock;
            for (; lock < i; lock++) {
                *dns++ = host[lock];
            }
            lock++;
        }
    }
    *dns++ = '\0';
}

// make dns query for given hostname of given types and class
struct dns_query make_dns_query(char *hostname, int type) {
    // 6 layers of header each of 2 bytes
    unsigned char *    qname;
    struct dns_header *dns   = NULL;
    struct question *  qinfo = NULL;
    unsigned char      buffer[DNS_QUERY_BUFFER_SIZE];
    // Set the DNS structure to standard queries
    dns = (struct dns_header *)&buffer;

    dns->id = (unsigned short)htons(rand()); // random id doesn't really matter
    dns->qr = 0;                             // This is a query
    dns->opcode     = 0;                     // This is a standard query
    dns->aa         = 0;                     // Not Authoritative
    dns->tc         = 0;                     // This message is not truncated
    dns->rd         = recurse;               // Recursion Desired
    dns->ra         = 0; // Recursion not available! hey we dont have it (lol)
    dns->z          = 0;
    dns->ad         = 0;
    dns->cd         = 0;
    dns->rcode      = 0;
    dns->q_count    = htons(1); // we have only 1 question
    dns->ans_count  = 0;
    dns->auth_count = 0;
    dns->add_count  = 0;
    // point to the query portion
    qname = &buffer[sizeof(struct dns_header)];

    change_to_dns_name_format(qname, (unsigned char *)hostname);
    qinfo = (struct question
                     *)&buffer[sizeof(struct dns_header)
                               + (strlen((const char *)qname) + 1)]; // fill it

    qinfo->qtype  = htons(type); // type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(INTERNET); // its internet (lol)

    struct dns_query query = {.query = buffer,
                              .len   = sizeof(struct dns_header)
                                     + (strlen((const char *)qname) + 1)
                                     + sizeof(struct question)};
    return query;
}
unsigned char *
read_name(unsigned char *reader, unsigned char *buffer, int *count) {
    unsigned char *name;
    unsigned int   p = 0, jumped = 0, offset;
    int            i, j;
    name    = (unsigned char *)malloc(256);
    *count  = 1;
    name[0] = '\0';

    // read the names in 3www6google3com format
    while (*reader != 0) {
        if (*reader >= 192) {
            offset = (((*reader) & (~0xC0)) << 8) + *(reader + 1);
            reader = buffer + offset - 1;
            jumped = 1; // we have jumped to another location so counting wont
                        // go up!
        } else {
            name[p++] = *reader;
        }

        reader = reader + 1;

        if (jumped == 0) {
            *count = *count + 1; // if we havent jumped to another location
                                 // then we can count up
        }
    }

    name[p] = '\0'; // string complete
    if (jumped == 1) {
        *count = *count + 1;
        // number of steps we actually moved forward in the packet
    }

    // now convert 3www6google3com0 to www.google.com
    for (i = 0; i < (int)strlen((const char *)name); i++) {
        p = name[i];
        for (j = 0; j < (int)p; j++) {
            name[i] = name[i + 1];
            i       = i + 1;
        }
        name[i] = '.';
    }
    name[i - 1] = '\0'; // remove the last dot
    return name;
}
void read_info(unsigned char *query_buffer, int buffer_len) {
    unsigned char *    reader = &query_buffer[buffer_len];
    struct dns_header *dns    = (struct dns_header *)query_buffer;
    struct res_record  answers[20];
    int                i, j, stop = 0;
    switch (dns->rcode) {
    case NOERROR: break;
    case FORMERR: {
        printf("Error in format :(\n");
        return;
    }
    case SERVFAIL: {
        printf("Server failed to complete request :(\n");
        return;
    }
    case NXDOMAIN: {
        printf("Domain name does not exist :(\n");
        return;
    }
    case NOTIMP: {
        printf("function not implemented :(\n");
        return;
    }
    case REFUSED: {
        printf("Server refused to answer :(\n");
        return;
    }
    case YXDOMAIN: {
        printf("Name should not exist, but exists :(\n");
        break;
    }
    case XRRSET: {
        printf("Something about RRset I dont completely understand :(\n");
        return;
    }
    case NOTAUTH: {
        printf("Server not authoritative for the zone :(\n");
        return;
    }
    case NOTZONE: {
        printf("Name not in zone :(\n");
        return;
    }
    }
    for (i = 0; i < ntohs(dns->ans_count); i++) {
        answers[i].name =
                read_name(reader, (unsigned char *)query_buffer, &stop);
        reader += stop;

        answers[i].resource = (struct res_data *)(reader);
        reader += sizeof(struct res_data);
        if (ntohs(answers[i].resource->type) == A
            || ntohs(answers[i].resource->type) == AAAA) {
            answers[i].rdata = (unsigned char *)malloc(
                    ntohs(answers[i].resource->rdlength));

            for (j = 0; j < ntohs(answers[i].resource->rdlength); j++) {
                answers[i].rdata[j] = reader[j];
            }

            answers[i].rdata[ntohs(answers[i].resource->rdlength)] = '\0';

            reader += ntohs(answers[i].resource->rdlength);
        } else if (ntohs(answers[i].resource->type) == MX) {
            reader += 2;
            answers[i].rdata = read_name(reader, query_buffer, &stop);
            reader += stop;
        } else {
            answers[i].rdata = read_name(reader, query_buffer, &stop);
            reader += stop;
        }
    }
    // printf("Storing done\n");
    for (i = 0; i < ntohs(dns->ans_count); i++) {
        switch (ntohs(answers[i].resource->type)) {
        case A: {
            char           addr[INET_ADDRSTRLEN];
            struct in_addr s;
            memcpy(&s, answers[i].rdata, 4);
            inet_ntop(AF_INET, &s, addr, INET_ADDRSTRLEN);

            printf("IPv4 for %s : %s\n", answers[i].name, addr);
            break;
        }
        case AAAA: {
            char            addr[INET6_ADDRSTRLEN];
            struct in6_addr s;
            memcpy(&s, answers[i].rdata, 16);
            inet_ntop(AF_INET6, &s, addr, INET6_ADDRSTRLEN);

            printf("IPv6 for %s : %s\n", answers[i].name, addr);
            break;
        }
        case CNAME: {
            printf("%s CNAME : %s\n", answers[i].name, answers[i].rdata);
            break;
        }
        case MX: {
            printf("%s MX : %s\n", answers[i].name, answers[i].rdata);
            break;
        }
        case SOA: {
            printf("%s PRIMARY NAME SERVER : %s\n",
                   answers[i].name,
                   answers[i].rdata);
            break;
        }
        case NS: {
            printf("%s NAME SERVER : %s\n", answers[i].name, answers[i].rdata);
            break;
        }

        default: {
            printf("This response type not supported yet :(\n");
        }
        }
    }
    for (i = 0; i < ntohs(dns->ans_count); i++) {
        free(answers[i].name);
        free(answers[i].rdata);
    }
}
int get_type(char *query_type) {
    if (!strcmp(query_type, "A")) return A;
    if (!strcmp(query_type, "AAAA")) return AAAA;
    if (!strcmp(query_type, "MX")) return MX;
    if (!strcmp(query_type, "CNAME")) return CNAME;
    if (!strcmp(query_type, "NS")) return NS;
    // Below dont work
    if (!strcmp(query_type, "SOA")) return SOA;
    if (!strcmp(query_type, "WKS")) return WKS;
    if (!strcmp(query_type, "MINFO")) return MINFO;
    if (!strcmp(query_type, "TXT")) return TXT;
    return -1;
}

struct res_record
get_answer(unsigned char *query_buffer, int buffer_len, int type) {
    unsigned char *    reader = &query_buffer[buffer_len];
    struct dns_header *dns    = (struct dns_header *)query_buffer;
    struct res_record  answers[20];
    int                i, j, stop = 0, k = 0;

    for (i = 0; i < ntohs(dns->ans_count); i++) {
        answers[i].name = read_name(reader, query_buffer, &stop);
        reader += stop;

        answers[i].resource = (struct res_data *)(reader);
        reader += sizeof(struct res_data);

        if (ntohs(answers[i].resource->type) == A) {
            if (ntohs(answers[i].resource->type) == type) k = i;
            answers[i].rdata = (unsigned char *)malloc(
                    ntohs(answers[i].resource->rdlength));

            for (j = 0; j < ntohs(answers[i].resource->rdlength); j++) {
                answers[i].rdata[j] = reader[j];
            }

            answers[i].rdata[ntohs(answers[i].resource->rdlength)] = '\0';

            reader += ntohs(answers[i].resource->rdlength);
        } else {
            answers[i].rdata = read_name(reader, query_buffer, &stop);
            reader += stop;
        }
    }
    return answers[k];
}

struct sockaddr_in get_ns(char *hostname, struct sockaddr_in dns_server) {
    int                sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    server_addr = dns_server;
    struct dns_query  q;
    int               len = sizeof(server_addr);
    unsigned char     answer_query_buffer[DNS_QUERY_BUFFER_SIZE];
    struct res_record answer;
    q = make_dns_query(hostname, NS);
    if (sendto(sock,
               q.query,
               q.len,
               0,
               (struct sockaddr *)&server_addr,
               sizeof(server_addr))
        < 0) {
        printf("SEND FAIL\n");
    }
    if (recvfrom(sock,
                 answer_query_buffer,
                 DNS_QUERY_BUFFER_SIZE,
                 0,
                 (struct sockaddr *)&server_addr,
                 (socklen_t *)(&len))
        < 0)
        printf("Recieve fail\n");
    answer = get_answer(answer_query_buffer, q.len, NS);
    printf("%s\n", answer.rdata);

    bzero(answer_query_buffer, DNS_QUERY_BUFFER_SIZE);
    q = make_dns_query((char *)answer.rdata, A);
    if (sendto(sock,
               q.query,
               q.len,
               0,
               (struct sockaddr *)&server_addr,
               sizeof(server_addr))
        < 0) {
        printf("SEND FAIL\n");
    }
    if (recvfrom(sock,
                 answer_query_buffer,
                 DNS_QUERY_BUFFER_SIZE,
                 0,
                 (struct sockaddr *)&server_addr,
                 (socklen_t *)(&len))
        < 0)
        printf("Recieve fail\n");
    free(answer.name);
    free(answer.rdata);

    answer                     = get_answer(answer_query_buffer, q.len, A);
    dns_server.sin_addr.s_addr = (long)answer.rdata;
    free(answer.name);
    free(answer.rdata);
    close(sock);
    return dns_server;
}

void get_info_iterate(char *             hostname,
                      struct sockaddr_in dns_server,
                      char *             query_type) {
    strcat(hostname, ".");
    char *temp = hostname + strlen(hostname) - 2;
    while (strcmp(temp, hostname)) {
        if (*temp == '.') dns_server = get_ns(temp + 1, dns_server);
        temp--;
    }
    get_info(hostname, dns_server, query_type);
}
void get_info(char *             hostname,
              struct sockaddr_in dns_server,
              char *             query_type) {
    int                sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    server_addr = dns_server;
    struct dns_query q;
    int              len = sizeof(server_addr);
    unsigned char    answer_query_buffer[DNS_QUERY_BUFFER_SIZE];
    int              type = get_type(query_type);
    if (type < 0) {
        printf("Invlaid query type :(\n");
        return;
    }
    q = make_dns_query(hostname, type);
    if (sendto(sock,
               q.query,
               q.len,
               0,
               (struct sockaddr *)&server_addr,
               sizeof(server_addr))
        < 0) {
        printf("SEND FAIL\n");
    } else {
        // printf("SEND SUCESS\n");
    }
    if (recvfrom(sock,
                 answer_query_buffer,
                 DNS_QUERY_BUFFER_SIZE,
                 0,
                 (struct sockaddr *)&server_addr,
                 (socklen_t *)(&len))
        < 0)
        printf("Recieve fail\n");
    else {
        // printf("Recieve success\n");
    }
    read_info(answer_query_buffer, q.len);

    printf("\n\n");
    close(sock);
}
