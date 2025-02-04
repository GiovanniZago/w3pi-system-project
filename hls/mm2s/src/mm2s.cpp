#include "mm2s.h"

extern "C" {

// void mm2s(ap_int<64>* mem, hls::stream<qdma_axis<32,0,0,0>>& s0, hls::stream<qdma_axis<32,0,0,0>>& s1, int mem_offset) 
// {
// 	#pragma HLS INTERFACE m_axi port=mem offset=slave bundle=gmem
// 	#pragma HLS INTERFACE axis port=s0
// 	#pragma HLS INTERFACE axis port=s1

// 	#pragma HLS INTERFACE s_axilite port=mem bundle=control
// 	#pragma HLS INTERFACE s_axilite port=mem_offset bundle=control
// 	#pragma HLS interface s_axilite port=return bundle=control

// 	ap_int<32> pt_arr[112] = { 0 };
// 	ap_int<32> eta_arr[112] = { 0 };
// 	ap_int<32> phi_arr[112] = { 0 };
// 	ap_int<32> pdg_id_arr[112] = { 0 };

// 	for(int i=0; i<112; i++) 
// 	{
// 		#pragma HLS PIPELINE II=1

// 		ap_int<64> w1 = mem[mem_offset + 2 * i    ];
// 		ap_int<64> w2 = mem[mem_offset + 2 * i + 1];

// 		ap_int<16> pt1     = w1.range(13, 0 );
// 		ap_int<16> eta1    = w1.range(25, 14);
// 		ap_int<16> phi1    = w1.range(36, 26);
// 		ap_int<16> pdg_id1 = w1.range(39, 37);

// 		ap_int<16> pt2     = w2.range(13, 0 );
// 		ap_int<16> eta2    = w2.range(25, 14);
// 		ap_int<16> phi2    = w2.range(36, 26);
// 		ap_int<16> pdg_id2 = w2.range(39, 37);

// 		pt_arr[i].range(15, 0 ) = pt2;
// 		pt_arr[i].range(31, 16) = pt1;

// 		eta_arr[i].range(15, 0 ) = eta2;
// 		eta_arr[i].range(31, 16) = eta1;

// 		phi_arr[i].range(15, 0 ) = phi2;
// 		phi_arr[i].range(31, 16) = phi1;

// 		pdg_id_arr[i].range(15, 0 ) = pdg_id2;
// 		pdg_id_arr[i].range(31, 16) = pdg_id1;
// 	}

// 	for (int i=0; i<112; i++)
// 	{
// 		#pragma HLS PIPELINE II=1

// 		qdma_axis<32,0,0,0> x_pt, x_eta;
// 		x_pt.data = pt_arr[i];
// 		x_eta.data = eta_arr[i];

// 		x_pt.keep_all();
// 		x_eta.keep_all();

// 		s0.write(x_pt);
// 		s1.write(x_eta);
// 	}

// 	for (int i=0; i<112; i++)
// 	{
// 		#pragma HLS PIPELINE II=1

// 		qdma_axis<32,0,0,0> x_phi, x_pdg_id;
// 		x_phi.data = pt_arr[i];
// 		x_pdg_id.data = eta_arr[i];

// 		x_phi.keep_all();
// 		x_pdg_id.keep_all();

// 		s0.write(x_phi);
// 		s1.write(x_pdg_id);
// 	}
// }

void mm2s(ap_int<64>* mem, hls::stream<qdma_axis<32,0,0,0>>& s0, hls::stream<qdma_axis<32,0,0,0>>& s1, int mem_offset) 
{
	#pragma HLS INTERFACE m_axi port=mem offset=slave bundle=gmem
	#pragma HLS INTERFACE axis port=s0
	#pragma HLS INTERFACE axis port=s1

	#pragma HLS INTERFACE s_axilite port=mem bundle=control
	#pragma HLS INTERFACE s_axilite port=mem_offset bundle=control
	#pragma HLS interface s_axilite port=return bundle=control

	ap_int<32> pt_arr[EV_SIZE] = { 0 };
	ap_int<32> eta_arr[EV_SIZE] = { 0 };
	ap_int<32> phi_arr[EV_SIZE] = { 0 };
	ap_int<32> pdg_id_arr[EV_SIZE] = { 0 };

	for(int i=0; i<EV_SIZE; i++) 
	{
		#pragma HLS PIPELINE II=1

		ap_int<64> word = mem[mem_offset + i];

		ap_int<16> pt     = word.range(13, 0 );
		ap_int<16> eta    = word.range(25, 14);
		ap_int<16> phi    = word.range(36, 26);
		ap_int<16> pdg_id = word.range(39, 37);

		pt_arr[i].range(15, 0) = pt;
		eta_arr[i].range(15, 0) = eta;
		phi_arr[i].range(15, 0) = phi;
		pdg_id_arr[i].range(15, 0) = pdg_id;
	}

	for (int i=0; i<EV_SIZE; i++)
	{
		#pragma HLS PIPELINE II=1

		qdma_axis<32,0,0,0> x_pt, x_eta;
		x_pt.data = pt_arr[i];
		x_eta.data = eta_arr[i];

		x_pt.keep_all();
		x_eta.keep_all();

		s0.write(x_pt);
		s1.write(x_eta);
	}

	for (int i=0; i<EV_SIZE; i++)
	{
		#pragma HLS PIPELINE II=1

		qdma_axis<32,0,0,0> x_phi, x_pdg_id;
		x_phi.data = phi_arr[i];
		x_pdg_id.data = pdg_id_arr[i];

		x_phi.keep_all();
		x_pdg_id.keep_all();

		s0.write(x_phi);
		s1.write(x_pdg_id);
	}
}

}