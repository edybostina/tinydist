#include "tinydist/protocol.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>
#define PORT 9000

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    int rc = bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    if (rc < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr;
    uint8_t buf[sizeof(packet_hdr_t)];
    socklen_t clen = sizeof(client_addr);

    uint32_t last_seen_packet = 0;
    bool recieved_anything = false;

    while (1) {
        rc = recvfrom(sockfd, buf, sizeof(packet_hdr_t), 0, (struct sockaddr *)&client_addr, &clen);
        if (rc < 0)
            break;

        packet_hdr_t recieved_packet;
        packet_hdr_deserialize(&recieved_packet, buf);

        packet_hdr_t ack = {0};
        ack.type = PACKET_TYPE_ACK;
        ack.magic = PACKET_MAGIC;
        ack.seq_num = recieved_packet.seq_num;
        packet_hdr_serialize(&ack, buf);

        rc = sendto(sockfd, buf, sizeof(ack), 0, (struct sockaddr *)&client_addr, clen);

        if (last_seen_packet == recieved_packet.seq_num && recieved_anything) {
            printf("got same packet, sending ack but dropping it\n");
            continue;
        }
        last_seen_packet = recieved_packet.seq_num;
        recieved_anything = true;

        printf("recieved: \n");
        printf("magic: %u\n", recieved_packet.magic);
        printf("version: %u\n", recieved_packet.version);
        printf("type: %u\n", recieved_packet.type);
        printf("payload len: %u\n", recieved_packet.payload_len);
        printf("seq num: %u\n", recieved_packet.seq_num);
        printf("checksum: %u\n", recieved_packet.checksum);
    }
}
