#include "dns_server.h"
#include "asciilogo.h"

#define PACKAGE_SIZE 8192
#define CACHE_CLEAN_TIME (MIN_TTL / 2 + 1)

typedef struct {
    SOCKET sock;
    char   buffer[PACKAGE_SIZE + sizeof(unsigned short)];
} LOCAL_DNS;

typedef struct {
    int                tcp;
    SOCKET             sock;
    struct sockaddr_in addr;
    unsigned int       head;
    unsigned int       rear;
    unsigned int       capacity;
    char               buffer[PACKAGE_SIZE * 3];
} REMOTE_DNS;

typedef struct {
    LOCAL_DNS  local;
    REMOTE_DNS remote;
} SERVER_ENGINE;

static const int enable        = 1;
static int       disable_cache = 0;

static void process_query(SERVER_ENGINE *engine) {
    LOCAL_DNS *        ldns;
    REMOTE_DNS *       rdns;
    DNS_HDR *          hdr, *rhdr;
    DNS_QDS *          qds;
    DNS_RRS *          rrs;
    DOMAIN_CACHE *     dcache;
    TRANSPORT_CACHE *  tcache;
    socklen_t          addrlen;
    struct sockaddr_in source;
    char *             pos, *head, *rear;
    char *             buffer, domain[PACKAGE_SIZE], rbuffer[PACKAGE_SIZE];
    int                size, dlen;
    time_t             current;
    unsigned char      qlen;
    unsigned int       ttl, ttl_tmp;
    unsigned short     index, q_len;

    ldns   = &engine->local;
    rdns   = &engine->remote;
    buffer = ldns->buffer + sizeof(unsigned short);

    addrlen = sizeof(struct sockaddr_in);
    size    = recvfrom(ldns->sock,
                    buffer,
                    PACKAGE_SIZE,
                    0,
                    (struct sockaddr *)&source,
                    &addrlen);
    if (size <= (int)sizeof(DNS_HDR)) return;

    hdr  = (DNS_HDR *)buffer;
    rhdr = (DNS_HDR *)rbuffer;
    memset(rbuffer, 0, sizeof(DNS_HDR));

    rhdr->id = hdr->id;
    rhdr->qr = 1;
    q_len    = 0;
    qds      = NULL;
    head     = buffer + sizeof(DNS_HDR);
    rear     = buffer + size;
    if (hdr->qr != 0 || hdr->tc != 0 || ntohs(hdr->qd_count) != 1)
        rhdr->rcode = 1;
    else {
        dlen = 0;
        pos  = head;
        while (pos < rear) {
            qlen = (unsigned char)*pos++;
            if (qlen > 63 || (pos + qlen) > (rear - sizeof(DNS_QDS))) {
                rhdr->rcode = 1;
                break;
            }
            if (qlen > 0) {
                if (dlen > 0) domain[dlen++] = '.';
                while (qlen-- > 0)
                    domain[dlen++] = (char)tolower(*pos++);
            } else {
                qds = (DNS_QDS *)pos;
                if (ntohs(qds->classes) != 0x01)
                    rhdr->rcode = 4;
                else {
                    pos += sizeof(DNS_QDS);
                    q_len = pos - head;
                }
                break;
            }
        }
        domain[dlen] = '\0';
    }
    if (hdr->rd == 0) {
        char temp[PACKAGE_SIZE];
        strcpy(temp, domain);
        strcat(temp, ".");
        int len = strlen(temp);
        int i   = len - 2;
        while (i) {
            if (temp[i] != '.')
                i--;
            else {
                dcache = domain_cache_search(temp + i + 1);
                if (dcache) {}
            }
        }
    }
    if (rhdr->rcode == 0 && ntohs(qds->type) == 0x01) {
        dcache = domain_cache_search(domain);
        if (dcache) {
            rhdr->qd_count = htons(1);
            rhdr->an_count = htons(dcache->an_count);
            pos            = rbuffer + sizeof(DNS_HDR);
            memcpy(pos, head, q_len);
            pos += q_len;
            memcpy(pos, dcache->answer, dcache->an_length);
            rear = pos + dcache->an_length;
            if (dcache->expire > 0) {
                if (time(&current) <= dcache->timestamp)
                    ttl = 1;
                else
                    ttl = (unsigned int)(current - dcache->timestamp);
                index = 0;
                while (pos < rear && index++ < dcache->an_count) {
                    rrs = NULL;
                    if ((unsigned char)*pos == 0xc0) {
                        pos += 2;
                        rrs = (DNS_RRS *)pos;
                    } else {
                        while (pos < rear) {
                            qlen = (unsigned char)*pos++;
                            if (qlen > 0)
                                pos += qlen;
                            else {
                                rrs = (DNS_RRS *)pos;
                                break;
                            }
                        }
                    }
                    ttl_tmp = ntohl(rrs->ttl);
                    if (ttl_tmp <= ttl)
                        ttl_tmp = 1;
                    else
                        ttl_tmp -= ttl;
                    rrs->ttl = htonl(ttl_tmp);
                    pos += sizeof(DNS_RRS) + ntohs(rrs->rd_length);
                }
            }
            sendto(ldns->sock,
                   rbuffer,
                   rear - rbuffer,
                   0,
                   (struct sockaddr *)&source,
                   sizeof(struct sockaddr_in));
            return;
        }
    }

    if (rhdr->rcode == 0) {
        tcache = transport_cache_insert(ntohs(hdr->id), &source, ldns);
        if (tcache == NULL)
            rhdr->rcode = 2;
        else {
            hdr->id = htons(tcache->new_id);
            if (!rdns->tcp) {
                if (sendto(rdns->sock,
                           buffer,
                           size,
                           0,
                           (struct sockaddr *)&rdns->addr,
                           sizeof(struct sockaddr_in))
                    != size)
                    rhdr->rcode = 2;
            } else {
                if (rdns->sock == INVALID_SOCKET) {
                    rdns->head = 0;
                    rdns->rear = 0;
                    rdns->sock = socket(AF_INET, SOCK_STREAM, 0);
                    if (rdns->sock != INVALID_SOCKET) {
                        setsockopt(rdns->sock,
                                   IPPROTO_TCP,
                                   TCP_NODELAY,
                                   (void *)&enable,
                                   sizeof(enable));
                        if (connect(rdns->sock,
                                    (struct sockaddr *)&rdns->addr,
                                    sizeof(struct sockaddr_in))
                            != 0) {
                            closesocket(rdns->sock);
                            rdns->sock = INVALID_SOCKET;
                        }
                    }
                }
                if (rdns->sock == INVALID_SOCKET)
                    rhdr->rcode = 2;
                else {
                    pos                    = ldns->buffer;
                    *(unsigned short *)pos = htons((unsigned short)size);
                    size += sizeof(unsigned short);
                    if (send(rdns->sock, ldns->buffer, size, 0) != size) {
                        rdns->head = 0;
                        rdns->rear = 0;
                        closesocket(rdns->sock);
                        rdns->sock  = INVALID_SOCKET;
                        rhdr->rcode = 2;
                    }
                }
            }
            if (rhdr->rcode != 0) transport_cache_delete(tcache);
        }
    }
    if (rhdr->rcode != 0)
        sendto(ldns->sock,
               rbuffer,
               sizeof(DNS_HDR),
               0,
               (struct sockaddr *)&source,
               sizeof(struct sockaddr_in));
}

static void process_response(char *buffer, int size) {
    DNS_HDR *        hdr;
    DNS_QDS *        qds;
    DNS_RRS *        rrs;
    LOCAL_DNS *      ldns;
    TRANSPORT_CACHE *cache;
    char             domain[PACKAGE_SIZE];
    char *           pos, *rear, *answer;
    int              badfmt, dlen, length;
    unsigned char    qlen;
    unsigned int     ttl, ttl_tmp;
    unsigned short   index, an_count;

    length   = size;
    hdr      = (DNS_HDR *)buffer;
    an_count = ntohs(hdr->an_count);
    if (hdr->qr == 1 && hdr->tc == 0 && ntohs(hdr->qd_count) == 1
        && an_count > 0) {
        dlen = 0;
        qds  = NULL;
        pos  = buffer + sizeof(DNS_HDR);
        rear = buffer + size;
        while (pos < rear) {
            qlen = (unsigned char)*pos++;
            if (qlen > 63 || (pos + qlen) > (rear - sizeof(DNS_QDS))) break;
            if (qlen > 0) {
                if (dlen > 0) domain[dlen++] = '.';
                while (qlen-- > 0)
                    domain[dlen++] = (char)tolower(*pos++);
            } else {
                qds = (DNS_QDS *)pos;
                if (ntohs(qds->classes) != 0x01)
                    qds = NULL;
                else
                    pos += sizeof(DNS_QDS);
                break;
            }
        }
        domain[dlen] = '\0';

        if (qds && ntohs(qds->type) == 0x01) {
            ttl    = MAX_TTL;
            index  = 0;
            badfmt = 0;
            answer = pos;
            while (badfmt == 0 && pos < rear && index++ < an_count) {
                rrs = NULL;
                if ((unsigned char)*pos == 0xc0) {
                    pos += 2;
                    rrs = (DNS_RRS *)pos;
                } else {
                    while (pos < rear) {
                        qlen = (unsigned char)*pos++;
                        if (qlen > 63
                            || (pos + qlen) > (rear - sizeof(DNS_RRS)))
                            break;
                        if (qlen > 0)
                            pos += qlen;
                        else {
                            rrs = (DNS_RRS *)pos;
                            break;
                        }
                    }
                }
                if (rrs == NULL || ntohs(rrs->classes) != 0x01)
                    badfmt = 1;
                else {
                    ttl_tmp = ntohl(rrs->ttl);
                    if (ttl_tmp < ttl) ttl = ttl_tmp;
                    pos += sizeof(DNS_RRS) + ntohs(rrs->rd_length);
                }
            }
            if (badfmt == 0) {
                hdr->nr_count = 0;
                hdr->ns_count = 0;
                length        = pos - buffer;
                if (!disable_cache)
                    domain_cache_append(
                            domain, dlen, ttl, an_count, pos - answer, answer);
            }
        }
    }

    cache = transport_cache_search(ntohs(hdr->id));
    if (cache) {
        ldns    = (LOCAL_DNS *)cache->context;
        hdr->id = htons(cache->old_id);
        sendto(ldns->sock,
               buffer,
               length,
               0,
               (struct sockaddr *)&cache->source,
               sizeof(struct sockaddr_in));
        transport_cache_delete(cache);
    }
}

static void process_response_udp(REMOTE_DNS *rdns) {
    int                size;
    socklen_t          addrlen;
    struct sockaddr_in source;

    addrlen = sizeof(struct sockaddr_in);
    size    = recvfrom(rdns->sock,
                    rdns->buffer,
                    PACKAGE_SIZE,
                    0,
                    (struct sockaddr *)&source,
                    &addrlen);
    if (size < (int)sizeof(DNS_HDR)) return;

    if (source.sin_addr.s_addr != rdns->addr.sin_addr.s_addr) return;

    process_response(rdns->buffer, size);
}

static void process_response_tcp(REMOTE_DNS *rdns) {
    char *       pos;
    int          to_down, size;
    unsigned int len, buflen;

    to_down = 0;
    size    = recv(rdns->sock,
                rdns->buffer + rdns->rear,
                rdns->capacity - rdns->rear,
                0);
    if (size < 1)
        to_down = 1;
    else {
        rdns->rear += size;
        while ((buflen = rdns->rear - rdns->head) > sizeof(unsigned short)) {
            pos = rdns->buffer + rdns->head;
            len = ntohs(*(unsigned short *)pos);
            if (len > PACKAGE_SIZE) {
                to_down = 1;
                break;
            }
            if (len + sizeof(unsigned short) > buflen) break;
            process_response(pos + sizeof(unsigned short), len);
            rdns->head += len + sizeof(unsigned short);
        }
        if (!to_down) {
            if (rdns->head == rdns->rear) {
                rdns->head = 0;
                rdns->rear = 0;
            } else if (rdns->head > PACKAGE_SIZE) {
                len = rdns->rear - rdns->head;
                memmove(rdns->buffer, rdns->buffer + rdns->head, len);
                rdns->head = 0;
                rdns->rear = len;
            }
        }
    }

    if (to_down) {
        rdns->head = 0;
        rdns->rear = 0;
        closesocket(rdns->sock);
        rdns->sock = INVALID_SOCKET;
    }
}

static SERVER_ENGINE g_engine;

static int dns_server(const char *   local_addr,
                      unsigned short local_port,
                      const char *   remote_addr,
                      unsigned short remote_port,
                      int            remote_tcp) {
    int                maxfd, fds;
    fd_set             readfds;
    struct timeval     timeout;
    struct sockaddr_in addr;
    time_t             current, last_clean;
    SERVER_ENGINE *    engine = &g_engine;
    LOCAL_DNS *        ldns   = &engine->local;
    REMOTE_DNS *       rdns   = &engine->remote;

    ldns->sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (ldns->sock == INVALID_SOCKET) {
        perror("create socket");
        return -1;
    }
    setsockopt(ldns->sock,
               SOL_SOCKET,
               SO_REUSEADDR,
               (char *)&enable,
               sizeof(enable));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr(local_addr);
    addr.sin_port        = htons(local_port);
    if (bind(ldns->sock, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind service port");
        return -1;
    }
    rdns->tcp = remote_tcp;
    //    rdns->sock                 = INVALID_SOCKET;
    rdns->addr.sin_family      = AF_INET;
    rdns->addr.sin_addr.s_addr = inet_addr(remote_addr);
    rdns->addr.sin_port        = htons(remote_port);
    rdns->head                 = 0;
    rdns->rear                 = 0;
    rdns->capacity             = sizeof(rdns->buffer);
    if (!rdns->tcp) {
        rdns->sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (rdns->sock == INVALID_SOCKET) {
            perror("create socket");
            return -1;
        }
    }
    last_clean = time(&current);
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(ldns->sock, &readfds);
        maxfd = (int)ldns->sock;
        if (rdns->sock != INVALID_SOCKET) {
            FD_SET(rdns->sock, &readfds);
            if (maxfd < (int)rdns->sock) maxfd = (int)rdns->sock;
        }
        timeout.tv_sec  = CACHE_CLEAN_TIME;
        timeout.tv_usec = 0;
        fds             = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        if (fds > 0) {
            if (rdns->sock != INVALID_SOCKET
                && FD_ISSET(rdns->sock, &readfds)) {
                if (rdns->tcp)
                    process_response_tcp(rdns);
                else
                    process_response_udp(rdns);
            }
            if (FD_ISSET(ldns->sock, &readfds)) process_query(engine);
        }
        if (time(&current) - last_clean > CACHE_CLEAN_TIME || fds == 0) {
            last_clean = current;
            domain_cache_clean(current);
            transport_cache_clean(current);
        }
    }
    return 0;
}

struct xoption options[] = {
        {'v', "version", xargument_no, NULL, -1},
        {'h', "help", xargument_no, NULL, -1},
        {'r', "local-addr", xargument_required, NULL, -1},
        {'p', "local-port", xargument_required, NULL, -1},
        {'T', "remote-tcp", xargument_no, NULL, -1},
        {'P', "remote-port", xargument_required, NULL, -1},
        {'R', "remote-addr", xargument_required, NULL, -1},
        {'f', "hosts-file", xargument_required, NULL, -1},
        {0, "disable-cache", xargument_no, &disable_cache, 1},
        {0, NULL, xargument_no, NULL, 0},
};

static void display_help() {
    printf("Usage: dns_server [options]\n"
           "  -r <ip> or --local-addr=<ip>\n"
           "                       (local server ip, default 127.0.0.1)\n"
           "  -p <port> or --loacl-port=<port>\n"
           "                       (local bind port, default 53)\n"
           "  -R <ip> or --remote-addr=<ip>\n"
           "                       (remote server ip, default 8.8.8.8)\n"
           "  -P <port> or --remote-port=<port>\n"
           "                       (remote server port, default 53)\n"
           "  -T or --remote-tcp\n"
           "  -f <file> or --hosts-file=<file>\n"
           "                       (connect remote server in tcp, default no)\n"
           "  -h, --help           (print help and exit)\n"
           "  -v, --version        (print version and exit)\n");
};

int main(int argc, const char *argv[]) {
    int            opt, optind;
    const char *   optarg;
    int            remote_tcp        = 0;
    int            transport_timeout = 5;
    const char *   hosts_file        = NULL;
    const char *   local_addr        = "127.0.0.1";
    const char *   remote_addr       = "8.8.8.8";
    unsigned short local_port = 53, remote_port = 53;

    optind = 0;
    opt    = xgetopt(argc, argv, options, &optind, &optarg);
    while (opt != -1) {
        switch (opt) {
        case 0: break;
        case 'p': local_port = (unsigned short)atoi(optarg); break;
        case 'P': remote_port = (unsigned short)atoi(optarg); break;
        case 'r': local_addr = optarg; break;
        case 'R': remote_addr = optarg; break;
        case 'T': remote_tcp = 1; break;
        case 'f': hosts_file = optarg; break;
        case 'v': printf("%s\n", ascii_logo); return 0;
        case 'h':
        default: display_help(); return -1;
        }
        opt = xgetopt(argc, argv, options, &optind, &optarg);
    }

    printf("%s"
           " * running at %s:%d,%s\n"
           " * transport to %s:%d,%s\n",
           ascii_logo,
           local_addr,
           local_port,
           disable_cache ? ", cache: off" : "",
           remote_addr,
           remote_port,
           remote_tcp ? "tcp" : "udp");
    fflush(stdout);

    srand((unsigned int)time(NULL));
    domain_cache_init(hosts_file);
    transport_cache_init(transport_timeout);
    return dns_server(
            local_addr, local_port, remote_addr, remote_port, remote_tcp);
}
