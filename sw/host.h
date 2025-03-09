#include <fstream>
#include <cstring>
#include <iostream>
#include <chrono>
#include <unordered_map>

#include "ap_int.h"
#include "experimental/xrt_kernel.h"
#include "experimental/xrt_graph.h"

#define EV_SIZE 224
#define BLOCK_SIZE 16
#define NUM_BLOCKS EV_SIZE / BLOCK_SIZE 
#define TRIPLET_VSIZE 4

static const int NUM_EVENTS = 3564;
static const int NUM_ORBITS = 14;

static const bool DEBUG = false;
static const bool PRINT_TIME = true;

void checkpoint(const std::string& flag, const bool debug_enabled) 
{   
    if (!debug_enabled) return;

    static const std::unordered_map<std::string, std::string> messages = {
        {"XCLBIN_UUID", "Checkpoint: Got xclbin_uuid."},
        {"KERNEL_INSTANCE", "Checkpoint: PL kernels are instantiated."},
        {"BUFFER_OBJECT", "Checkpoint: Buffer objects created."},
        {"MEM_ALLOC", "Checkpoint: Buffer allocated on the device."},
        {"MM2S_DONE", "Checkpoint: mm2s kernel finished execution"},
        {"S2MM_DONE", "Checkpoint: s2mm kernel finished execution"},
        {"MEM_READOUT", "Checkpoint: Buffer readout from the device"}
    };

    auto it = messages.find(flag);
    if (it != messages.end()) {
        std::cout << it->second << std::endl;
    } else {
        std::cout << "Checkpoint: Unknown flag (" << flag << ")" << std::endl;
    }
}