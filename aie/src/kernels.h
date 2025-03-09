#include <adf.h>
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include "aie_api/utils.hpp"

#ifndef FUNCTION_KERNELS_H
#define FUNCTION_KERNELS_H
#define V_SIZE 32
#define EV_SIZE 224
#define P_BUNCHES 7

// #define __X86DEBUG__ 
// #define __X86PTCUTDEBUG__
// #define __x86ISODEBUG__
// #define __x86ANGSEPDEBUG__

static const int16 N_MIN = 16;
static const int16 MIN_PT = 28; // (7  Gev) looser cuts; 48 (12 GeV) tighter cuts for offline reconstruction
static const int16 MED_PT = 48; // (12 Gev) looser cuts; 60 (15 GeV) tighter cuts for offline reconstruction
static const int16 HIG_PT = 60; // (15 Gev) looser cuts; 72 (18 GeV) tighter cuts for offline reconstruction
static const int16 PI = 720;
static const int16 MPI = -720;
static const int16 TWOPI = 1440;
static const int16 MTWOPI = -1440;
static const float MIN_MASS = 60.0; 
static const float MAX_MASS = 100.0; 
static const float MINDR2_FLOAT = 0.01 * 0.01;
static const float MAXDR2_FLOAT = 0.25 * 0.25;
static const float MINDR2_ANGSEP_FLOAT = 0.5 * 0.5;
static const float PI_FLOAT = 3.1415926;
static const float F_CONV = PI_FLOAT / PI;
static const float F_CONV2 = (PI_FLOAT / PI) * (PI_FLOAT / PI);
static const float PT_CONV = 0.25;
static const float MAX_ISO = 2.0; // 0.5 tighter cut
static const float MASS_P = 0.13957039;

using namespace adf;

void w3pi(input_stream<int16> * __restrict in0, input_stream<int16> * __restrict in1, output_stream<int32> * __restrict out);

#endif