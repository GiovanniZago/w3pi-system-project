#include <iostream>
#include "hls_stream.h"
#include "ap_int.h"
#include "mm2s"

#define TEST_SIZE 112

int main() 
{
    ap_int<64> mem[TEST_SIZE * 2];
    hls::stream<qdma_axis<32,0,0,0>> s0, s1;
    int mem_offset = 0;
    
    // Initialize memory with test data
    for (int i = 0; i < TEST_SIZE * 2; i++) {
        mem[i] = i; // Simple incremental data for testing
    }
    
    // Call the module
    mm2s(mem, s0, s1, mem_offset);
    
    // Check the output
    for (int i = 0; i < TEST_SIZE * 2; i++) {
        if (!s0.empty() && !s1.empty()) {
            qdma_axis<32,0,0,0> out_s0 = s0.read();
            qdma_axis<32,0,0,0> out_s1 = s1.read();
            
            std::cout << "s0: " << out_s0.data.to_int() << "\ts1: " << out_s1.data.to_int() << std::endl;
        } else {
            std::cerr << "Error: Stream underflow at index " << i << std::endl;
            return 1;
        }
    }
    
    std::cout << "Test Passed!" << std::endl;
    return 0;
}
