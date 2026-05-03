#include "tinydist/protocol.h"
#include "tinydist/crc32.h"
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>

void packet_hdr_serialize(const packet_hdr_t *hdr, uint8_t *buf)
{
    packet_hdr_t wire;
    memcpy(&wire, hdr, sizeof(packet_hdr_t));
    wire.magic = htons(hdr->magic);
    wire.session_id = htons(hdr->session_id);
    wire.seq_num = htonl(hdr->seq_num);
    wire.payload_len = htonl(hdr->payload_len);
    wire.checksum = htonl(hdr->checksum);
    memcpy(buf, &wire, sizeof(packet_hdr_t));
}

void packet_hdr_deserialize(packet_hdr_t *hdr, const uint8_t *buf)
{
    memcpy(hdr, buf, sizeof(packet_hdr_t));
    hdr->magic = ntohs(hdr->magic);
    hdr->session_id = ntohs(hdr->session_id);
    hdr->seq_num = ntohl(hdr->seq_num);
    hdr->payload_len = ntohl(hdr->payload_len);
    hdr->checksum = ntohl(hdr->checksum);
}

void tensor_meta_serialize(const tensor_meta_t *meta, uint8_t *buf)
{
    tensor_meta_t wire;
    memcpy(&wire, meta, sizeof(tensor_meta_t));
    wire.total_bytes = htonl(meta->total_bytes);
    wire.fragment_count = htonl(meta->fragment_count);
    for (int i = 0; i < (int)wire.ndims; i++)
        wire.dims[i] = htonl(wire.dims[i]);
    memcpy(buf, &wire, sizeof(tensor_meta_t));
}

void tensor_meta_deserialize(tensor_meta_t *meta, const uint8_t *buf)
{
    memcpy(meta, buf, sizeof(tensor_meta_t));
    meta->total_bytes = ntohl(meta->total_bytes);
    meta->fragment_count = ntohl(meta->fragment_count);
    for (int i = 0; i < (int)meta->ndims; i++)
        meta->dims[i] = ntohl(meta->dims[i]);
}

void tensor_fragment_serialize(const tensor_fragment_t *frag, uint8_t *buf)
{
    tensor_fragment_t wire;
    wire.offset = htonl(frag->offset);
    memcpy(buf, &wire, sizeof(tensor_fragment_t));
}

void tensor_fragment_deserialize(tensor_fragment_t *frag, const uint8_t *buf)
{
    memcpy(frag, buf, sizeof(tensor_fragment_t));
    frag->offset = ntohl(frag->offset);
}

void tensor_end_serialize(const tensor_end_t *end, uint8_t *buf)
{
    tensor_end_t wire;
    wire.full_crc = htonl(end->full_crc);
    memcpy(buf, &wire, sizeof(tensor_end_t));
}

void tensor_end_deserialize(tensor_end_t *end, const uint8_t *buf)
{
    memcpy(end, buf, sizeof(tensor_end_t));
    end->full_crc = ntohl(end->full_crc);
}

void tinydist_session_init(tinydist_session_t *s)
{
    s->next_session_id = 0;
    s->next_seq = 1;
    s->retransmit_timeout_ms = 50;
    s->max_retries = 3;
}

// send a zero-payload ack or nack back on t with matching session/seq
static void send_reply(tinydist_transport_t *t, uint16_t session_id, uint32_t seq_num,
                       tinydist_msg_type_t type)
{
    uint8_t buf[sizeof(packet_hdr_t)];
    packet_hdr_t hdr;
    hdr.magic = TINYDIST_MAGIC;
    hdr.version = TINYDIST_VERSION;
    hdr.type = type;
    hdr.session_id = session_id;
    hdr.seq_num = seq_num;
    hdr.payload_len = 0;
    hdr.checksum = 0;
    packet_hdr_serialize(&hdr, buf);
    t->send_fn(t->ctx, buf, sizeof(buf));
}

// send pkt then wait for ack matching (session_id, seq_num)
// retransmits on timeout or nack, up to s->max_retries times
static int send_with_ack(tinydist_transport_t *t, tinydist_session_t *s, uint16_t session_id,
                         uint32_t seq_num, const uint8_t *pkt, size_t pkt_len)
{
    uint8_t resp[sizeof(packet_hdr_t)];
    for (int attempt = 0; attempt <= (int)s->max_retries; attempt++) {
        if (t->send_fn(t->ctx, pkt, pkt_len) < 0)
            return TINYDIST_ERR_SOCKET;
        for (;;) {
            int r = t->recv_fn(t->ctx, resp, sizeof(resp));
            if (r < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK)
                    return TINYDIST_ERR_SOCKET;
                break;
            }
            packet_hdr_t ack;
            packet_hdr_deserialize(&ack, resp);
            if (ack.session_id != session_id || ack.seq_num != seq_num)
                continue;
            if (ack.type == TINYDIST_TYPE_ACK)
                return TINYDIST_OK;
            break;
        }
    }
    return TINYDIST_ERR_TIMEOUT;
}

int tensor_send(tinydist_transport_t *t, tinydist_session_t *s, const void *data,
                size_t total_bytes, tinydist_data_type_t data_type, const uint32_t *dims,
                uint8_t ndims)
{
    if (ndims == 0 || ndims > 4)
        return TINYDIST_ERR_PARAM;

    if (t->set_timeout_fn)
        t->set_timeout_fn(t->ctx, s->retransmit_timeout_ms);

    uint16_t session_id = s->next_session_id++;
    size_t max_payload = t->max_payload;
    uint32_t frag_count = (uint32_t)((total_bytes + max_payload - 1) / max_payload);

    tensor_meta_t meta;
    meta.total_bytes = (uint32_t)total_bytes;
    meta.fragment_count = frag_count;
    meta.data_type = data_type;
    meta.ndims = ndims;
    memset(meta.dims, 0, sizeof(meta.dims));
    memcpy(meta.dims, dims, sizeof(uint32_t) * ndims);

    uint8_t meta_buf[sizeof(packet_hdr_t) + sizeof(tensor_meta_t)];
    tensor_meta_serialize(&meta, meta_buf + sizeof(packet_hdr_t));

    packet_hdr_t hdr;
    hdr.magic = TINYDIST_MAGIC;
    hdr.version = TINYDIST_VERSION;
    hdr.type = TINYDIST_TYPE_METADATA;
    hdr.session_id = session_id;
    hdr.seq_num = s->next_seq++;
    hdr.payload_len = sizeof(tensor_meta_t);
    hdr.checksum = crc32(meta_buf + sizeof(packet_hdr_t), hdr.payload_len);
    packet_hdr_serialize(&hdr, meta_buf);

    if (t->send_fn(t->ctx, meta_buf, sizeof(meta_buf)) < 0)
        return TINYDIST_ERR_SOCKET;

    // data fragments
    size_t remaining = total_bytes;
    uint32_t n = frag_count;
    while (n--) {
        size_t chunk = remaining < max_payload ? remaining : max_payload;

        tensor_fragment_t frag;
        frag.offset = (uint32_t)(total_bytes - remaining);

        uint8_t pkt[TINYDIST_MAX_PACKET_SIZE];
        tensor_fragment_serialize(&frag, pkt + sizeof(packet_hdr_t));
        memcpy(pkt + sizeof(packet_hdr_t) + sizeof(tensor_fragment_t),
               (const uint8_t *)data + frag.offset, chunk);

        size_t payload_len = sizeof(tensor_fragment_t) + chunk;
        hdr.type = TINYDIST_TYPE_DATA;
        hdr.seq_num = s->next_seq++;
        hdr.payload_len = (uint32_t)payload_len;
        hdr.checksum = crc32(pkt + sizeof(packet_hdr_t), payload_len);
        packet_hdr_serialize(&hdr, pkt);

        int rc =
            send_with_ack(t, s, session_id, hdr.seq_num, pkt, sizeof(packet_hdr_t) + payload_len);
        if (rc < 0)
            return rc;

        remaining -= chunk;
    }

    // end packet
    tensor_end_t end;
    end.full_crc = crc32(data, total_bytes);
    uint8_t end_buf[sizeof(packet_hdr_t) + sizeof(tensor_end_t)];
    tensor_end_serialize(&end, end_buf + sizeof(packet_hdr_t));
    hdr.type = TINYDIST_TYPE_END;
    hdr.seq_num = s->next_seq++;
    hdr.payload_len = sizeof(tensor_end_t);
    hdr.checksum = crc32(end_buf + sizeof(packet_hdr_t), sizeof(tensor_end_t));
    packet_hdr_serialize(&hdr, end_buf);

    if (t->send_fn(t->ctx, end_buf, sizeof(end_buf)) < 0)
        return TINYDIST_ERR_SOCKET;

    return (int)total_bytes;
}

int tensor_recv(tinydist_transport_t *t, tinydist_session_t *s, void *dest_buf,
                uint32_t dest_buf_len, tinydist_data_type_t *data_type)
{
    // metadata packet
    uint8_t meta_buf[sizeof(packet_hdr_t) + sizeof(tensor_meta_t)];
    if (t->recv_fn(t->ctx, meta_buf, sizeof(meta_buf)) < 0)
        return TINYDIST_ERR_SOCKET;

    packet_hdr_t meta_hdr;
    packet_hdr_deserialize(&meta_hdr, meta_buf);

    if (meta_hdr.magic != TINYDIST_MAGIC)
        return TINYDIST_ERR_MAGIC;
    if (meta_hdr.type != TINYDIST_TYPE_METADATA)
        return TINYDIST_ERR_PARAM;
    if (crc32(meta_buf + sizeof(packet_hdr_t), meta_hdr.payload_len) != meta_hdr.checksum)
        return TINYDIST_ERR_CRC;

    tensor_meta_t meta;
    tensor_meta_deserialize(&meta, meta_buf + sizeof(packet_hdr_t));

    if (meta.total_bytes > dest_buf_len)
        return TINYDIST_ERR_OVERFLOW;
    if (meta.fragment_count > TINYDIST_MAX_FRAGS)
        return TINYDIST_ERR_OVERFLOW;

    uint32_t frag_count = meta.fragment_count;
    uint32_t total_bytes = meta.total_bytes;
    uint16_t session_id = meta_hdr.session_id;
    *data_type = meta.data_type;

    // data fragments
    size_t max_payload = t->max_payload;
    uint8_t seen[TINYDIST_MAX_FRAGS / 8];
    memset(seen, 0, sizeof(seen));
    uint32_t received = 0;

    while (received < frag_count) {
        uint8_t pkt[TINYDIST_MAX_PACKET_SIZE];
        if (t->recv_fn(t->ctx, pkt, sizeof(pkt)) < 0)
            return TINYDIST_ERR_SOCKET;

        packet_hdr_t hdr;
        packet_hdr_deserialize(&hdr, pkt);

        if (crc32(pkt + sizeof(packet_hdr_t), hdr.payload_len) != hdr.checksum) {
            send_reply(t, session_id, hdr.seq_num, TINYDIST_TYPE_NACK);
            continue;
        }
        if (hdr.type != TINYDIST_TYPE_DATA)
            continue; // ignore stale or unexpected packet types

        tensor_fragment_t frag;
        tensor_fragment_deserialize(&frag, pkt + sizeof(packet_hdr_t));

        uint32_t frag_idx = (uint32_t)(frag.offset / max_payload);
        if (frag_idx >= TINYDIST_MAX_FRAGS)
            return TINYDIST_ERR_OVERFLOW;

        size_t chunk = hdr.payload_len - sizeof(tensor_fragment_t);
        if (frag.offset + chunk > dest_buf_len)
            return TINYDIST_ERR_OVERFLOW;

        send_reply(t, session_id, hdr.seq_num, TINYDIST_TYPE_ACK);

        if (!(seen[frag_idx / 8] & (1u << (frag_idx % 8)))) {
            memcpy((uint8_t *)dest_buf + frag.offset,
                   pkt + sizeof(packet_hdr_t) + sizeof(tensor_fragment_t), chunk);
            seen[frag_idx / 8] |= (1u << (frag_idx % 8));
            received++;
        }
    }

    if (s && t->set_timeout_fn)
        t->set_timeout_fn(t->ctx, s->retransmit_timeout_ms * (s->max_retries + 2u));

    // end packet
    uint8_t end_buf[sizeof(packet_hdr_t) + sizeof(tensor_end_t)];
    if (t->recv_fn(t->ctx, end_buf, sizeof(end_buf)) < 0)
        return TINYDIST_ERR_SOCKET;

    packet_hdr_t end_hdr;
    packet_hdr_deserialize(&end_hdr, end_buf);
    if (end_hdr.type != TINYDIST_TYPE_END)
        return TINYDIST_ERR_PARAM;
    if (crc32(end_buf + sizeof(packet_hdr_t), sizeof(tensor_end_t)) != end_hdr.checksum)
        return TINYDIST_ERR_CRC;

    tensor_end_t end;
    tensor_end_deserialize(&end, end_buf + sizeof(packet_hdr_t));
    if (end.full_crc != crc32(dest_buf, total_bytes))
        return TINYDIST_ERR_CRC;

    return (int)total_bytes;
}
