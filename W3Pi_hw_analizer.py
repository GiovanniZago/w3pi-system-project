import config
import numpy as np
import h5py
import matplotlib as mpl
import matplotlib.pyplot as plt
from tqdm import tqdm
import mplhep as hep

hep.style.use("CMS")

MASS_P = 0.13957039

# dataset variables
acc_events = []
matched_events = []
good_events = []

good_matched_masses = []

# algorithm reconstruction variables
hwreco_events = []
hwreco_acc_events = []
hwreco_good_matched_events = []

hwreco_acc_masses = []
hwreco_good_matched_masses = []

num_orbits = 14
num_events = 3564
num_tot_events = num_events * num_orbits
evt_list = np.arange(num_tot_events)

with h5py.File(config.DATA + "/l1Nano_WTo3Pion_PU200.hdf5", "r") as f_gm:
    with h5py.File(config.RESULTS + "/l1Nano_WTo3Pion_PU200_hwreco_fulldata.hdf5", "r") as f_hwreco:
        for (grp_name_gm, grp_gm) in tqdm(f_gm.items()):
            if int(grp_name_gm) not in evt_list:
                continue
            
            if grp_name_gm not in f_hwreco.keys():
                raise ValueError("The event should be inside f_hwreco")
            
            if grp_gm.attrs["is_acc"] == 1:
                acc_events.append(int(grp_name_gm))
            
            if grp_gm.attrs["is_gm"] == 1:
                matched_events.append(int(grp_name_gm))

            if grp_gm.attrs["is_good"] == 1:
                good_events.append(int(grp_name_gm))

            # get reco data of the current event
            grp_hwreco = f_hwreco[grp_name_gm]
            
            # if event is not reconstructed by the algorithm, skip it
            if np.allclose(grp_hwreco["hwreco_triplet_idxs"], [0, 0, 0]):
                continue
            
            # count the total number of reconstructed triplets
            hwreco_events.append(int(grp_name_gm))

            # count the number of reconstructed triplets that are within acceptance (or not)
            if grp_gm.attrs["is_acc"] == 1:
                hwreco_acc_events.append(int(grp_name_gm))

                # save the reco mass: this is key because when we reconstruct something 
                # with our algorithm, this in reality can occur only with the particles
                # that fall in the acceptance region of the detector
                hwreco_acc_masses.append(grp_hwreco["hwreco_w_mass"][...].item())
            
            # count the number of reconstructed events in which there is a good triplet reconstructed correctly
            if grp_gm.attrs["is_good"] == 1:
                hwreco_right = np.allclose(np.sort(grp_gm.attrs["good_triplet_idxs"]), np.sort(grp_hwreco["hwreco_triplet_idxs"][...]))

                if hwreco_right:
                    hwreco_good_matched_events.append(int(grp_name_gm))
                    hwreco_good_matched_masses.append(grp_hwreco["hwreco_w_mass"][...].item())

                    pt_gen = grp_gm["gen_pi_pt"][...]
                    eta_gen = grp_gm["gen_pi_eta"][...]
                    phi_gen = grp_gm["gen_pi_phi"][...]

                    px0 = pt_gen[0] * np.cos(phi_gen[0])
                    py0 = pt_gen[0] * np.sin(phi_gen[0])
                    pz0 = pt_gen[0] * np.sinh(eta_gen[0])
                    e0 = np.sqrt(px0**2 + py0**2 + pz0**2 + MASS_P**2)

                    px1 = pt_gen[1] * np.cos(phi_gen[1])
                    py1 = pt_gen[1] * np.sin(phi_gen[1])
                    pz1 = pt_gen[1] * np.sinh(eta_gen[1])
                    e1 = np.sqrt(px1**2 + py1**2 + pz1**2 + MASS_P**2)

                    px2 = pt_gen[2] * np.cos(phi_gen[2])
                    py2 = pt_gen[2] * np.sin(phi_gen[2])
                    pz2 = pt_gen[2] * np.sinh(eta_gen[2])
                    e2 = np.sqrt(px2**2 + py2**2 + pz2**2 + MASS_P**2)

                    invariant_mass = np.sqrt((e0 + e1 + e2)**2 - (px0 + px1 + px2)**2 - (py0 + py1 + py2)**2 - (pz0 + pz1 + pz2)**2)
                    good_matched_masses.append(invariant_mass)

acc_events.sort()
matched_events.sort()
good_events.sort()

hwreco_events.sort()
hwreco_acc_events.sort()
hwreco_good_matched_events.sort()

n_acc = len(acc_events)
n_match = len(matched_events)
n_good = len(good_events)

n_hwreco = len(hwreco_events)
n_hwreco_acc = len(hwreco_acc_events)
n_hwreco_good_matched = len(hwreco_good_matched_events)

print("Total number of events: ", num_tot_events)
print(f"Events within acceptance: {n_acc} ({(n_acc / num_tot_events) * 100:.2f} % of total)")
print(f"Events with a matched triplet: {n_match} ({(n_match / num_tot_events) * 100:.2f} % of total, {(n_match / n_acc) * 100:.2f} % of acc)")
print(f"Events with a good triplet: {n_good} ({(n_good / num_tot_events) * 100:.2f} % of total, {(n_good / n_acc) * 100:.2f} % of acc)")
print("\n")
print("Total number of reconstructed events: ", n_hwreco)
print("Reconstructed events within acceptance: ", n_hwreco_acc)
print("Reconstructed events with a correct good triplet: ", n_hwreco_good_matched)

nbins = 40
binarray = np.linspace(60, 100, nbins, dtype=np.float32)

plt.figure()
plt.hist(good_matched_masses, bins=binarray, label="Generated", histtype='step', linestyle="-", lw=3)
plt.hist(hwreco_good_matched_masses, bins=binarray, label="AIE L1 Puppi Reco (VCK5000)", histtype='step', linestyle="-", lw=3)
hep.cms.label(label="Phase-2 Simulation Preliminary", data=True, rlabel="PU 200 (14 TeV)", fontsize=20);
plt.xlabel(r"$m_{3\pi}$ (GeV)")
plt.ylabel(f"Events")
plt.legend(fontsize="16", title=r"$W\to3\pi$")
plt.tight_layout()
plt.savefig(config.FIGURES + "/hist.png")
