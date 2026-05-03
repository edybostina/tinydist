#ifndef TINYDIST_NN_H_
#define TINYDIST_NN_H_

#include <stdint.h>

#define TINYDIST_ACTIVATION_NONE    0
#define TINYDIST_ACTIVATION_RELU    1
#define TINYDIST_ACTIVATION_SIGMOID 2

typedef struct {
    const float *W;
    const float *b;
    int in_size;
    int out_size;
    uint8_t activation;
} layer_t;

void layer_forward(const layer_t *layer, const float *in, float *out);

#endif // TINYDIST_NN_H_
