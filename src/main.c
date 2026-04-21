#include "tinydist/protocol.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    printf("Size of struct is: %zu\n", sizeof(packet_hdr_t));
    printf("magic offset: %zu\n", offsetof(packet_hdr_t, magic));
    printf("version offset: %zu\n", offsetof(packet_hdr_t, version));
    printf("type offset: %zu\n", offsetof(packet_hdr_t, type));
    printf("seq_num offset: %zu\n", offsetof(packet_hdr_t, seq_num));
    printf("payload_len offset: %zu\n", offsetof(packet_hdr_t, payload_len));
    printf("checksum offset: %zu\n", offsetof(packet_hdr_t, checksum));

    packet_hdr_t test = {0};
    test.magic = PACKET_MAGIC;
    test.version = 1;
    test.checksum = 67;
    test.seq_num = 42;
    test.payload_len = 256;
    test.type = PACKET_TYPE_DATA;

    uint8_t buf[sizeof(packet_hdr_t)];
    packet_hdr_serialize(&test, buf);

    packet_hdr_t recieved;
    packet_hdr_deserialize(&recieved, buf);

    assert(test.type == recieved.type);
    assert(test.magic == recieved.magic);
    assert(test.payload_len == recieved.payload_len);
    assert(test.seq_num == recieved.seq_num);
    assert(test.version == recieved.version);
    assert(test.checksum == recieved.checksum);
    printf("success\n");

    printf("size of tensor metadata: %lu\n", sizeof(tensor_meta_t));

    return 0;
}
