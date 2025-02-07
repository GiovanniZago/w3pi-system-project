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

// void mm2s(ap_int<64>* mem, hls::stream<qdma_axis<32,0,0,0>>& s0, hls::stream<qdma_axis<32,0,0,0>>& s1, int mem_offset) 
// {
// 	#pragma HLS INTERFACE m_axi port=mem offset=slave bundle=gmem
// 	#pragma HLS INTERFACE axis port=s0
// 	#pragma HLS INTERFACE axis port=s1

// 	#pragma HLS INTERFACE s_axilite port=mem bundle=control
// 	#pragma HLS INTERFACE s_axilite port=mem_offset bundle=control
// 	#pragma HLS interface s_axilite port=return bundle=control

// 	ap_int<32> pt_arr[EV_SIZE] = { 0 };
// 	ap_int<32> eta_arr[EV_SIZE] = { 0 };
// 	ap_int<32> phi_arr[EV_SIZE] = { 0 };
// 	ap_int<32> pdg_id_arr[EV_SIZE] = { 0 };

// 	for(int i=0; i<EV_SIZE; i++) 
// 	{
// 		#pragma HLS PIPELINE II=1

// 		ap_int<64> word = mem[mem_offset + i];

// 		ap_int<32> pt = word.range(13, 0);

// 		// use tmp variable to automatically sign-extend the variables
// 		ap_int<32> eta = word.range(25, 14);
// 		eta = eta | ((eta[11]) ? 0xFFFFF000 : 0x00000000);
// 		ap_int<16> phi = word.range(36, 26);
// 		phi = phi | ((phi[10]) ? 0xFFFFFC00 : 0x00000000);
// 		ap_int<16> pdg_id = word.range(39, 37);
// 		pdg_id = pdg_id | ((pdg_id[2]) ? 0xFFFFFFF8 : 0x00000000);

// 		pt_arr[i] = pt;
// 		eta_arr[i] = eta;
// 		phi_arr[i] = phi;
// 		pdg_id_arr[i] = pdg_id;
// 	}

// 	for (int i=0; i<EV_SIZE; i++)
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

// 	for (int i=0; i<EV_SIZE; i++)
// 	{
// 		#pragma HLS PIPELINE II=1

// 		qdma_axis<32,0,0,0> x_phi, x_pdg_id;
// 		x_phi.data = phi_arr[i];
// 		x_pdg_id.data = pdg_id_arr[i];

// 		x_phi.keep_all();
// 		x_pdg_id.keep_all();

// 		s0.write(x_phi);
// 		s1.write(x_pdg_id);
// 	}
// }

// }

void mm2s(ap_int<64 * BLOCK_SIZE>* mem, hls::stream<qdma_axis<32,0,0,0>>& s0, hls::stream<qdma_axis<32,0,0,0>>& s1, int mem_offset) 
{
	#pragma HLS INTERFACE m_axi port=mem offset=slave bundle=gmem
	#pragma HLS INTERFACE axis port=s0
	#pragma HLS INTERFACE axis port=s1

	#pragma HLS INTERFACE s_axilite port=mem bundle=control
	#pragma HLS INTERFACE s_axilite port=mem_offset bundle=control
	#pragma HLS interface s_axilite port=return bundle=control

	ap_int<32> pt[EV_SIZE];
	ap_int<32> eta[EV_SIZE];
	ap_int<32> phi[EV_SIZE];
	ap_int<32> pdg_id[EV_SIZE];

	// ap_int<32> min_pt_counter=0;
	// ap_int<32> med_pt_counter=0;
	// ap_int<32> hig_pt_counter=0;

	ap_int<32> is_filter[N_MIN];
	ap_int<16> is_filter_idx=0;

	for (int i=0; i<NUM_BLOCKS; i++)
	{
		#pragma HLS PIPELINE II=1

		ap_int<64 * BLOCK_SIZE> burst_data = mem[mem_offset + i];
		ap_int<64> buffer[BLOCK_SIZE];

		for (int j=0; j<BLOCK_SIZE; j++)
		{
			#pragma HLS UNROLL

			buffer[j] = burst_data.range(64 * (j + 1) - 1, 64 * j);

			pt[i * BLOCK_SIZE + j] = buffer[j].range(13, 0);
			// min_pt_counter = (pt[i * BLOCK_SIZE + j] >= MIN_PT) ? min_pt_counter++ : min_pt_counter;
			// med_pt_counter = (pt[i * BLOCK_SIZE + j] >= MED_PT) ? med_pt_counter++ : min_pt_counter;
			// hig_pt_counter = (pt[i * BLOCK_SIZE + j] >= HIG_PT) ? hig_pt_counter++ : min_pt_counter;

			eta[i * BLOCK_SIZE + j] = buffer[j].range(25, 14);
			eta[i * BLOCK_SIZE + j] = eta[i * BLOCK_SIZE + j] | ((eta[i * BLOCK_SIZE + j][11]) ? 0xFFFFF000 : 0x00000000);

			phi[i * BLOCK_SIZE + j] = buffer[j].range(36, 26);
			phi[i * BLOCK_SIZE + j] = phi[i * BLOCK_SIZE + j] | ((phi[i * BLOCK_SIZE + j][10]) ? 0xFFFFFC00 : 0x00000000);

			pdg_id[i * BLOCK_SIZE + j] = buffer[j].range(39, 37);
			pdg_id[i * BLOCK_SIZE + j] = pdg_id[i * BLOCK_SIZE + j] | ((pdg_id[i * BLOCK_SIZE + j][2]) ? 0xFFFFFFF8 : 0x00000000);

			if ((pt[i * BLOCK_SIZE + j] >= MIN_PT) & (pdg_id[i * BLOCK_SIZE + j] >= 2))
			{
				is_filter[is_filter_idx] = i * BLOCK_SIZE + j + 1;
				is_filter_idx++;
			}
		}
	}

	qdma_axis<32,0,0,0> x_pt, x_eta, x_phi, x_pdg_id;

	for (int j=0; j<EV_SIZE; j++)
	{
		#pragma HLS PIPELINE II=1

		x_pt.data = pt[j];
		x_eta.data = eta[j];

		x_pt.keep_all();
		x_eta.keep_all();

		s0.write(x_pt);
		s1.write(x_eta);
	}

	for (int j=0; j<EV_SIZE; j++)
	{
		#pragma HLS PIPELINE II=1

		x_phi.data = phi[j];
		x_pdg_id.data = pdg_id[j];

		x_phi.keep_all();
		x_pdg_id.keep_all();

		s0.write(x_phi);
		s1.write(x_pdg_id);
	}

	qdma_axis<32,0,0,0> x_is_filter;

	for (int j=0; j<N_MIN; j++)
	{
		#pragma HLS PIPELINE II=1

		x_is_filter.data = is_filter[j];
		x_is_filter.keep_all();
		s0.write(x_is_filter);
	}

	// qdma_axis<32,0,0,0> x_counters;

	// #pragma HLS PIPELINE

	// x_counters.data = min_pt_counter;
	// x_counters.keep_all();
	// s1.write(x_counters);

	// x_counters.data = med_pt_counter;
	// x_counters.keep_all();
	// s1.write(x_counters);

	// x_counters.data = hig_pt_counter;
	// x_counters.keep_all();
	// s1.write(x_counters);
}

}