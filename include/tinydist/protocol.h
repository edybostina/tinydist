#ifndef TINYDIST_PROTOCOL_H_
#define TINYDIST_PROTOCOL_H_

#include <stddef.h>
#include <stdint.h>

#define TINYDIST_MAGIC   0xD157
#define TINYDIST_VERSION 2

#define TINYDIST_TYPE_ACK      0x0
#define TINYDIST_TYPE_DATA     0x1
#define TINYDIST_TYPE_METADATA 0x2
#define TINYDIST_TYPE_ERR      0x3
#define TINYDIST_TYPE_END      0x4
#define TINYDIST_TYPE_NACK     0x5

// max fragments tracked by the dedup bitmap in tensor_recv
#define TINYDIST_MAX_FRAGS 256

#define TINYDIST_DTYPE_FLOAT32 0x1
#define TINYDIST_DTYPE_INT8    0x2

typedef uint8_t tinydist_msg_type_t;
typedef uint8_t tinydist_data_type_t;

typedef enum {
    TINYDIST_OK = 0,
    TINYDIST_ERR_SOCKET = -1,
    TINYDIST_ERR_CRC = -2,
    TINYDIST_ERR_MAGIC = -3,
    TINYDIST_ERR_OVERFLOW = -4,
    TINYDIST_ERR_TIMEOUT = -5,
    TINYDIST_ERR_PARAM = -6,
} tinydist_err_t;

typedef struct packet_hdr {
    uint16_t magic;
    uint8_t version;
    tinydist_msg_type_t type;
    uint16_t session_id;
    uint32_t seq_num;
    uint32_t payload_len;
    uint32_t checksum;
} __attribute__((packed)) packet_hdr_t;

void packet_hdr_serialize(const packet_hdr_t *hdr, uint8_t *buf);
void packet_hdr_deserialize(packet_hdr_t *hdr, const uint8_t *buf);

typedef struct tensor_meta {
    uint32_t dims[4];
    uint8_t ndims;
    tinydist_data_type_t data_type;
    uint32_t total_bytes;
    uint32_t fragment_count;
} __attribute__((packed)) tensor_meta_t;

typedef struct tensor_fragment {
    uint32_t offset;
} __attribute__((packed)) tensor_fragment_t;

// payload for TYPE_END; carries a crc over the full reassembled tensor
typedef struct {
    uint32_t full_crc;
} __attribute__((packed)) tensor_end_t;

void tensor_meta_serialize(const tensor_meta_t *meta, uint8_t *buf);
void tensor_meta_deserialize(tensor_meta_t *meta, const uint8_t *buf);
void tensor_fragment_serialize(const tensor_fragment_t *frag, uint8_t *buf);
void tensor_fragment_deserialize(tensor_fragment_t *frag, const uint8_t *buf);
void tensor_end_serialize(const tensor_end_t *end, uint8_t *buf);
void tensor_end_deserialize(tensor_end_t *end, const uint8_t *buf);

typedef struct {
    int (*send_fn)(void *ctx, const uint8_t *buf, size_t len);
    int (*recv_fn)(void *ctx, uint8_t *buf, size_t max_len);
    int (*set_timeout_fn)(void *ctx, uint32_t ms); // may be NULL
    size_t max_payload; // max data bytes per fragment, not counting headers
    void *ctx;
} tinydist_transport_t;

// per-stream state; call tinydist_session_init() before use
typedef struct {
    uint16_t next_session_id;
    uint32_t next_seq;
    uint32_t retransmit_timeout_ms;
    uint8_t max_retries;
} tinydist_session_t;

void tinydist_session_init(tinydist_session_t *s);

// max fragment payload for wifi udp (1472 byte mtu - headers)
// other transports set their own limit via tinydist_transport_t.max_payload
#define TINYDIST_UDP_MAX_PAYLOAD \
    (1472u - (uint32_t)sizeof(packet_hdr_t) - (uint32_t)sizeof(tensor_fragment_t))

// upper bound for stack packet buffers; sized for wifi udp, the largest transport
#define TINYDIST_MAX_PACKET_SIZE                                          \
    ((uint32_t)sizeof(packet_hdr_t) + (uint32_t)sizeof(tensor_fragment_t) \
     + TINYDIST_UDP_MAX_PAYLOAD)

// split data into fragments and send via transport. returns total bytes sent, or negative
// tinydist_err_t on error
int tensor_send(tinydist_transport_t *t, tinydist_session_t *s, const void *data,
                size_t total_bytes, tinydist_data_type_t data_type, const uint32_t *dims,
                uint8_t ndims);

// receive and reassemble fragments into dest_buf. returns total bytes written, or negative
// tinydist_err_t on error
int tensor_recv(tinydist_transport_t *t, tinydist_session_t *s, void *dest_buf,
                uint32_t dest_buf_len, tinydist_data_type_t *data_type);

#endif // TINYDIST_PROTOCOL_H_
