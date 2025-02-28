/*
Copyright (C) 2023, Advanced Micro Devices, Inc. All rights reserved.
SPDX-License-Identifier: X11
*/

#include <fstream>
#include <cstring>
#include <iostream>
#include <chrono>

#include "ap_int.h"
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_graph.h"
#include "data.h" // contains input and golden output data 

#define EV_SIZE 224
#define BLOCK_SIZE 16
#define NUM_BLOCKS EV_SIZE / BLOCK_SIZE 

#define TRIPLET_VSIZE 4

static const int NUM_EVENTS = 3564;

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
	auto events_buf   = xrt::bo(device, NUM_EVENTS * EV_SIZE * sizeof(int64_t)          , xrt::bo::flags::normal, mm2s.group_id(0));
	auto triplets_buf = xrt::bo(device, NUM_EVENTS * TRIPLET_VSIZE * sizeof(ap_int<32>) , xrt::bo::flags::normal, s2mm.group_id(0));

	// allocate and sync buffer with events data into the device memory	
	std::cout << "Opening binary file with input data" << std::endl;

	// open stream with data inside the file
    std::ifstream bin_file("/home/giovanni/w3pi-system-project/data/PuppiSignal_224.dump", std::ios::binary);

    if (!bin_file) 
    {
        std::cerr << "Error: Unable to open input binary file" << std::endl;
        return 1;
    }

    // set the start position of the stream to a specific event
    const uint32_t start_event_idx = 3564;
    std::streampos start = start_event_idx * EV_SIZE * sizeof(ap_int<64>);
    bin_file.seekg(start, std::ios::beg);

    // fill array with data for a predefined number of events
	ap_int<64 * BLOCK_SIZE> mem[NUM_EVENTS * NUM_BLOCKS];
    bin_file.read(reinterpret_cast<char*>(mem), NUM_EVENTS * EV_SIZE * sizeof(ap_int<64>));
    bin_file.close();

	// write and sync the array to the device
	std::cout << "Writing data into device memory" << std::endl;
	auto start_allocate = std::chrono::high_resolution_clock::now();

	events_buf.write(mem);
	events_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);

	auto end_allocate = std::chrono::high_resolution_clock::now();
	auto dt_allocate = std::chrono::duration<double, std::milli>(end_allocate - start_allocate).count();

	// create run instances of the kernels

	auto start_mm2s = std::chrono::high_resolution_clock::now();
	auto mm2s_run = mm2s(events_buf, nullptr, nullptr);
	auto s2mm_run = s2mm(triplets_buf, nullptr);
	
	mm2s_run.wait();
	std::cout << "mm2s kernels executed" << std::endl;
	
	s2mm_run.wait();
	auto end_s2mm = std::chrono::high_resolution_clock::now();
	std::cout << "s2mm kernels executed" << std::endl;

	auto dt_exec = std::chrono::duration<double, std::milli>(end_s2mm - start_mm2s).count();

	// read output buffer content from device to host
	int32_t triplets[NUM_EVENTS * TRIPLET_VSIZE];
	auto start_readout = std::chrono::high_resolution_clock::now();
	triplets_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	triplets_buf.read(triplets);
	auto end_readout = std::chrono::high_resolution_clock::now();
	auto dt_readout = std::chrono::duration<double, std::milli>(end_readout - start_readout).count();
	std::cout << "Finished executing W3Pi on the input events" << std::endl;
	
	// print out the output
	uint32_t event_counter = 0;
	for (unsigned int i = 0; i < NUM_EVENTS * TRIPLET_VSIZE; i++)
	{
		if (i % TRIPLET_VSIZE == 0)
		{
			std::cout << "--------- EVENT NO. " << event_counter << " ---------" << std::endl;
			event_counter++;
		}
		float val = *reinterpret_cast<float*>(&triplets[i]);
		std::cout << val << std::endl;
	}

	std::cout << "Allocation time: " << dt_allocate << " ms" << std::endl;
	std::cout << "Execuion time: " << dt_exec << " ms" << std::endl;
	std::cout << "Readout time: " << dt_readout << " ms" << std::endl;
    
	std::cout << "Releasing remaining XRT objects...\n";
	
	return 0;
}