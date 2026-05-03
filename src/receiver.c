#include "tinydist/nn.h"
#include "tinydist/protocol.h"
#include "tinydist/transport_udp.h"
#include <stdio.h>
#include <stdlib.h>

#define PORT 9000

int main()
{
    tinydist_udp_ctx_t udp;
    tinydist_transport_t transport;
    tinydist_session_t session;

    if (tinydist_udp_receiver(&udp, PORT) < 0) {
        perror("tinydist_udp_receiver");
        exit(EXIT_FAILURE);
    }
    tinydist_udp_transport(&transport, &udp);
    tinydist_session_init(&session);

    static const float W2[2 * 3] = {
        0.5f, -0.5f, 1.0f, -1.0f, 0.5f, 0.5f,
    };
    static const float b2[2] = {0.2f, 0.3f};

    layer_t layer = {
        .W = W2,
        .b = b2,
        .in_size = 3,
        .out_size = 2,
        .activation = TINYDIST_ACTIVATION_NONE,
    };

    float recv_buf[3];
    tinydist_data_type_t data_type;

    printf("Receiver: waiting for tensor...\n");
    int rc = tensor_recv(&transport, &session, recv_buf, sizeof(recv_buf), &data_type);
    if (rc < 0) {
        fprintf(stderr, "tensor_recv failed: %d\n", rc);
        exit(EXIT_FAILURE);
    }

    if (data_type != TINYDIST_DTYPE_FLOAT32) {
        fprintf(stderr, "unexpected data type %u\n", data_type);
        exit(EXIT_FAILURE);
    }

    printf("Receiver: got %d bytes, input: [ ", rc);
    for (int i = 0; i < 3; i++)
        printf("%.4f ", recv_buf[i]);
    printf("]\n");

    float out2[2];
    layer_forward(&layer, recv_buf, out2);

    printf("Receiver: output: [ ");
    for (int i = 0; i < 2; i++)
        printf("%.4f ", out2[i]);
    printf("]\n");

    return 0;
}
