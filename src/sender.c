#include "tinydist/nn.h"
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

    float in[4] = {1.0f, 0.5f, -0.2f, 0.8f};

    float W[3 * 4] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f};

    float b[3] = {0.1f, -0.1f, 0.0f};

    float out[3];

    printf("Sender: performing forward pass on 4-element input...\n");
    layer_forward(in, W, b, out, 4, 3);

    printf("Sender: output vector to send: [ ");
    for (int i = 0; i < 3; i++) {
        printf("%.4f ", out[i]);
    }
    printf("]\n");

    uint32_t dims[1] = {3};

    rc = tensor_send(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr), out, sizeof(out),
                     PACKET_TENSOR_TYPE_FLOAT32, dims, 1);

    if (rc < 0) {
        perror("tensor_send failed");
        exit(EXIT_FAILURE);
    }

    printf("Successfully sent %d bytes\n", rc);

    return 0;
}
