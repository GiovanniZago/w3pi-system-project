#include "s2mm.h"

extern "C"
{

void s2mm(ap_int<32>* mem, hls::stream<qdma_axis<32,0,0,0>>& s) 
{

#pragma HLS INTERFACE m_axi port=mem offset=slave bundle=gmem
#pragma HLS INTERFACE axis port=s

#pragma HLS INTERFACE s_axilite port=mem bundle=control
#pragma HLS interface s_axilite port=return bundle=control

ap_int<32> mem_offset = 0;

for (unsigned int event_idx = 0; event_idx < NUM_EVENTS; event_idx++)
{
	for (unsigned int i = 0; i < TRIPLET_VSIZE; i++) 
	{
		#pragma HLS PIPELINE II=1

		qdma_axis<32,0,0,0> x = s.read();
		ap_uint<32> temp = x.get_data();
		mem[i + mem_offset] = temp;
	}

	mem_offset += TRIPLET_VSIZE;
}

}

}