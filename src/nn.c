#include "tinydist/nn.h"
#include <math.h>

void layer_forward(const layer_t *layer, const float *in, float *out)
{
    for (int i = 0; i < layer->out_size; i++) {
        float acc = layer->b[i];
        for (int j = 0; j < layer->in_size; j++)
            acc += layer->W[i * layer->in_size + j] * in[j];

        switch (layer->activation) {
            case TINYDIST_ACTIVATION_RELU:
                out[i] = acc > 0.0f ? acc : 0.0f;
                break;
            case TINYDIST_ACTIVATION_SIGMOID:
                out[i] = 1.0f / (1.0f + expf(-acc));
                break;
            default:
                out[i] = acc;
                break;
        }
    }
}
