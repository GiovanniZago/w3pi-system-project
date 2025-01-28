#include "kernels.h"

void dr2_11(input_stream<int16> * __restrict in, output_stream<int32> * __restrict out)
{
    // data variables
    aie::vector<int16, V_SIZE> etas[P_BUNCHES], phis[P_BUNCHES];

    // auxiliary variables
    const aie::vector<int16, V_SIZE> pi_vector     = aie::broadcast<int16, V_SIZE>(PI_11);
    const aie::vector<int16, V_SIZE> mpi_vector    = aie::broadcast<int16, V_SIZE>(MPI_11);
    const aie::vector<int16, V_SIZE> twopi_vector  = aie::broadcast<int16, V_SIZE>(TWOPI_11);
    const aie::vector<int16, V_SIZE> mtwopi_vector = aie::broadcast<int16, V_SIZE>(MTWOPI_11);

    // output variables
    aie::vector<int32, V_SIZE> dr2s[P_BUNCHES];

    for(int i=0; i<P_BUNCHES; i++)
    {
        etas[i].insert(0, readincr_v<V_SIZE>(in));
    }

    for(int i=0; i<P_BUNCHES; i++)
    {
        phis[i].insert(0, readincr_v<V_SIZE>(in));
    }

    int16 eta_cur = etas[0][0];
    int16 phi_cur = phis[0][0];

    for (int i=0; i<P_BUNCHES; i++)
    {
        // calculate d_eta
        aie::vector<int16, V_SIZE> d_eta = aie::sub(eta_cur, etas[i]);

        // calculate d_phi
        aie::vector<int16, V_SIZE> d_phi        = aie::sub(phi_cur, phis[i]);
        aie::vector<int16, V_SIZE> d_phi_ptwopi = aie::add(d_phi, twopi_vector); // d_eta + 2 * pi
        aie::vector<int16, V_SIZE> d_phi_mtwopi = aie::add(d_phi, mtwopi_vector); // d_eta - 2 * pi
        aie::mask<V_SIZE> is_gt_pi              = aie::gt(d_phi, pi_vector);
        aie::mask<V_SIZE> is_lt_mpi             = aie::lt(d_phi, mpi_vector);
        d_phi                                   = aie::select(d_phi, d_phi_ptwopi, is_lt_mpi); // select element from d_phi if element is geq of -pi, otherwise from d_phi_ptwopi
        d_phi                                   = aie::select(d_phi, d_phi_mtwopi, is_gt_pi); // select element from d_phi if element is leq of pi, otherwise from d_phi_mtwopi

        // calculate dr2
        aie::accum<acc48, V_SIZE> acc     = aie::mul_square(d_eta); // acc = d_eta ^ 2
                                  acc     = aie::mac_square(acc, d_phi); // acc = acc + d_phi ^ 2
                                  dr2s[i] = acc.to_vector<int32>(0); // convert accumulator into vector
    }

    for (int i=0; i<P_BUNCHES; i++)
    {
        writeincr(out, dr2s[i]);
    }
}   

void dr2_16(input_stream<int16> * __restrict in0, input_stream<int16> * __restrict in1, output_stream<int32> * __restrict out)
{
    // data variables
    aie::vector<int16, V_SIZE> etas[P_BUNCHES], phis[P_BUNCHES];

    // output variables
    aie::vector<int32, V_SIZE> dr2s[P_BUNCHES];

    for(int i=0; i<P_BUNCHES; i++)
    {
        etas[i] = readincr_v<V_SIZE>(in0);
        phis[i] = readincr_v<V_SIZE>(in1);
    }

    int16 eta_cur = etas[0][0];
    int16 phi_cur = phis[0][0];

    for (int i=0; i<P_BUNCHES; i++)
    {
        // calculate d_eta
        aie::vector<int16, V_SIZE> d_eta = aie::sub(eta_cur, etas[i]);

        // calculate d_phi
        aie::vector<int16, V_SIZE> d_phi = aie::sub(phi_cur, phis[i]);

        // calculate dr2
        aie::accum<acc48, V_SIZE> acc     = aie::mul_square(d_eta); // acc = d_eta ^ 2
                                  acc     = aie::mac_square(acc, d_phi); // acc = acc + d_phi ^ 2
                                  dr2s[i] = acc.to_vector<int32>(0); // convert accumulator into vector
    }

    for (int i=0; i<P_BUNCHES; i++)
    {
        writeincr(out, dr2s[i]);
    }
}   