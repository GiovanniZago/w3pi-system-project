#include <adf.h>
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"

#ifndef FUNCTION_KERNELS_H
#define FUNCTION_KERNELS_H

#define NUM_PARTICLES 128
#define V_SIZE 16
#define P_BUNCHES 8

static const int16 PI_11 = 720;
static const int16 MPI_11 = -720;
static const int16 TWOPI_11 = 1440;
static const int16 MTWOPI_11 = -1440;

using namespace adf;

void dr2_11(input_stream<int16> * __restrict in, output_stream<int32> * __restrict out);
void dr2_16(input_stream<int16> * __restrict in0, input_stream<int16> * __restrict in1, output_stream<int32> * __restrict out0);

#endif 