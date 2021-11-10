#ifndef DNS_H
#define DNS_H
#include <netinet/in.h>
#include <sys/types.h>
#define DNS_HEADER_SIZE sizeof(unsigned short) * 6
#define DNS_QUERY_BUFFER_SIZE sizeof(unsigned char) * 655335
#define MAX_IP_LEN 15

// Importnant macros
#define INTERNET 1 // QUERY CLASS = INTERNET : 1
// Query types
#define A 1      // a host address
#define NS 2     // an authoritative name server
#define CNAME 5  // the canonical name for an alias
#define SOA 6    // marks the start of a zone of authority
#define WKS 11   // a well known service description
#define PTR 12   // a domain name pointer
#define HINFO 13 // host information
#define MINFO 14 // mailbox or mail list information
#define MX 15    // mail exchange
#define TXT 16   // text strings
#define AAAA 28  // IPv6 address
// Response codes important ones
#define NOERROR 0  // DNS Query completed successfully
#define FORMERR 1  // DNS Query Format Error
#define SERVFAIL 2 // Server failed to complete the DNS request
#define NXDOMAIN 3 // Domain name does not exist.
#define NOTIMP 4   // Function not implemented
#define REFUSED 5  // The server refused to answer for the query
#define YXDOMAIN 6 // Name that should not exist, does exist
#define XRRSET 7   // RRset that should not exist, does exist
#define NOTAUTH 8  // Server not authoritative for the zone
#define NOTZONE 9  // Name not in zone

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
    unsigned short rdlength;
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
void get_info_recurse(char *             hostname,
                      struct sockaddr_in dns_server,
                      char *             query_type);
void get_info_iterate(char *             hostname,
                      struct sockaddr_in dns_server,
                      char *             query_type);
#endif
