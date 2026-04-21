#ifndef TINYDIST_PROTOCOL_H_
#define TINYDIST_PROTOCOL_H_

#include <stdint.h>
#include <sys/socket.h>

#define PACKET_MAGIC 0xD157

#define PACKET_TYPE_ACK      0x0
#define PACKET_TYPE_DATA     0x1
#define PACKET_TYPE_METADATA 0x2
#define PACKET_TYPE_ERR      0x3

#define PACKET_TENSOR_TYPE_FLOAT32 0x1
#define PACKET_TENSOR_TYPE_INT8    0x2

typedef uint8_t msg_type_t;
typedef uint8_t data_type_t;

typedef struct packet_hdr {
    uint16_t magic;
    uint8_t version;
    msg_type_t type;
    uint32_t seq_num;
    uint32_t payload_len;
    uint16_t checksum;
} __attribute__((packed)) packet_hdr_t;

// serialize a packet header to a buffer ready to be sent to network
void packet_hdr_serialize(const packet_hdr_t *hdr, uint8_t *buf);

// deserialize a buffer recieved from network to a packet header
void packet_hdr_deserialize(packet_hdr_t *hdr, const uint8_t *buf);

typedef struct tensor_meta {
    uint32_t dims[4];
    uint8_t ndims;
    data_type_t data_type;
    uint32_t total_bytes;
    uint32_t fragment_count;

} __attribute__((packed)) tensor_meta_t;

typedef struct tensor_fragment {
    uint32_t offset;
} __attribute__((packed)) tensor_fragment_t;

// serialize a tensor metadata to a buffer ready to be sent to network
void tensor_meta_serialize(const tensor_meta_t *hdr, uint8_t *buf);

// deserialize a buffer recieved from network to a tensor metadata
void tensor_meta_deserialize(tensor_meta_t *hdr, const uint8_t *buf);

// serialize a tensor fragment to a buffer ready to be sent to network
void tensor_fragment_serialize(const tensor_fragment_t *hdr, uint8_t *buf);

// deserialize a buffer recieved from network to a tensor fragment
void tensor_fragment_deserialize(tensor_fragment_t *hdr, const uint8_t *buf);

// split a tensor buf into tensor fragments, calling send for each.
// returns number of packets sent, or -1 on error
int tensor_send(int sockfd, const struct sockaddr *dest, socklen_t dest_len, const void *data,
                size_t total_bytes, data_type_t data_type, const uint32_t *dims, uint8_t ndims);

// recieve and reassemble packets into dest_buf
// returns total bytes written, or -1 on error
int tensor_recv(int sockfd, float *dest_buf, uint32_t dest_buf_len);

#endif // TINYDIST_PROTOCOL_H_
