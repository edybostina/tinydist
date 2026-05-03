#include "tinydist/nn.h"
#include "tinydist/protocol.h"
#include "tinydist/transport_udp.h"
#include <stdio.h>
#include <stdlib.h>

#define PORT      9000
#define SERVER_IP "127.0.0.1"

int main()
{
    tinydist_udp_ctx_t udp;
    tinydist_transport_t transport;
    tinydist_session_t session;

    if (tinydist_udp_sender(&udp, SERVER_IP, PORT) < 0) {
        perror("tinydist_udp_sender");
        exit(EXIT_FAILURE);
    }
    tinydist_udp_transport(&transport, &udp);
    tinydist_session_init(&session);

    static const float W[3 * 4] = {
        0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f, 1.1f, 1.2f,
    };
    static const float b[3] = {0.1f, -0.1f, 0.0f};

    layer_t layer = {
        .W = W,
        .b = b,
        .in_size = 4,
        .out_size = 3,
        .activation = TINYDIST_ACTIVATION_RELU,
    };

    float in[4] = {1.0f, 0.5f, -0.2f, 0.8f};
    float out[3];

    printf("Sender: forward pass on 4-element input...\n");
    layer_forward(&layer, in, out);

    printf("Sender: output: [ ");
    for (int i = 0; i < 3; i++)
        printf("%.4f ", out[i]);
    printf("]\n");

    uint32_t dims[1] = {3};
    int rc = tensor_send(&transport, &session, out, sizeof(out), TINYDIST_DTYPE_FLOAT32, dims, 1);
    if (rc < 0) {
        fprintf(stderr, "tensor_send failed: %d\n", rc);
        exit(EXIT_FAILURE);
    }

    printf("Sender: sent %d bytes\n", rc);
    return 0;
}
