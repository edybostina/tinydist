#include "tinydist/protocol.h"
#include "tinydist/crc32.h"
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
    data_to_send.checksum = htonl(hdr->checksum);
    memcpy(buf, &data_to_send, sizeof(packet_hdr_t));
}

void packet_hdr_deserialize(packet_hdr_t *hdr, const uint8_t *buf)
{
    memcpy(hdr, buf, sizeof(packet_hdr_t));

    hdr->magic = ntohs(hdr->magic);
    hdr->seq_num = ntohl(hdr->seq_num);
    hdr->payload_len = ntohl(hdr->payload_len);
    hdr->checksum = ntohl(hdr->checksum);
}

void tensor_meta_serialize(const tensor_meta_t *hdr, uint8_t *buf)
{
    tensor_meta_t data_to_send;
    memcpy(&data_to_send, hdr, sizeof(tensor_meta_t));

    data_to_send.total_bytes = htonl(hdr->total_bytes);
    data_to_send.fragment_count = htonl(hdr->fragment_count);

    for (int i = 0; i < data_to_send.ndims; i++)
        data_to_send.dims[i] = htonl(data_to_send.dims[i]);

    memcpy(buf, &data_to_send, sizeof(tensor_meta_t));
}

void tensor_meta_deserialize(tensor_meta_t *hdr, const uint8_t *buf)
{
    memcpy(hdr, buf, sizeof(tensor_meta_t));

    hdr->fragment_count = ntohl(hdr->fragment_count);
    hdr->total_bytes = ntohl(hdr->total_bytes);

    for (int i = 0; i < hdr->ndims; i++)
        hdr->dims[i] = ntohl(hdr->dims[i]);
}

void tensor_fragment_serialize(const tensor_fragment_t *hdr, uint8_t *buf)
{
    tensor_fragment_t data_to_send;
    memcpy(&data_to_send, hdr, sizeof(tensor_fragment_t));

    data_to_send.offset = htonl(data_to_send.offset);
    memcpy(buf, &data_to_send, sizeof(tensor_fragment_t));
}

void tensor_fragment_deserialize(tensor_fragment_t *hdr, const uint8_t *buf)
{
    memcpy(hdr, buf, sizeof(tensor_fragment_t));

    hdr->offset = ntohl(hdr->offset);
}

int tensor_send(int sockfd, const struct sockaddr *dest, socklen_t dest_len, const void *data,
                size_t total_bytes, data_type_t data_type, const uint32_t *dims, uint8_t ndims)
{
    uint8_t meta_buf[sizeof(packet_hdr_t) + sizeof(tensor_meta_t)];
    packet_hdr_t header;
    header.magic = PACKET_MAGIC;
    header.payload_len = sizeof(tensor_meta_t);
    header.seq_num = 1;
    header.type = PACKET_TYPE_METADATA;
    header.version = PACKET_VERSION;

    size_t fragment_count = (total_bytes + MAX_FRAGMENT_PAYLOAD - 1) / MAX_FRAGMENT_PAYLOAD;

    tensor_meta_t metadata;
    metadata.total_bytes = total_bytes;
    metadata.fragment_count = fragment_count;
    metadata.data_type = data_type;
    metadata.ndims = ndims;
    memset(metadata.dims, 0, sizeof(uint32_t) * 4);
    memcpy(metadata.dims, dims, sizeof(uint32_t) * ndims);

    tensor_meta_serialize(&metadata, meta_buf + sizeof(packet_hdr_t));

    header.checksum = crc32(meta_buf + sizeof(packet_hdr_t), header.payload_len);
    packet_hdr_serialize(&header, meta_buf);

    int rc;
    int total_sent = 0;
    rc = sendto(sockfd, meta_buf, sizeof(packet_hdr_t) + sizeof(tensor_meta_t), 0, dest, dest_len);
    if (rc < 0) {
        return -1;
    }

    total_sent += rc;

    size_t remaining_bytes = total_bytes;
    size_t current_offset = 0;
    int seq = 2;

    while (fragment_count) {
        size_t current_packet_size =
            remaining_bytes <= MAX_FRAGMENT_PAYLOAD
                ? remaining_bytes + sizeof(tensor_fragment_t) + sizeof(packet_hdr_t)
                : (MAX_FRAGMENT_PAYLOAD + sizeof(packet_hdr_t) + sizeof(tensor_fragment_t));
        size_t raw_data_to_send =
            (current_packet_size - sizeof(packet_hdr_t) - sizeof(tensor_fragment_t));
        packet_hdr_t header;
        header.magic = PACKET_MAGIC;
        header.payload_len = current_packet_size - sizeof(packet_hdr_t);
        header.seq_num = seq++;
        header.type = PACKET_TYPE_DATA;
        header.version = PACKET_VERSION;

        tensor_fragment_t current_fragment;
        current_fragment.offset = current_offset;

        uint8_t packet_buf[1500];

        tensor_fragment_serialize(&current_fragment, packet_buf + sizeof(packet_hdr_t));

        memcpy(packet_buf + sizeof(packet_hdr_t) + sizeof(tensor_fragment_t),
               (uint8_t *)data + (total_bytes - remaining_bytes), raw_data_to_send);

        header.checksum = crc32(packet_buf + sizeof(packet_hdr_t), header.payload_len);
        packet_hdr_serialize(&header, packet_buf);

        rc = sendto(sockfd, packet_buf, current_packet_size, 0, dest, dest_len);
        if (rc < 0) {
            return total_sent;
        }
        total_sent += rc;

        remaining_bytes -= raw_data_to_send;
        current_offset += raw_data_to_send;
        fragment_count--;
    }
    return total_bytes;
}

int tensor_recv(int sockfd, void *dest_buf, uint32_t dest_buf_len, uint8_t *data_type)
{
    int rc;
    int total_recv = 0;
    uint8_t meta_buf[sizeof(packet_hdr_t) + sizeof(tensor_meta_t)];
    rc = recv(sockfd, meta_buf, sizeof(packet_hdr_t) + sizeof(tensor_meta_t), 0);
    if (rc < 0)
        return -1;
    total_recv += rc;

    packet_hdr_t meta_header;
    packet_hdr_deserialize(&meta_header, meta_buf);

    if (meta_header.magic != PACKET_MAGIC) {
        return -1;
    }

    if (meta_header.type != PACKET_TYPE_METADATA) {
        return -1;
    }

    uint32_t computed_checksum = crc32(meta_buf + sizeof(packet_hdr_t), meta_header.payload_len);
    if (computed_checksum != meta_header.checksum) {
        return -1;
    }

    tensor_meta_t meta;
    tensor_meta_deserialize(&meta, meta_buf + sizeof(packet_hdr_t));

    if (meta.total_bytes > dest_buf_len) {
        // buffer is too small to recieve data
        return -1;
    }

    uint32_t fragment_count = meta.fragment_count;
    uint32_t total_bytes = meta.total_bytes;
    uint32_t remaining_bytes = total_bytes;
    *data_type = meta.data_type;

    while (fragment_count) {
        size_t current_packet_size =
            remaining_bytes <= MAX_FRAGMENT_PAYLOAD
                ? remaining_bytes + sizeof(tensor_fragment_t) + sizeof(packet_hdr_t)
                : (MAX_FRAGMENT_PAYLOAD + sizeof(packet_hdr_t) + sizeof(tensor_fragment_t));

        size_t raw_data_to_recv =
            (current_packet_size - sizeof(packet_hdr_t) - sizeof(tensor_fragment_t));
        packet_hdr_t header;
        tensor_fragment_t current_fragment;

        uint8_t packet_buf[1500];

        rc = recv(sockfd, packet_buf, current_packet_size, 0);
        if (rc < 0) {
            return total_recv;
        }

        total_recv += rc;
        packet_hdr_deserialize(&header, packet_buf);

        uint32_t computed_checksum = crc32(packet_buf + sizeof(packet_hdr_t), header.payload_len);
        if (computed_checksum != header.checksum) {
            return -1;
        }

        tensor_fragment_deserialize(&current_fragment, packet_buf + sizeof(packet_hdr_t));

        memcpy((uint8_t *)dest_buf + current_fragment.offset,
               packet_buf + sizeof(packet_hdr_t) + sizeof(tensor_fragment_t), raw_data_to_recv);

        remaining_bytes -= raw_data_to_recv;
        fragment_count--;
    }
    return total_recv;
}
