#ifndef DNS_H
#define DNS_H

#define DNS_HEADER_SIZE sizeof(unsigned short) * 6
#define DNS_QUERY_BUFFER_SIZE sizeof(unsigned char) * 655335
#define MAX_IP_LEN 15

// Necessary structures and methods for making DNS query and resolving the
// response

struct dns_header {
    unsigned short id; // identification number

    unsigned char rd : 1;     // recursion desired
    unsigned char tc : 1;     // truncated message
    unsigned char aa : 1;     // authoritive answer
    unsigned char opcode : 4; // purpose of message
    unsigned char qr : 1;     // query/response flag

    unsigned char rcode : 4; // response code
    unsigned char cd : 1;    // checking disabled
    unsigned char ad : 1;    // authenticated data
    unsigned char z : 1;     // its z! reserved
    unsigned char ra : 1;    // recursion available

    unsigned short q_count;    // number of question entries
    unsigned short ans_count;  // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count;  // number of resource entries
};
struct dns_query {
    unsigned char *query;
    int            len;
};
struct res_data {
    unsigned short type;
    unsigned short class;
    unsigned int   ttl;
    unsigned short data_len;
} __attribute__((packed));
struct res_record {
    unsigned char *  name;
    struct res_data *resource;
    unsigned char *  rdata;
};
struct question {
    unsigned short qtype;
    unsigned short qclass;
};
void get_ip_from_name(char *hostname, char dns_server[], char *ip_buffer);
#endif
