#include "tinydist/protocol.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>

void packet_hdr_serialize(const packet_hdr_t *hdr, uint8_t *buf)
{
    packet_hdr_t data_to_send;
    memcpy(&data_to_send, hdr, sizeof(packet_hdr_t));

    data_to_send.magic = htons(hdr->magic);
    data_to_send.seq_num = htonl(hdr->seq_num);
    data_to_send.payload_len = htonl(hdr->payload_len);
    data_to_send.checksum = htons(hdr->checksum);
    memcpy(buf, &data_to_send, sizeof(packet_hdr_t));
}

void packet_hdr_deserialize(packet_hdr_t *hdr, const uint8_t *buf)
{
    memcpy(hdr, buf, sizeof(packet_hdr_t));

    hdr->magic = ntohs(hdr->magic);
    hdr->seq_num = ntohl(hdr->seq_num);
    hdr->payload_len = ntohl(hdr->payload_len);
    hdr->checksum = ntohs(hdr->checksum);
}

void tensor_meta_serialize(const tensor_meta_t *hdr, uint8_t *buf)
{
    tensor_meta_t data_to_send;
    memcpy(&data_to_send, hdr, sizeof(tensor_meta_t));

    data_to_send.total_bytes = htonl(hdr->total_bytes);
    data_to_send.fragment_count = htonl(hdr->fragment_count);

    for (int i = 0; i < 4; i++)
        data_to_send.dims[i] = htonl(data_to_send.dims[i]);

    memcpy(buf, &data_to_send, sizeof(tensor_meta_t));
}

void tensor_meta_deserialize(tensor_meta_t *hdr, const uint8_t *buf)
{
    memcpy(hdr, buf, sizeof(tensor_meta_t));

    hdr->fragment_count = ntohl(hdr->fragment_count);
    hdr->total_bytes = ntohl(hdr->total_bytes);

    for (int i = 0; i < 4; i++)
        hdr->dims[i] = ntohl(hdr->dims[i]);
}

void tensor_fragment_serialize(const tensor_fragment_t *hdr, uint8_t *buf)
{
    tensor_fragment_t data_to_send;
    memcpy(&data_to_send, hdr, sizeof(tensor_fragment_t));

    data_to_send.offset = htonl(data_to_send.offset);
}

void tensor_fragment_deserialize(tensor_fragment_t *hdr, const uint8_t *buf)
{
    memcpy(hdr, buf, sizeof(tensor_fragment_t));

    hdr->offset = ntohl(hdr->offset);
}

int tensor_send(int sockfd, const struct sockaddr *dest, socklen_t dest_len, const void *data,
                size_t total_bytes, data_type_t data_type, const uint32_t *dims, uint8_t ndims)
{
    size_t fragment_count = (total_bytes + 1453) / 1454;

    tensor_meta_t metadata;
    metadata.total_bytes = total_bytes;
    metadata.fragment_count = fragment_count;
    metadata.data_type = data_type;
    metadata.ndims = ndims;
    memcpy(metadata.dims, dims, sizeof(uint32_t) * 4);

    uint8_t meta_buf[sizeof(tensor_meta_t)];
    tensor_meta_serialize(&metadata, meta_buf);

    int rc;
    rc = sendto(sockfd, &meta_buf, sizeof(packet_hdr_t), 0, dest, dest_len);

    while (fragment_count) {
        // TODO: finish this
        tensor_fragment_t current_fragment;
    }
}

int tensor_recv(int sockfd, float *dest_buf, uint32_t dest_buf_len);
