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

    rc = sendto(sockfd, &, sizeof(packet_hdr_t), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
}

int tensor_recv(int sockfd, float *dest_buf, uint32_t dest_buf_len);
