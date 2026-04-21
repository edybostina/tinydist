#include "tinydist/protocol.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define PORT        9000
#define SERVER_IP   "127.0.0.1"
#define MAX_RETRIES 5

int main()
{
    struct sockaddr_in servaddr;
    int sockfd, rc;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 250000; // 250ms

    rc = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    rc = inet_aton(SERVER_IP, &servaddr.sin_addr);
    if (rc == 0) {
        perror("invalid ip");
        exit(EXIT_FAILURE);
    }

    packet_hdr_t data;
    data.magic = PACKET_MAGIC;
    data.checksum = 123;
    data.payload_len = 256;
    data.seq_num = 67;
    data.type = PACKET_TYPE_DATA;
    data.version = 1;

    uint8_t buf[sizeof(packet_hdr_t)];
    packet_hdr_serialize(&data, buf);

    int tries = MAX_RETRIES;
send_buf:
    if (tries == 0) {
        perror("bad connection");
        exit(EXIT_FAILURE);
    }
    rc = sendto(sockfd, buf, sizeof(packet_hdr_t), 0, (struct sockaddr *)&servaddr,
                sizeof(servaddr));

    socklen_t servlen = sizeof(servaddr);
    rc = recvfrom(sockfd, buf, sizeof(packet_hdr_t), 0, (struct sockaddr *)&servaddr, &servlen);

    if (rc < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
        tries--;
        goto send_buf;
    }
    packet_hdr_t recieved_data;
    packet_hdr_deserialize(&recieved_data, buf);
    if (recieved_data.type == PACKET_TYPE_ACK && data.seq_num == recieved_data.seq_num)
        printf("ack for packet %u\n", recieved_data.seq_num);
}
