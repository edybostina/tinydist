#include "tinydist/protocol.h"
#include <arpa/inet.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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

    float recv_buf[6];
    uint8_t data_type;

    printf("Receiver: waiting for tensor...\n");
    rc = tensor_recv(sockfd, recv_buf, sizeof(recv_buf), &data_type);

    if (rc < 0) {
        perror("tensor_recv failed");
        exit(EXIT_FAILURE);
    }

    printf("Receiver: received %d bytes, type %u\n", rc, data_type);
    if (data_type == PACKET_TENSOR_TYPE_FLOAT32) {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 3; j++) {
                printf("%.1f ", recv_buf[i * 3 + j]);
            }
            printf("\n");
        }
    }

    return 0;
}
