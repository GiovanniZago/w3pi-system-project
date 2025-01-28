#include <adf.h>
#include "kernels.h"

using namespace adf;

class Dr2Graph : public graph 
{
    private:
        kernel dr2_11_k;

    public:
        input_plio in_11;
        output_plio out_11;

        Dr2Graph() 
        {
            dr2_11_k = kernel::create(dr2_11);

            in_11 = input_plio::create("in_11", plio_64_bits, "etaphi_11_plio_64_if_16.txt", 360);
            out_11 = output_plio::create("out_11", plio_32_bits, "out_11.csv", 360);

            // PL inputs
            connect<stream>(in_11.out[0], dr2_11_k.in[0]);

            // PL outputs
            connect<stream>(dr2_11_k.out[0], out_11.in[0]);

            // sources and runtime ratios
            source(dr2_11_k) = "kernels.cpp";
            runtime<ratio>(dr2_11_k) = 1;
        }
};