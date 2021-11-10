#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
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


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "embed/list.h"
#include "embed/rbtree.h"
#include "embed/xgetopt.h"


typedef struct {
    unsigned short id;         // identification number
    unsigned char  rd : 1;     // recursion desired
    unsigned char  tc : 1;     // truncated message
    unsigned char  aa : 1;     // authoritive answer
    unsigned char  opcode : 4; // purpose of message
    unsigned char  qr : 1;     // query/response flag
    unsigned char  rcode : 4;  // response code
    unsigned char  cd : 1;     // checking disabled
    unsigned char  ad : 1;     // authenticated data
    unsigned char  z : 1;      // its z! reserved
    unsigned char  ra : 1;     // recursion available
    unsigned short qd_count;   // number of question entries
    unsigned short an_count;   // number of answer entries
    unsigned short ns_count;   // number of authority entries
    unsigned short nr_count;   // number of resource entries
} DNS_HDR;

typedef struct {
    unsigned short type;
    unsigned short classes;
} DNS_QDS;

typedef struct {
    unsigned short type;
    unsigned short classes;
    unsigned int   ttl;
    unsigned short rd_length;
    char           rd_data[0];
} __attribute__((packed)) DNS_RRS;


#define MIN_TTL 10
#define MAX_TTL (30 * 60)

typedef struct {
    struct rbnode rb_name;
    union {
        struct {
            char *           prefix;
            int              p_length;
            int              d_same;
            struct list_head lh_name;
        };
        struct {
            time_t        expire;
            struct rbnode rb_expire;
        };
    };
    time_t         timestamp;
    char *         domain;
    int            d_length;
    char *         answer;
    unsigned short an_count;
    unsigned short an_length;
    char           buffer[0];
} DOMAIN_CACHE;

void          domain_cache_init(const char *file);
DOMAIN_CACHE *domain_cache_search(char *domain);
void          domain_cache_append(char *         domain,
                                  int            d_length,
                                  unsigned int   ttl,
                                  unsigned short an_count,
                                  unsigned short an_length,
                                  char *         answer);
void          domain_cache_clean(time_t current);

typedef struct {
    struct rbnode      rb_new;
    struct list_head   list;
    void *             context;
    time_t             expire;
    unsigned short     new_id;
    unsigned short     old_id;
    struct sockaddr_in source;
} TRANSPORT_CACHE;

void             transport_cache_init(unsigned short timeout);
TRANSPORT_CACHE *transport_cache_search(unsigned short new_id);
TRANSPORT_CACHE *transport_cache_insert(unsigned short      old_id,
                                        struct sockaddr_in *address,
                                        void *              context);
void             transport_cache_delete(TRANSPORT_CACHE *cache);
void             transport_cache_clean(time_t current);
