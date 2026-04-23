#include "tinydist/protocol.h"
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define PORT      9000
#define SERVER_IP "127.0.0.1"

int main()
{
    struct sockaddr_in servaddr;
    int sockfd, rc;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    rc = inet_aton(SERVER_IP, &servaddr.sin_addr);
    if (rc == 0) {
        fprintf(stderr, "invalid ip\n");
        exit(EXIT_FAILURE);
    }

    float tensor_data[2][3] = {{1.1f, 2.2f, 3.3f}, {4.4f, 5.5f, 6.6f}};
    uint32_t dims[2] = {2, 3};

    printf("Sender: sending 2x3 tensor:\n");
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 3; j++) {
            printf("%.1f ", tensor_data[i][j]);
        }
        printf("\n");
    }

    rc = tensor_send(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr), tensor_data,
                     sizeof(tensor_data), PACKET_TENSOR_TYPE_FLOAT32, dims, 2);

    if (rc < 0) {
        perror("tensor_send failed");
        exit(EXIT_FAILURE);
    }

    printf("Successfully sent %d bytes\n", rc);

    return 0;
}
