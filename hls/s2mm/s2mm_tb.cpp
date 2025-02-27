#include <iostream>
#include <vector>

#include "src/s2mm.h"

#define TRIPLET_VSIZE 4

int main()
{
    hls::stream<qdma_axis<32,0,0,0>> s;

    float input_triplet[TRIPLET_VSIZE] = {3.0, 5.0, 6.0, 82.82484436035156};

    for (unsigned int i = 0; i < TRIPLET_VSIZE; i++)
    {
        qdma_axis<32,0,0,0> temp;
        temp.set_data(*reinterpret_cast<ap_uint<32>*>(&input_triplet[i]));
        s.write(temp);
    }

    ap_uint<32> output_triplet[TRIPLET_VSIZE];
    s2mm(output_triplet, s, 0);

    for (unsigned int i = 0; i < TRIPLET_VSIZE; i++)
    {
        float val = *reinterpret_cast<float*>(&output_triplet[i]);
        std::cout << val << std::endl;
    }

    return 0;
}