#include <iostream>
#include <fstream>
#include <vector>

#include "hls_stream.h"
#include "ap_int.h"
#include "src/mm2s.h"

int main() 
{
    ap_int<64 * BLOCK_SIZE> mem[NUM_BLOCKS * NUM_EVENTS];
    hls::stream<qdma_axis<32,0,0,0>> s0, s1;
    
    // open binary data file 
    std::ifstream bin_file("/home/giovanni/w3pi-system-project/data/PuppiSignal_224.dump", std::ios::binary);

    if (!bin_file) 
    {
        std::cerr << "Error: Unable to open input binary file" << std::endl;
        return 1;
    }

    // start = event_idx * EV_SIZE * sizeof(ap_int<64>) means that we will skip the 
    // number of events specified by event_idx
    const uint32_t start_event_idx = 0;
    std::streampos start = start_event_idx * EV_SIZE * sizeof(ap_int<64>);
    bin_file.seekg(start, std::ios::beg);

    // cast mem to a pointer to char and we fill it with EV_SIZE * no. of bytes occupied by an ap_int<64>
    // i.e. we fill mem with the data corresponding to an event
    bin_file.read(reinterpret_cast<char*>(mem), NUM_EVENTS * EV_SIZE * sizeof(ap_int<64>));
    bin_file.close();

    mm2s(mem, s0, s1);

    while (!s0.empty()) 
    {
        qdma_axis<32,0,0,0> out_s0 = s0.read();
        int32_t data_s0 = out_s0.data.to_int();
        int16_t data_s0_0 = (data_s0 >> 16) & 0xFFFF; 
        int16_t data_s0_1 = data_s0 & 0xFFFF; 

        if (!s1.empty())
        {
            qdma_axis<32,0,0,0> out_s1 = s1.read();
            int32_t data_s1 = out_s1.data.to_int();
            int16_t data_s1_0 = (data_s1 >> 16) & 0xFFFF; 
            int16_t data_s1_1 = data_s1 & 0xFFFF; 

            std::cout << data_s0_0 << "\t" << data_s0_1 << "\t\t" << data_s1_0 << "\t" << data_s1_1 << std::endl;
        } else
        {
            std::cout << data_s0_0 << "\t" << data_s0_1 << "\t\t" << "/" << "\t" << "/" << std::endl;
        }
    }
    return 0;
}