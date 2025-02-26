/*
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11
*/


#include <ap_int.h>
#include <hls_stream.h>
#include <ap_axi_sdata.h>

#define TRIPLET_VSIZE 4

extern "C" {

void s2mm(ap_int<32>* mem, hls::stream<qdma_axis<32,0,0,0>>& s, int mem_offset) 
{

#pragma HLS INTERFACE m_axi port=mem offset=slave bundle=gmem
#pragma HLS INTERFACE axis port=s

#pragma HLS INTERFACE s_axilite port=mem bundle=control
#pragma HLS INTERFACE s_axilite port=mem_offset bundle=control
#pragma HLS interface s_axilite port=return bundle=control

	for(int i=0; i<TRIPLET_VSIZE; i++) 
	{
		qdma_axis<32,0,0,0> x = s.read();
		mem[i + mem_offset] = x.data;
	}

}

}