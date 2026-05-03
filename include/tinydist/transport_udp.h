#ifndef TINYDIST_TRANSPORT_UDP_H_
#define TINYDIST_TRANSPORT_UDP_H_

#include "tinydist/protocol.h"
#include <netinet/in.h>
#include <sys/socket.h>

// context for the udp transport backend
typedef struct {
    int sockfd;
    struct sockaddr_storage remote_addr;
    socklen_t remote_addr_len;
} tinydist_udp_ctx_t;

// open a udp socket aimed at remote_ip:port, sender side
// returns 0 on success, -1 on error (errno set)
int tinydist_udp_sender(tinydist_udp_ctx_t *ctx, const char *remote_ip, uint16_t port);

// open and bind a udp socket to port, receiver side
// returns 0 on success, -1 on error (errno set)
int tinydist_udp_receiver(tinydist_udp_ctx_t *ctx, uint16_t port);

// fill a transport backed by ctx
void tinydist_udp_transport(tinydist_transport_t *t, tinydist_udp_ctx_t *ctx);

#endif // TINYDIST_TRANSPORT_UDP_H_
