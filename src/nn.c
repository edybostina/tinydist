#include "tinydist/nn.h"

void layer_forward(const float *in, const float *W, const float *b, float *out, int in_size,
                   int out_size)
{
    for (int i = 0; i < out_size; i++) {
        out[i] = b[i];
        for (int j = 0; j < in_size; j++) {
            out[i] += W[i * in_size + j] * in[j];
        }
    }
}
