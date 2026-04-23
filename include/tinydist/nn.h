#ifndef TINYDIST_NN_H_
#define TINYDIST_NN_H_

void layer_forward(const float *in, const float *W, const float *b, float *out, int in_size,
                   int out_size);

#endif // TINYDIST_NN_H_
