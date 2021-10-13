#include "dns.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// convert www.google.com to 3www6google3com
void ChangetoDnsNameFormat(unsigned char *dns, unsigned char *host) {
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
struct dns_query make_dns_query(char *hostname, int type, int class) {
    // 6 layers of header each of 2 bytes
    int                i;
    unsigned char *    qname;
    struct dns_header *dns   = NULL;
    struct question *  qinfo = NULL;
    unsigned char      buffer[DNS_QUERY_BUFFER_SIZE];
    // Set the DNS structure to standard queries
    dns = (struct dns_header *)&buffer;

    dns->id         = (unsigned short)htons(rand()); // Instead of random
    dns->qr         = 0;                             // This is a query
    dns->opcode     = 0; // This is a standard query
    dns->aa         = 0; // Not Authoritative
    dns->tc         = 0; // This message is not truncated
    dns->rd         = 1; // Recursion Desired
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

    ChangetoDnsNameFormat(qname, (unsigned char *)hostname);
    qinfo = (struct question
                     *)&buffer[sizeof(struct dns_header)
                               + (strlen((const char *)qname) + 1)]; // fill it

    qinfo->qtype  = htons(type); // type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(class); // its internet (lol)

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
            offset = (*reader) * 256 + *(reader + 1)
                     - 49152; // 49152 = 11000000 00000000 ;)
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
        *count =
                *count
                + 1; // number of steps we actually moved forward in the packet
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
    int                i, j, stop = 0, k = 0;

    for (i = 0; i < ntohs(dns->ans_count); i++) {
        answers[i].name =
                read_name(reader, (unsigned char *)query_buffer, &stop);
        reader += stop;

        answers[i].resource = (struct res_data *)(reader);
        reader += sizeof(struct res_data);

        if (ntohs(answers[i].resource->type)) {
            // if (ntohs(answers[i].resource->type) == A) k = i;
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
    printf("Storing done\n");
    for (i = 0; i < ntohs(dns->ans_count); i++) {
        switch (ntohs(answers[i].resource->type)) {
        case A: {
            struct sockaddr_in a;
            long *             p;
            p                 = (long *)answers[k].rdata;
            a.sin_addr.s_addr = (*p);
            printf("IPv4 : %s\n", inet_ntoa(a.sin_addr));
            break;
        }
        case AAAA: {
            struct sockaddr_in a;
            double long *      p;
            p                 = (double long *)answers[i].rdata;
            a.sin_addr.s_addr = (*p);
            printf("IPv6 : %s\n", inet_ntoa(a.sin_addr));
            break;
        }
        case CNAME: {
            printf("%s CNAME : %s\n", answers[i].name, answers[i].rdata);
        }
        }
    }
    //    p                 = (long *)answers[k].rdata;
    //    a.sin_addr.s_addr = (*p);
    //  strcpy(ip_buffer, inet_ntoa(a.sin_addr));
    for (i = 0; i < ntohs(dns->ans_count); i++) {
        free(answers[i].name);
        free(answers[i].rdata);
    }
}
void get_info(char hostname[], char dns_server[]) {
    int                sock = socket(AF_INET, SOCK_DGRAM, 0);
    char               ip_buffer[15];
    struct sockaddr_in server_addr;
    server_addr.sin_family      = AF_INET;
    server_addr.sin_port        = htons(53);
    server_addr.sin_addr.s_addr = inet_addr(dns_server);
    struct dns_query q;
    int              len = sizeof(server_addr);
    unsigned char    answer_query_buffer[DNS_QUERY_BUFFER_SIZE];

    q = make_dns_query(hostname, 1, A);
    if (sendto(sock,
               q.query,
               q.len,
               0,
               (struct sockaddr *)&server_addr,
               sizeof(server_addr))
        < 0) {
        printf("SEND FAIL\n");
    } else
        printf("SEND SUCESS\n");
    if (recvfrom(sock,
                 answer_query_buffer,
                 DNS_QUERY_BUFFER_SIZE,
                 0,
                 (struct sockaddr *)&server_addr,
                 (socklen_t *)(&len))
        < 0)
        printf("Recieve fail\n");
    else
        printf("Recieve success\n");
    read_info(answer_query_buffer, q.len);

    q = make_dns_query(hostname, 1, AAAA);
    if (sendto(sock,
               q.query,
               q.len,
               0,
               (struct sockaddr *)&server_addr,
               sizeof(server_addr))
        < 0) {
        printf("SEND FAIL\n");
    } else
        printf("SEND SUCESS\n");
    if (recvfrom(sock,
                 answer_query_buffer,
                 DNS_QUERY_BUFFER_SIZE,
                 0,
                 (struct sockaddr *)&server_addr,
                 (socklen_t *)(&len))
        < 0)
        printf("Recieve fail\n");
    else
        printf("Recieve success\n");
    read_info(answer_query_buffer, q.len);

    close(sock);
}
