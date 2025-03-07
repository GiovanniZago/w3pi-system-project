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

# algorithm reconstruction variables
hwreco_events = []
hwreco_acc_events = []
hwreco_notacc_events = []
hwreco_good_events = []

# histogram variables
good_masses = []
hwreco_good_masses = []

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

                pt = grp_gm["pt"][...]
                eta = grp_gm["eta"][...]
                phi = grp_gm["phi"][...]

                pt0 = pt[grp_gm.attrs["good_triplet_idxs"][0]]
                pt1 = pt[grp_gm.attrs["good_triplet_idxs"][1]]
                pt2 = pt[grp_gm.attrs["good_triplet_idxs"][2]]

                eta0 = eta[grp_gm.attrs["good_triplet_idxs"][0]]
                eta1 = eta[grp_gm.attrs["good_triplet_idxs"][1]]
                eta2 = eta[grp_gm.attrs["good_triplet_idxs"][2]]

                phi0 = phi[grp_gm.attrs["good_triplet_idxs"][0]]
                phi1 = phi[grp_gm.attrs["good_triplet_idxs"][1]]
                phi2 = phi[grp_gm.attrs["good_triplet_idxs"][2]]

                px0 = pt0 * np.cos(phi0)
                py0 = pt0 * np.sin(phi0)
                pz0 = pt0 * np.sinh(eta0)
                e0 = np.sqrt(px0**2 + py0**2 + pz0**2 + MASS_P**2)

                px1 = pt1 * np.cos(phi1)
                py1 = pt1 * np.sin(phi1)
                pz1 = pt1 * np.sinh(eta1)
                e1 = np.sqrt(px1**2 + py1**2 + pz1**2 + MASS_P**2)

                px2 = pt2 * np.cos(phi2)
                py2 = pt2 * np.sin(phi2)   
                pz2 = pt2 * np.sinh(eta2)
                e2 = np.sqrt(px2**2 + py2**2 + pz2**2 + MASS_P**2)

                invariant_mass = np.sqrt((e0 + e1 + e2)**2 - (px0 + px1 + px2)**2 - (py0 + py1 + py2)**2 - (pz0 + pz1 + pz2)**2)
                good_masses.append(invariant_mass)

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

            else:
                hwreco_notacc_events.append(int(grp_name_gm))
            
            # count the number of reconstructed events in which there is a good triplet reconstructed correctly
            if grp_gm.attrs["is_good"] == 1:
                hwreco_right = np.allclose(np.sort(grp_gm.attrs["gm_triplet_idxs"]), np.sort(grp_hwreco["hwreco_triplet_idxs"][...]))

                if hwreco_right:
                    hwreco_good_events.append(int(grp_name_gm))
                    hwreco_good_masses.append(grp_hwreco["hwreco_w_mass"][...].item())

acc_events.sort()
matched_events.sort()
good_events.sort()

hwreco_events.sort()
hwreco_acc_events.sort()
hwreco_notacc_events.sort()
hwreco_good_events.sort()

n_acc = len(acc_events)
n_match = len(matched_events)
n_good = len(good_events)

n_hwreco = len(hwreco_events)
n_hwreco_acc = len(hwreco_acc_events)
n_hwreco_notacc = len(hwreco_notacc_events)
n_hwreco_good = len(hwreco_good_events)

print("Total number of events: ", num_tot_events)
print("Events within acceptance: ", n_acc)
print("Events with a matched triplet: ", n_match)
print("Events with a good triplet: ", n_good)
print("\n")
print("Total number of reconstructed events: ", n_hwreco)
print("Reconstructed events within acceptance: ", n_hwreco_acc)
print("Reconstructed events NOT within acceptance: ", n_hwreco_notacc)
print("Reconstructed events with a correct good triplet: ", n_hwreco_good)

# plt.figure()
# barplot = plt.bar(["correct", "partial", "wrong", "not_reco"], [n_cor, n_par, n_wrong, n_not])
# plt.bar_label(barplot, labels=[f"{n_cor}/{num_good_events}", f"{n_par}/{num_good_events}", f"{n_wrong}/{num_good_events}", f"{n_not}/{num_good_events}"], 
#               label="AIE (VCK5000)")
# hep.cms.label(label="Phase-2 Simulation Preliminary", data=True, rlabel="14 TeV (PU 200)", fontsize=18);
# plt.xlabel("Triplet reco category")
# plt.ylabel("Event counts")
# plt.ylim(0, max(n_cor, n_par, n_wrong, n_not) * 1.1)
# plt.tight_layout()
# plt.savefig(config.FIGURES + "/barplot.png")

nbins = 40
binarray = np.linspace(60, 100, nbins, dtype=np.float32)

plt.figure()
plt.hist(good_masses, bins=binarray, label="L1 Scouting, GEN-matched", histtype='step', linestyle="-", lw=3)
plt.hist(hwreco_good_masses, bins=binarray, label="AIE (VCK5000)", histtype='step', linestyle="-", lw=3)
hep.cms.label(label="Phase-2 Simulation Preliminary", data=True, rlabel="PU 200 (14 TeV)", fontsize=18);
plt.xlabel(r"$m_{3\pi}$ (GeV)")
plt.ylabel(f"Events")
plt.legend(fontsize="16")
plt.tight_layout()
plt.savefig(config.FIGURES + "/hist.png")
