#include <adf.h>
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include <aie_api/utils.hpp>
#include "kernels.h"
#include <math.h>

using namespace adf;

void unpack_filter_iso(input_stream<int16> * __restrict in0, input_stream<int16> * __restrict in1, output_stream<float> * __restrict out)
{   
    // data variables
    aie::vector<int16, V_SIZE> pts[P_BUNCHES] = { aie::broadcast<int16, V_SIZE>(0) };
    aie::vector<int16, V_SIZE> etas[P_BUNCHES] = { aie::broadcast<int16, V_SIZE>(0) };
    aie::vector<int16, V_SIZE> phis[P_BUNCHES] = { aie::broadcast<int16, V_SIZE>(0) }; 
    aie::vector<int16, V_SIZE> pdg_ids[P_BUNCHES] = { aie::broadcast<int16, V_SIZE>(0) };

    // filter pt and pdg id
    aie::mask<V_SIZE> is_min_pt[P_BUNCHES], is_med_pt[P_BUNCHES], is_hig_pt[P_BUNCHES], is_pdg_id[P_BUNCHES];
    int16 min_pt_counter=0, med_pt_counter=0, hig_pt_counter=0;
    aie::vector<int16, N_MIN> is_filter = aie::broadcast<int16, N_MIN>(0);
    int16 is_filter_idx=0;

    // auxiliary variables
    int16 out_idx, in_idx;

    for (int i=0; i<P_BUNCHES; i++)
    {
        pts[i] = readincr_v<V_SIZE>(in0);
        is_min_pt[i] = aie::geq(pts[i], MIN_PT);
        is_med_pt[i] = aie::geq(pts[i], MED_PT);
        is_hig_pt[i] = aie::geq(pts[i], HIG_PT);
        min_pt_counter += is_min_pt[i].count();
        med_pt_counter += is_med_pt[i].count();
        hig_pt_counter += is_hig_pt[i].count();

        etas[i] = readincr_v<V_SIZE>(in1);
    }

    for (int i=0; i<P_BUNCHES; i++)
    {
        phis[i] = readincr_v<V_SIZE>(in0);

        pdg_ids[i] = readincr_v<V_SIZE>(in1);
        is_pdg_id[i] = aie::geq(ptg_ids[i], 2);
    }

    bool skip_event = false;
    if ((min_pt_counter < 3) | (med_pt_counter < 2) | (hig_pt_counter < 1)) skip_event = true;

    // isolation variables
    int16 pt_sum=0;
    aie::vector<int16, V_SIZE> d_eta, d_phi;
    aie::vector<int16, V_SIZE> pt_to_sum;
    aie::vector<int32, V_SIZE> dr2;
    aie::vector<float, V_SIZE> dr2_float;
    aie::accum<acc48, V_SIZE> acc;
    aie::accum<accfloat, V_SIZE> acc_float;
    aie::mask<V_SIZE> is_ge_mindr2, is_le_maxdr2, pt_cut_mask;

    // output variables
    aie::vector<int16, N_MIN> pts_iso_filter = aie::broadcast<int16, N_MIN>(0);
    aie::vector<int16, N_MIN> etas_iso_filter = aie::broadcast<int16, N_MIN>(0);
    aie::vector<int16, N_MIN> phis_iso_filter = aie::broadcast<int16, N_MIN>(0);
    aie::vector<int16, N_MIN> pdg_ids_iso_filter = aie::broadcast<int16, N_MIN>(0);
    aie::vector<int16, N_MIN> is_iso_filter = aie::broadcast<int16, N_MIN>(0);

    for (int i=0; i<N_MIN; i++)
    {   
        if (skip_event) continue;
        if (!is_filter[i]) continue;

        pt_sum = 0;
        out_idx = (is_filter[i] - 1) / V_SIZE;
        in_idx = (is_filter[i] - 1) % V_SIZE;

        for (int k=0; k<P_BUNCHES; k++)
        chess_prepare_for_pipelining
        {
            d_eta = aie::sub(etas[out_idx][in_idx], etas[k]);
            d_phi = aie::sub(phis[out_idx][in_idx], phis[k]);

            acc = aie::mul_square(d_eta); // acc = d_eta ^ 2
            acc = aie::mac_square(acc, d_phi); // acc = acc + d_phi ^ 2
            dr2 = acc.to_vector<int32>(0); // convert accumulator into vector
            dr2_float = aie::to_float(dr2, 0);
            acc_float = aie::mul(dr2_float, F_CONV2); // dr2_float = dr2_int * ((pi / 720) ^ 2)
            dr2_float = acc_float.to_vector<float>(0);

            is_ge_mindr2 = aie::ge(dr2_float, MINDR2_FLOAT);
            is_le_maxdr2 = aie::le(dr2_float, MAXDR2_FLOAT);
            pt_cut_mask = is_ge_mindr2 & is_le_maxdr2;

            pt_to_sum = aie::select(zeros_vector, pts[k], pt_cut_mask); // select only the pts that fall in the desired range
            pt_sum += aie::reduce_add(pt_to_sum); // update the pt sum
        }

        if (pt_sum <= (pts[out_idx][in_idx] * MAX_ISO))
        {
            pts_iso_filter[i] = pts[out_idx][in_idx];
            etas_iso_filter[i] = etas[out_idx][in_idx];
            phis_iso_filter[i] = phis[out_idx][in_idx];
            pdg_ids_iso_filter[i] = pdg_ids[out_idx][in_idx];
            is_iso_filter[i] = is_filter[i];
        }
    }

    // ang sep specific variables
    int16 d_eta, d_phi;
    int32 dr2;
    float dr2_float;
    int16 eta_hig_pt_target0, eta_hig_pt_target1, eta_hig_pt_target2;
    int16 phi_hig_pt_target0, phi_hig_pt_target1, phi_hig_pt_target2;
    int16 pt_hig_pt_target0, pt_hig_pt_target1, pt_hig_pt_target2;

    // triplet variables
    int16 charge_tot, charge0, charge1, charge2;
    int16 triplet_score = 0, best_triplet_score = 0;
    float mass0, mass1, mass2;
    float px0, py0, pz0, px1, py1, pz1, px2, py2, pz2;
    float e0, e1, e2;
    float px_tot, py_tot, pz_tot, e_tot;
    float x, sinh;
    float invariant_mass=0;

    aie::vector<float, 4> triplet = aie::zeros<float, 4>();

    aie::mask<N_MIN> is_iso_filter_mask = aie::gt(is_iso_filter, (int16) 0);
    int16 n_iso_filter = is_iso_filter_mask.count();
    bool skip_event = (n_iso_filter < 3);

    for (int i0=0; i0<N_MIN; i0++)
    {
        if (skip_event) continue;
        if (!is_iso_filter[i0]) continue;

        pt_hig_pt_target0 = pts_iso_filter[i0];
        eta_hig_pt_target0 = etas_iso_filter[i0];
        phi_hig_pt_target0 = phis_iso_filter[i0];

        for (int i1=0; i1<N_MIN; i1++)
        {
            if (i1 == i0) continue;
            if (!is_iso_filter[i1]) continue;

            d_eta = etas_iso_filter[i1] - eta_hig_pt_target0;
            d_phi = phis_iso_filter[i1] - phi_hig_pt_target0;
            d_phi = (d_phi <= PI) ? ((d_phi >= MPI) ? d_phi : d_phi + TWOPI) : d_phi + MTWOPI;

            dr2 = d_eta * d_eta + d_phi * d_phi;
            dr2_float = dr2 * F_CONV2;

            if (dr2_float < MINDR2_ANGSEP_FLOAT) continue;

            pt_hig_pt_target1 = pts_iso_filter[i1];
            eta_hig_pt_target1 = etas_iso_filter[i1];
            phi_hig_pt_target1 = phis_iso_filter[i1];

            for (int i2=0; i2<N_MIN; i2++)
            {
                if (i2 == i0) continue;
                if (i2 == i1) continue;
                if (!is_iso_filter[i2]) continue;

                d_eta = etas_iso_filter[i2] - eta_hig_pt_target1;
                d_phi = phis_iso_filter[i2] - phi_hig_pt_target1;
                d_phi = (d_phi <= PI) ? ((d_phi >= MPI) ? d_phi : d_phi + TWOPI) : d_phi + MTWOPI;

                dr2 = d_eta * d_eta + d_phi * d_phi;
                dr2_float = dr2 * F_CONV2;

                if (dr2_float < MINDR2_ANGSEP_FLOAT) continue;

                pt_hig_pt_target2 = pts_iso_filter[i2];
                eta_hig_pt_target2 = etas_iso_filter[i2];
                phi_hig_pt_target2 = phis_iso_filter[i2];

                charge0 = (pdg_ids_iso_filter[i0] >= 4) ? ((pdg_ids_iso_filter[i0] == 4) ? -1 : 1) : ((pdg_ids_iso_filter[i0] == 2) ? -1 : 1);
                charge1 = (pdg_ids_iso_filter[i1] >= 4) ? ((pdg_ids_iso_filter[i1] == 4) ? -1 : 1) : ((pdg_ids_iso_filter[i1] == 2) ? -1 : 1);
                charge2 = (pdg_ids_iso_filter[i2] >= 4) ? ((pdg_ids_iso_filter[i2] == 4) ? -1 : 1) : ((pdg_ids_iso_filter[i2] == 2) ? -1 : 1);

                charge_tot = charge0 + charge1 + charge2;

                if ((charge_tot != 1) & (charge_tot != -1)) continue;

                px0 = pt_hig_pt_target0 * PT_CONV * aie::cos(phi_hig_pt_target0 * F_CONV);
                py0 = pt_hig_pt_target0 * PT_CONV * aie::sin(phi_hig_pt_target0 * F_CONV);
                x = eta_hig_pt_target0 * F_CONV;
                sinh = x + ((x * x * x) / 6);
                pz0 = pt_hig_pt_target0 * PT_CONV * sinh;
                e0 = aie::sqrt(px0 * px0 + py0 * py0 + pz0 * pz0 + MASS_P * MASS_P);
            
                px1 = pt_hig_pt_target1 * PT_CONV * aie::cos(phi_hig_pt_target1 * F_CONV);
                py1 = pt_hig_pt_target1 * PT_CONV * aie::sin(phi_hig_pt_target1 * F_CONV);
                x = eta_hig_pt_target1 * F_CONV;
                sinh = x + ((x * x * x) / 6);
                pz1 = pt_hig_pt_target1 * PT_CONV * sinh;
                e1 = aie::sqrt(px1 * px1 + py1 * py1 + pz1 * pz1 + MASS_P * MASS_P);

                px2 = pt_hig_pt_target2 * PT_CONV * aie::cos(phi_hig_pt_target2 * F_CONV);
                py2 = pt_hig_pt_target2 * PT_CONV * aie::sin(phi_hig_pt_target2 * F_CONV);
                x = eta_hig_pt_target2 * F_CONV;
                sinh = x + ((x * x * x) / 6);
                pz2 = pt_hig_pt_target2 * PT_CONV * sinh;
                e2 = aie::sqrt(px2 * px2 + py2 * py2 + pz2 * pz2 + MASS_P * MASS_P);

                px_tot = px0 + px1 + px2;
                py_tot = py0 + py1 + py2;
                pz_tot = pz0 + pz1 + pz2;
                e_tot = e0 + e1 + e2;

                invariant_mass = aie::sqrt(e_tot * e_tot - px_tot * px_tot - py_tot * py_tot - pz_tot * pz_tot);

                if ((invariant_mass < MIN_MASS) | (invariant_mass > MAX_MASS)) continue;

                triplet_score = pt_hig_pt_target0 + pt_hig_pt_target1 + pt_hig_pt_target2;

                if (triplet_score > best_triplet_score)
                {   
                    best_triplet_score = triplet_score;
                    triplet[0] = is_iso_filter[i0] - 1;
                    triplet[1] = is_iso_filter[i1] - 1;
                    triplet[2] = is_iso_filter[i2] - 1;
                    triplet[3] = invariant_mass;
                }
            }
        }
    }

    writeincr(out, triplet);
}