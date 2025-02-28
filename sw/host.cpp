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
    const uint32_t start_event_idx = 0;
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
	std::cout << "Creating run instances of the kernels" << std::endl;
	auto mm2s_run = xrt::run(mm2s);
	auto s2mm_run = xrt::run(s2mm);

	int triplet_offset = 0;
	uint event_idx = 0;

	auto start_exec = std::chrono::high_resolution_clock::now();
	for (unsigned int block_offset = 0; block_offset < NUM_BLOCKS * NUM_EVENTS; block_offset += NUM_BLOCKS)
	{
		// std::cout << "----- EVENT " << event_idx << " -----" << std::endl;
		// std::cout << "Setting args for mm2s" << std::endl;
		mm2s_run.set_arg(0, events_buf);
		mm2s_run.set_arg(1, nullptr);
		mm2s_run.set_arg(2, nullptr);
		mm2s_run.set_arg(3, block_offset);
		
		// std::cout << "Setting args for s2mm" << std::endl;
		s2mm_run.set_arg(0, triplets_buf);
		s2mm_run.set_arg(1, nullptr);
		s2mm_run.set_arg(2, triplet_offset);

		// std::cout << "Starting mm2s" << std::endl;
		mm2s_run.start();
		// std::cout << "Starting s2mm" << std::endl;
		s2mm_run.start();

		mm2s_run.wait();
		// std::cout << "mm2s kernels executed" << std::endl;
		
		s2mm_run.wait();
		// std::cout << "s2mm kernels executed" << std::endl;

		triplet_offset += TRIPLET_VSIZE;
		event_idx++;
	} 

	auto end_exec = std::chrono::high_resolution_clock::now();
	auto dt_exec = std::chrono::duration<double, std::milli>(end_exec - start_exec).count();

	// auto mm2s_run = mm2s(events_buf, nullptr, nullptr, 0);
	// auto s2mm_run = s2mm(triplets_buf, nullptr, 0);

	// mm2s_run.wait();
	// std::cout << "mm2s kernels executed" << std::endl;

	// s2mm_run.wait();
	// std::cout << "s2mm kernels executed" << std::endl;

	// read output buffer content from device to host
	int32_t triplets[NUM_EVENTS * TRIPLET_VSIZE];
	triplets_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
	triplets_buf.read(triplets);
	std::cout << "Finished executing W3Pi on the input events" << std::endl;
	
	// print out the output
	for (unsigned int i = 0; i < NUM_EVENTS * TRIPLET_VSIZE; i++)
	{
		float val = *reinterpret_cast<float*>(&triplets[i]);
		std::cout << val << std::endl;
	}

	std::cout << "Allocation time: " << dt_allocate << " ms" << std::endl;
	std::cout << "Execuion time: " << dt_exec << " ms" << std::endl;
    
	std::cout << "Releasing remaining XRT objects...\n";
	
	return 0;
}