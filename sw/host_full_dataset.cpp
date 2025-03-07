#include "host.h"

int main(int argc, char* argv[])
{
	char* xclbinFile = argv[1];
	
	auto device = xrt::device(0);

	if(device == nullptr) throw std::runtime_error("No valid device handle found. Make sure using right xclOpen index.");
	auto xclbin_uuid = device.load_xclbin(xclbinFile);
	checkpoint("XCLBIN_UUID", DEBUG);
	
	// instantiate kernels
	auto mm2s = xrt::kernel(device, xclbin_uuid, "mm2s");
	auto s2mm = xrt::kernel(device, xclbin_uuid, "s2mm");
	checkpoint("KERNEL_INSTANCE", DEBUG);

	// allocate buffer for the input of the two mm2s kernels	
	auto events_buf   = xrt::bo(device, NUM_EVENTS * EV_SIZE * sizeof(int64_t)          , xrt::bo::flags::normal, mm2s.group_id(0));
	auto triplets_buf = xrt::bo(device, NUM_EVENTS * TRIPLET_VSIZE * sizeof(ap_int<32>) , xrt::bo::flags::normal, s2mm.group_id(0));
	checkpoint("BUFFER_OBJECT", DEBUG);

	// open out file in append mode
	std::ofstream out_file("/home/giovanni/w3pi-system-project/results/l1Nano_WTo3Pion_PU200_hwreco_fulldata.csv", std::ios::app);

	if (!out_file) 
    {
        std::cerr << "Error: Unable to open output file" << std::endl;
        return 1;
    }

	// define the start event index for each orbit
	uint32_t start_event_idx = 0;

	for (unsigned int idx_orbit = 0; idx_orbit < NUM_ORBITS; idx_orbit++)
	{
		std::cout << "------------- Processing Orbit: " << idx_orbit << " Start Evt Index: " << start_event_idx << " -------------" << std::endl;

		std::ifstream bin_file("/home/giovanni/w3pi-system-project/data/PuppiSignal_224.dump", std::ios::binary);

		if (!bin_file) 
		{
			std::cerr << "Error: Unable to open input binary file" << std::endl;
			return 1;
		}

		// set the start position of the stream to the first event of the orbit
		std::streampos start = start_event_idx * EV_SIZE * sizeof(ap_int<64>);
		bin_file.seekg(start, std::ios::beg);

		// fill array with data of an entire orbit
		ap_int<64 * BLOCK_SIZE> mem[NUM_EVENTS * NUM_BLOCKS];
		bin_file.read(reinterpret_cast<char*>(mem), NUM_EVENTS * EV_SIZE * sizeof(ap_int<64>));
		bin_file.close();

		// write and sync the array to the device
		auto start_allocate = std::chrono::high_resolution_clock::now();
	
		events_buf.write(mem);
		events_buf.sync(XCL_BO_SYNC_BO_TO_DEVICE);
	
		auto end_allocate = std::chrono::high_resolution_clock::now();
		auto dt_allocate = std::chrono::duration<double, std::milli>(end_allocate - start_allocate).count();
		checkpoint("MEM_ALLOC", DEBUG);
	
		// create run instances of the kernels
		auto start_mm2s = std::chrono::high_resolution_clock::now();
		auto mm2s_run = mm2s(events_buf, nullptr, nullptr);
		auto s2mm_run = s2mm(triplets_buf, nullptr);
		
		mm2s_run.wait();
		checkpoint("MM2S_DONE", DEBUG);
		
		s2mm_run.wait();
		checkpoint("S2MM_DONE", DEBUG);
		auto end_s2mm = std::chrono::high_resolution_clock::now();
		auto dt_exec = std::chrono::duration<double, std::milli>(end_s2mm - start_mm2s).count();
	
		// read output buffer content from device to host
		int32_t triplets[NUM_EVENTS * TRIPLET_VSIZE];
		auto start_readout = std::chrono::high_resolution_clock::now();
		triplets_buf.sync(XCL_BO_SYNC_BO_FROM_DEVICE);
		triplets_buf.read(triplets);
		auto end_readout = std::chrono::high_resolution_clock::now();
		auto dt_readout = std::chrono::duration<double, std::milli>(end_readout - start_readout).count();
		checkpoint("MEM_READOUT", DEBUG);

		// print out the output to the out file
		for (unsigned int i = 0; i < NUM_EVENTS * TRIPLET_VSIZE; i++)
		{
			float val = *reinterpret_cast<float*>(&triplets[i]);
			out_file << val << std::endl;
		}
	
		if (PRINT_TIME)
		{
			std::cout << "Allocation time: " << dt_allocate << " ms" << std::endl;
			std::cout << "Execuion time: " << dt_exec << " ms" << std::endl;
			std::cout << "Readout time: " << dt_readout << " ms" << std::endl;
		}

		start_event_idx += NUM_EVENTS;
	}

	return 0;
}