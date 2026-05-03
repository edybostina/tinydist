#include "tinydist/protocol.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

int main()
{
    printf("packet_hdr_t size:      %zu\n", sizeof(packet_hdr_t));
    printf("  magic offset:         %zu\n", offsetof(packet_hdr_t, magic));
    printf("  version offset:       %zu\n", offsetof(packet_hdr_t, version));
    printf("  type offset:          %zu\n", offsetof(packet_hdr_t, type));
    printf("  session_id offset:    %zu\n", offsetof(packet_hdr_t, session_id));
    printf("  seq_num offset:       %zu\n", offsetof(packet_hdr_t, seq_num));
    printf("  payload_len offset:   %zu\n", offsetof(packet_hdr_t, payload_len));
    printf("  checksum offset:      %zu\n", offsetof(packet_hdr_t, checksum));

    packet_hdr_t test = {
        .magic = TINYDIST_MAGIC,
        .version = TINYDIST_VERSION,
        .type = TINYDIST_TYPE_DATA,
        .session_id = 7,
        .seq_num = 42,
        .payload_len = 256,
        .checksum = 67,
    };

    uint8_t buf[sizeof(packet_hdr_t)];
    packet_hdr_serialize(&test, buf);

    packet_hdr_t received;
    packet_hdr_deserialize(&received, buf);

    assert(test.magic == received.magic);
    assert(test.version == received.version);
    assert(test.type == received.type);
    assert(test.session_id == received.session_id);
    assert(test.seq_num == received.seq_num);
    assert(test.payload_len == received.payload_len);
    assert(test.checksum == received.checksum);
    printf("serialize/deserialize:  OK\n");

    printf("tensor_meta_t size:     %zu\n", sizeof(tensor_meta_t));
    printf("TINYDIST_UDP_MAX_PAYLOAD: %u\n", TINYDIST_UDP_MAX_PAYLOAD);

    return 0;
}
