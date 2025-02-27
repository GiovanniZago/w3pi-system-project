#ifndef s2mm_h
#define s2mm_h

#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define TRIPLET_VSIZE 4

extern "C"
{

void s2mm(ap_int<32>* mem, hls::stream<qdma_axis<32,0,0,0>>& s, int mem_offset); 

}

#endif
