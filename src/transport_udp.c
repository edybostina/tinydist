#include "tinydist/transport_udp.h"
#include <arpa/inet.h>
#include <string.h>
#include <sys/time.h>

static int udp_send_fn(void *ctx, const uint8_t *buf, size_t len)
{
    tinydist_udp_ctx_t *u = ctx;
    return (int)sendto(u->sockfd, buf, len, 0, (const struct sockaddr *)&u->remote_addr,
                       u->remote_addr_len);
}

static int udp_recv_fn(void *ctx, uint8_t *buf, size_t max_len)
{
    tinydist_udp_ctx_t *u = ctx;
    struct sockaddr_storage src;
    socklen_t src_len = sizeof(src);
    int r = (int)recvfrom(u->sockfd, buf, max_len, 0, (struct sockaddr *)&src, &src_len);
    if (r >= 0) {
        memcpy(&u->remote_addr, &src, src_len);
        u->remote_addr_len = src_len;
    }
    return r;
}

static int udp_set_timeout_fn(void *ctx, uint32_t ms)
{
    tinydist_udp_ctx_t *u = ctx;
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (long)(ms % 1000) * 1000;
    return setsockopt(u->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

int tinydist_udp_sender(tinydist_udp_ctx_t *ctx, const char *remote_ip, uint16_t port)
{
    ctx->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sockfd < 0)
        return -1;

    struct sockaddr_in *addr = (struct sockaddr_in *)&ctx->remote_addr;
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (inet_aton(remote_ip, &addr->sin_addr) == 0)
        return -1;

    ctx->remote_addr_len = sizeof(struct sockaddr_in);
    return 0;
}

int tinydist_udp_receiver(tinydist_udp_ctx_t *ctx, uint16_t port)
{
    ctx->sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (ctx->sockfd < 0)
        return -1;

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(ctx->sockfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0)
        return -1;

    memset(&ctx->remote_addr, 0, sizeof(ctx->remote_addr));
    ctx->remote_addr_len = 0;
    return 0;
}

void tinydist_udp_transport(tinydist_transport_t *t, tinydist_udp_ctx_t *ctx)
{
    t->send_fn = udp_send_fn;
    t->recv_fn = udp_recv_fn;
    t->set_timeout_fn = udp_set_timeout_fn;
    t->max_payload = TINYDIST_UDP_MAX_PAYLOAD;
    t->ctx = ctx;
}
