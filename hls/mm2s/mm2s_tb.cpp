#include <iostream>
#include <fstream>
#include "hls_stream.h"
#include "ap_int.h"
#include "src/mm2s.h"

int main() 
{
    ap_int<64 * BLOCK_SIZE> mem[NUM_BLOCKS];
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
    const int event_idx = 1;
    std::streampos start = event_idx * EV_SIZE * sizeof(ap_int<64>);
    bin_file.seekg(start, std::ios::beg);

    // cast mem to a pointer to char and we fill it with EV_SIZE * no. of bytes occupied by an ap_int<64>
    // i.e. we fill mem with the data corresponding to an event
    bin_file.read(reinterpret_cast<char*>(mem), EV_SIZE * sizeof(ap_int<64>));
    bin_file.close();
    
    // execute the hls kernel
    std::cout << "\n\n-------------------------- Kernel Debug Printfs --------------------------\n\n" << std::endl;

    int mem_offset = 0;
    mm2s(mem, s0, s1, mem_offset);
    
    std::cout << "\n\n-------------------------- Reading out Streams --------------------------\n\n" << std::endl;
    
    // compare kernel output with golden output
    for (int i = 0; i < 2 * EV_SIZE; i++) 
    {
        if (!s0.empty() && !s1.empty()) 
        {
            qdma_axis<32,0,0,0> out_s0 = s0.read();
            qdma_axis<32,0,0,0> out_s1 = s1.read();
            
            std::cout << "idx: " << i << "\ts0: " << out_s0.data.to_int() << "\ts1: " << out_s1.data.to_int() << std::endl;
        } else 
        {
            std::cerr << "Error: Stream underflow at index " << i << std::endl;
            return 1;
        }
    }

    for (int i=0; i<N_MIN; i++)
    {
        if (!s0.empty()) 
        {
            qdma_axis<32,0,0,0> out_s0 = s0.read();
            
            std::cout << "idx: " << i << "\ts0: " << out_s0.data.to_int() << std::endl;
        } else 
        {
            std::cerr << "Error: Stream (s0) underflow at index " << i << std::endl;
            return 1;
        }
    }
    
    return 0;
}