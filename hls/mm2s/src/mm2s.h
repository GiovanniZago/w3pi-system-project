#ifndef mm2s_h
#define mm2s_h

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define EV_SIZE 224
#define BLOCK_SIZE 4
#define NUM_BLOCKS EV_SIZE / BLOCK_SIZE 
#define N_MIN 16

#define MIN_PT 28
#define MED_PT 48
#define HIG_PT 60

#define __CSIM__

extern "C" 
{

void mm2s(ap_int<64 * BLOCK_SIZE>* mem, hls::stream<qdma_axis<32,0,0,0>>& s0, hls::stream<qdma_axis<32,0,0,0>>& s1, int mem_offset);

}

#endif