#ifndef mm2s_h
#define mm2s_h

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define EV_SIZE = 224

extern "C" 
{

void mm2s(ap_int<64>* mem, hls::stream<qdma_axis<32,0,0,0>>& s0, hls::stream<qdma_axis<32,0,0,0>>& s1, int mem_offset);

}

#endif