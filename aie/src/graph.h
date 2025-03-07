#include <adf.h>
#include "kernels.h"

using namespace adf;

class W3PiGraph : public graph 
{
    private:
        kernel w3pi_k;

    public:
        input_plio in0;
        input_plio in1;
        output_plio out;

        W3PiGraph() 
        {
            w3pi_k = kernel::create(w3pi);

            in0 = input_plio::create("in0", plio_32_bits, "zoo0.txt", 360);
            in1 = input_plio::create("in1", plio_32_bits, "zoo1.txt", 360);
            out = output_plio::create("out", plio_32_bits, "out.txt", 360);

            // PL inputs
            connect<stream>(in0.out[0], w3pi_k.in[0]);
            connect<stream>(in1.out[0], w3pi_k.in[1]);

            // PL outputs
            connect<stream>(w3pi_k.out[0], out.in[0]);

            // sources and runtime ratios
            source(w3pi_k) = "kernels.cpp";
            runtime<ratio>(w3pi_k) = 1;
        }
};