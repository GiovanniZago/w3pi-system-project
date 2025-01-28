/*
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11
*/

#include <fstream>
#include <cstring>
#include <iostream>

#include "experimental/xrt_kernel.h"
#include "experimental/xrt_graph.h"
#include "data.h" // contains input and golden output data 

static const int NUM_PARTICLES = 128;
static const int NUM_MEM_READ  = 64;  // input: we have to transfer 256 x 16b variables by reading 64b chunks from memory -> 64 reads
static const int NUM_MEM_WRITE = 128; // output: we have to transfer 128 x 32b variables by reading 32b chunks from AI Engine -> 128 writes
							         

int main(int argc, char* argv[])
{
	char* xclbinFile = argv[1];
	
	auto device = xrt::device(0);

	if(device == nullptr) throw std::runtime_error("No valid device handle found. Make sure using right xclOpen index.");
	auto xclbin_uuid = device.load_xclbin(xclbinFile);
	std::cout << "Got xclbin_uuid" << std::endl;

	// instantiate kernels
	std::cout << "Instatiating kernels" << std::endl;
	auto mm2s = xrt::kernel(device, xclbin_uuid, "mm2s");
	auto s2mm = xrt::kernel(device, xclbin_uuid, "s2mm");

	// allocate buffer for the input of the two mm2s kernels	
	std::cout << "Allocating buffers for kernel arguments" << std::endl;
	auto in11_bo  = xrt::bo(device, 2 * NUM_PARTICLES * sizeof(int16_t), xrt::bo::flags::normal, mm2s.group_id(0));
	auto out11_bo = xrt::bo(device,     NUM_PARTICLES * sizeof(int32_t), xrt::bo::flags::normal, s2mm.group_id(0));

	// write buffer content and transfer buffer data from host to device	
	std::cout << "Write input buffer content and transfer to device" << std::endl;
	in11_bo.write(input11);
	in11_bo.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	// create run instances of the kernels
	std::cout << "Starting the kernels" << std::endl;
	auto mm2s_run = mm2s(in11_bo,  nullptr, NUM_MEM_READ );
	auto s2mm_run = s2mm(out11_bo, nullptr, NUM_MEM_WRITE);

	// wait for kernels to be done
	mm2s_run.wait();
	std::cout << "mm2s kernels executed" << std::endl;

	// // obtain the graph handle from the XCLBIN that is loaded into the device
	// auto myGraph = xrt::graph(device, xclbin_uuid, "Dr2Graph");
	
	// // run th graph for 1 iteration
	// std::cout << "Running 1 iteration of the ADF graph" << std::endl;
	// myGraph.run(1);
	
	// // graph end
	// myGraph.end();
	
	// read output buffer content from device to host
	int32_t dr2Output[NUM_PARTICLES];
	s2mm_run.wait();
	out11_bo.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	out11_bo.read(dr2Output);
	std::cout << "ADF graph iteration is over" << std::endl;
	
	//////////////////////////////////////////
	// Comparing the execution data to the golden data
	//////////////////////////////////////////	
	
	int error11_count = 0;
	{
		for (int i=0; i<NUM_PARTICLES; i++)
		{
				if (dr2Output[i] != out11_golden[i])
				{
					printf("OUT11 ERROR found @ %d, %d != %d\n", i, dr2Output[i], out11_golden[i]);
					error11_count++;
				}
		}

		if (error11_count)
			printf("11-bit computation encountered %d errors\n", error11_count);
		else
			printf("11-bit TEST PASSED\n");
	}
	
    
	std::cout << "Releasing remaining XRT objects...\n";
	
	return 0;
}