#include "mm2s.h"

extern "C" {

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

	ap_int<32> is_filter[N_MIN];

	for (int i=0; i<N_MIN; i++) 
	{
    	#pragma HLS UNROLL
    	is_filter[i] = 0;
	}

	ap_int<16> is_filter_idx = 0;

	for (int i=0; i<NUM_BLOCKS; i++)
	{
		#pragma HLS PIPELINE II=1

		ap_int<64 * BLOCK_SIZE> burst_data = mem[mem_offset + i];
		ap_int<64> buffer[BLOCK_SIZE];

		for (int j=0; j<BLOCK_SIZE; j++)
		{
			#pragma HLS UNROLL

			buffer[j] = burst_data.range(64 * (j + 1) - 1, 64 * j);

			pt    [i * BLOCK_SIZE + j] = buffer[j].range(13, 0);

			eta   [i * BLOCK_SIZE + j] = buffer[j].range(25, 14);
			eta   [i * BLOCK_SIZE + j] = eta[i * BLOCK_SIZE + j] | ((eta[i * BLOCK_SIZE + j][11]) ? 0xFFFFF000 : 0x00000000);

			phi   [i * BLOCK_SIZE + j] = buffer[j].range(36, 26);
			phi   [i * BLOCK_SIZE + j] = phi[i * BLOCK_SIZE + j] | ((phi[i * BLOCK_SIZE + j][10]) ? 0xFFFFFC00 : 0x00000000);

			pdg_id[i * BLOCK_SIZE + j] = buffer[j].range(39, 37);
			pdg_id[i * BLOCK_SIZE + j] = pdg_id[i * BLOCK_SIZE + j];

			#ifdef __CSIM__
			bool is_filter_condition = (pt[i] >= MIN_PT) && ((pdg_id[i] >= 2) && (pdg_id[i] <= 5)); 
			printf("Idx %d\tpt (%d) >= PT_MIN = %d \t pdg_id (%d) in [2,5] = %d\n", i * BLOCK_SIZE + j, pt[i * BLOCK_SIZE + j], pt[i * BLOCK_SIZE + j] >= MIN_PT, pdg_id[i * BLOCK_SIZE + j], (pdg_id[i] >= 2) && (pdg_id[i] <= 5));
			printf("      \tpt (%d) >= PT_MED = %d \t pt (%d) >= PT_HIG = %d\n", pt[i * BLOCK_SIZE + j], pt[i * BLOCK_SIZE + j] >= MED_PT, pdg_id[i * BLOCK_SIZE + j], pt[i * BLOCK_SIZE + j] >= HIG_PT);
			printf("      \tfiltered = %d\n", is_filter_condition);
			#endif
		}
	}

	for (int i=0; i<EV_SIZE; i++)
	{
		#pragma HLS PIPELINE II=1

		bool is_filter_condition = (pt[i] >= MIN_PT) && ((pdg_id[i] >= 2) && (pdg_id[i] <= 5));
		is_filter[is_filter_idx] = (is_filter_condition) ? ap_int<32>(i + 1) : is_filter[is_filter_idx];
		is_filter_idx += is_filter_condition;
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
}

}