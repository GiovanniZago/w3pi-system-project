import numpy as np
import h5py
import pandas as pd
from tqdm import tqdm

with h5py.File("l1Nano_WTo3Pion_PU200_hwreco_fulldata.hdf5", "w") as f_out:
    for ev_idx, ev_raw_data in tqdm(enumerate(pd.read_csv("l1Nano_WTo3Pion_PU200_hwreco_fulldata.csv", header=None, names=["data"], chunksize=4))):
        aiereco_triplet_idxs = ev_raw_data["data"].to_numpy()[:3]
        aiereco_w_mass = ev_raw_data["data"].to_numpy()[3]

        grp = f_out.create_group(f"{ev_idx}")
        grp.create_dataset("hwreco_triplet_idxs", data=aiereco_triplet_idxs, dtype=np.int16)
        grp.create_dataset("hwreco_w_mass", data=aiereco_w_mass, dtype=np.float32)