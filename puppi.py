import sys
import traceback
import os
import struct
import h5py
import numpy as np
import uproot
import awkward as ak
from tqdm import tqdm

class PuppiData:
    line_size = 8 # each line is 8 bytes
    data_path = "/home/giovanni/pod/thesis/code/scripts-sources/W3Pi-selection-algorithm/data/"
    head_columns = ["idx", "vld_header", "err_bit", "lr_number", "orbit_cnt", "bx_cnt", "n_cand"]
    part_columns = ["idx", "pdg_id", "phi", "eta", "pt"]


    def __init__(self, file_name):
        self.file_name = file_name
        self.file_path = self.data_path + file_name
        file_stats = os.stat(self.file_path)
        self.file_size = file_stats.st_size # file size in Bytes
        self.file_rows = int(self.file_size / self.line_size) # each row has line_size Bytes
        self.file = None

    def __enter__(self):
        self.open_file()
        return self
    
    def __exit__(self, exc_type, exc_value, tb):
        if exc_type is not None:
            traceback.print_exception(exc_type, exc_value, tb)

        self.close_file()
        return True

    def open_file(self):
        self.file = open(self.file_path, "rb")

    def close_file(self):
        if self.file:
            self.file.close()

    def _get_line(self, idx):
        if not self.file:
            raise ValueError("File not opened. Call self.open_file() first or enter the context.")
        
        byte_position = idx * self.line_size
        self.file.seek(byte_position)
        return self.file.read(self.line_size)

    def _get_lines(self, idx_start, idx_end):
        lines_str = []
        for idx in range(idx_start, idx_end):
            lines_str.append(self._get_line(idx))

        return lines_str

    def get_lines_data(self, idx_start, idx_end=None):
        if not idx_end:
            lines_str = [self._get_line(idx_start)]
            idx_end = idx_start + 1
        else:
            lines_str = self._get_lines(idx_start, idx_end)
        
        headers_data = []
        particles_data = []
        
        for idx, line_str in zip(list(range(idx_start, idx_end)), lines_str):
            header = self._unpack_header(line_str)

            if (int.from_bytes(line_str) == 0): 
                is_null = True
            else:
                is_null = False
            
            if ((not is_null) and (header["err_bit"] == 0) and (header["lr_num"] == 0)
                    and (header["orbit_cnt"] < 100_000) and (header["bx_cnt"] <= 3563) and (header["n_cand"] >= 0)):
                headers_data.append([idx, header["vld_header"], header["err_bit"], header["lr_num"], 
                                     header["orbit_cnt"], header["bx_cnt"], header["n_cand"]])
            else:
                particle = self._unpack_particle(line_str)
                if is_null: particle["pdg_id"] = 0
                particles_data.append([idx, particle["pdg_id"], particle["phi"], particle["eta"], particle["pt"]])
                
        return np.array(headers_data), np.array(particles_data)

        
    def print_lines_data(self, idx_start, idx_end=None):
        headers_data, particles_data = self.get_lines_data(idx_start, idx_end)

        if len(headers_data) > 0:
            self._print_table(headers_data, self.head_columns)
            print("\n\n")

        self._print_table(particles_data, self.part_columns)

    def pad_file(self, ev_size, out_file_name, include_headers=False):
        if not self.file:
            raise ValueError("File not opened. Call self.open_file() first or enter the context.")
        
        ii          = 0
        is_header   = True
        p_count     = 0
        p_counter   = 0
        p_to_append = 0

        with open(self.data_path + out_file_name, "wb") as out_file:
            while True:
                row_bytes = self._get_line(ii)
                
                if include_headers:
                    out_file.write(row_bytes)

                elif (not is_header):
                    out_file.write(row_bytes)

                ii += 1

                if not row_bytes: 
                    print(f"While loop ended at line {ii}")
                    break

                if is_header:
                    row_data = self._unpack_header(row_bytes)
                    p_count = row_data["n_cand"]
                    p_to_append = ev_size - p_count
                                                        
                    if p_count == 0:
                        for _ in range(p_to_append):
                            out_file.write(b"\x00" * self.line_size)

                        continue

                    if p_to_append < 0:
                        print(f"Header at row index {ii} has more particles than ev_size ({ev_size})")
                        p_count = ev_size
                        p_to_append = 0

                    is_header = False

                else:
                    p_counter += 1

                    if p_counter == p_count:
                        for _ in range(p_to_append):
                            out_file.write(b"\x00" * self.line_size)
                        is_header = True
                        p_counter = 0


    def to_aiecsv(self):
        if not self.file:
            raise ValueError("File not opened. Call self.open_file() first or enter the context.")
        
        file_out_name_H = str.split(self.file_name, ".")[0] + "_H.csv"
        file_out_name_L = str.split(self.file_name, ".")[0] + "_L.csv"
        
        with open(self.data_path + "/aie_data/" + file_out_name_H, "w") as puppi_csv_H:
            with open(self.data_path + "/aie_data/" + file_out_name_L, "w") as puppi_csv_L:
                puppi_csv_H.write("CMD,D,TLAST,TKEEP\n")
                puppi_csv_L.write("CMD,D,TLAST,TKEEP\n")

                while True:
                    row_bytes = self.file.read(self.line_size)

                    if not row_bytes:
                        break

                    row_data = struct.unpack("ii", row_bytes)
                    puppi_csv_H.write("DATA," + f"{str(row_data[1])}," + "0," + "-1\n")
                    puppi_csv_L.write("DATA," + f"{str(row_data[0])}," + "0," + "-1\n")

    def to_aiecsv64(self):
        if not self.file:
            raise ValueError("File not opened. Call self.open_file() first or enter the context.")
        
        file_out_name = str.split(self.file_name, ".")[0] + ".csv"
        
        with open(self.data_path + "/aie_data/" + file_out_name, "w") as puppi_csv:
            puppi_csv.write("CMD,D,TLAST,TKEEP\n")

            while True:
                row_bytes = self.file.read(self.line_size)

                if not row_bytes:
                    break

                row_data = struct.unpack("q", row_bytes)
                puppi_csv.write("DATA," + f"{str(row_data[0])}," + "0," + "-1\n")

    def to_aiecsv64_unpacked(self):
        if not self.file:
            raise ValueError("File not opened. Call self.open_file() first or enter the context.")
        
        file_out_name = str.split(self.file_name, ".")[0] + "_unpacked.csv"
        
        with open(self.data_path + "/aie_data/" + file_out_name, "w") as puppi_csv:
            puppi_csv.write("CMD,D,D,D,D,TLAST,TKEEP\n")

            while True:
                row_bytes = self.file.read(self.line_size)

                if not row_bytes:
                    break
                
                row_data = struct.unpack("q", row_bytes)

                if row_data[0]:
                    next_row_bytes = self.file.read(self.line_size) # reading the next line causes the cursor to advance of 8 bytes 
                    next_row_data = struct.unpack("q", next_row_bytes)
                    
                    # check if the next line contains only a zero or if it contains data
                    if next_row_data[0]:
                        part_dict = self._unpack_particle(row_bytes)
                        part_dict["pdg_id"] = self._get_raw_pdgid(part_dict["pdg_id"])
                        puppi_csv.write("DATA," + f"{part_dict['pt']}," + f"{part_dict['eta']}," + f"{part_dict['phi']}," + f"{part_dict['pdg_id']}," + "0,-1\n")

                    else:
                        part_dict = self._unpack_particle(row_bytes)
                        part_dict["pdg_id"] = self._get_raw_pdgid(part_dict["pdg_id"])
                        puppi_csv.write("DATA," + f"{part_dict['pt']}," + f"{part_dict['eta']}," + f"{part_dict['phi']}," + f"{part_dict['pdg_id']}," + "1,-1\n") # assert tlast here

                    self.file.seek(-8, 1) # put the curson of the file back to the previous position, namely shift it 8 bytes back

    def to_hdf5(self, ev_size, has_headers=False):
        if not self.file:
            raise ValueError("File not opened. Call self.open_file() first or enter the context.")
        
        file_out_name = str.split(self.file_name, ".")[0] + ".hdf5"
        
        block_size = ev_size if (not has_headers) else ev_size + 1
        foo = self.file_rows % block_size

        if foo != 0:
            raise ValueError("The number of rows in the file is not a multiple of block_size.")

        n_events = int(self.file_rows / block_size)
        
        idxs_start = [x * block_size for x in range(n_events)]
        idxs_end = [x * block_size for x in range(1, n_events+1)]
        
        with h5py.File(self.data_path + file_out_name, "w") as f:
            f.attrs["columns"] = self.part_columns # save the columns name as metadata  
            f.attrs["ev_size"] = ev_size
            
            for idx_event, (idx_start, idx_end) in enumerate(zip(idxs_start, idxs_end)):
                head_data, part_data = self.get_lines_data(idx_start, idx_end)
                part_data = np.array(part_data)
                d_name = str(idx_event)

                # save all the rows but skip the first column since it's the index
                f.create_dataset(d_name, data=part_data[:,1:])

                # save the header if present
                if has_headers:
                    if (len(head_data.shape) > 1):
                        ValueError(f"More than one header detected in Event #{idx_event}")
                        
                    f.create_dataset(d_name + "_header", data=head_data[1:])   

    def to_uproot(self, ev_size, has_headers=False):
        if not self.file:
            raise ValueError("File not opened. Call self.open_file() first or enter the context.")
        
        file_out_name = str.split(self.file_name, ".")[0] + ".root"

        block_size = ev_size if (not has_headers) else ev_size + 1
        foo = self.file_rows % block_size

        if foo != 0:
            raise ValueError("The number of rows in the file is not a multiple of block_size.")
        
        n_events = int(self.file_rows / block_size)

        idxs_start = [x * block_size for x in range(n_events)]
        idxs_end = [x * block_size for x in range(1, n_events+1)]

        if has_headers:
            # create headers numpy arrays
            vld_header_array = np.zeros(n_events)
            err_bit_array    = np.zeros(n_events)
            lr_num_array     = np.zeros(n_events)
            orbit_cnt_array  = np.zeros(n_events)
            bx_cnt_array     = np.zeros(n_events)
            n_cand_array     = np.zeros(n_events)

        # create particles numpy arrays
        pts_array     = np.zeros((n_events, ev_size), dtype="int32")
        etas_array    = np.zeros((n_events, ev_size), dtype="int32")
        phis_array    = np.zeros((n_events, ev_size), dtype="int32")
        pdg_ids_array = np.zeros((n_events, ev_size), dtype="int32")

        with uproot.recreate(self.data_path + file_out_name) as f:
            for idx_event, (idx_start, idx_end) in enumerate(zip(idxs_start, idxs_end)):
                head_data, part_data = self.get_lines_data(idx_start, idx_end)
                
                if has_headers:
                    if (len(head_data.shape) > 1):
                        ValueError(f"More than one header detected in Event #{idx_event}")

                    # update headers arrays
                    vld_header_array[idx_event] = head_data[1]
                    err_bit_array[idx_event]    = head_data[2]
                    lr_num_array[idx_event]     = head_data[3]
                    orbit_cnt_array[idx_event]  = head_data[4]
                    bx_cnt_array[idx_event]     = head_data[5]
                    n_cand_array[idx_event]     = head_data[6]

                # update particles arrays
                pdg_ids_array[idx_event] = part_data[:,1]
                phis_array[idx_event]    = part_data[:,2]
                etas_array[idx_event]    = part_data[:,3]
                pts_array[idx_event]     = part_data[:,4]

            f["Events"] = {"pt":      ak.from_numpy(pts_array), 
                            "eta":    ak.from_numpy(etas_array), 
                            "phi":   ak.from_numpy(phis_array), 
                            "pdg_id": ak.from_numpy(pdg_ids_array)}
            
            f["Metadata"] = {"n_events": ak.Array([n_events]), 
                             "ev_size": ak.Array([ev_size])}

            if has_headers:
                f["Headers"] = {"vld_header": ak.from_numpy(vld_header_array), 
                                "err_bit":    ak.from_numpy(err_bit_array), 
                                "lr_num":     ak.from_numpy(lr_num_array), 
                                "orbit_cnt":  ak.from_numpy(orbit_cnt_array), 
                                "bx_cnt":     ak.from_numpy(bx_cnt_array), 
                                "n_cand":     ak.from_numpy(n_cand_array)}

    def _print_table(self, data, column_names):
        columns = len(column_names)
        col_widths = [max(len(str(row[i])) for row in data) for i in range(columns)]
        col_widths = [max(len(column_names[i]), col_widths[i]) for i in range(columns)]
        format_str = ' | '.join([f'{{:<{width}}}' for width in col_widths])

        print(format_str.format(*column_names))
        print('-' * (sum(col_widths) + 3 * (columns - 1)))

        for row in data:
            print(format_str.format(*row))

    def _unpack_header(self, line_str):
        data = int.from_bytes(line_str, sys.byteorder)

        vld_header = (data >> 62) & 0b11                    # Bits 63-62 (2 bits)
        err_bit    = (data >> 61) & 0b1                     # Bit 61 (1 bit)
        lr_num     = (data >> 56) & 0b11111                 # Bits 60-56 (5 bits)
        orbit_cnt  = (data >> 24) & 0xFFFFFFFF              # Bits 55-24 (32 bits)
        bx_cnt     = (data >> 12) & 0xFFF                   # Bits 23-12 (12 bits)
        n_cand     = data & 0xFF                            # Bits 11-00 (12 bits) BUT ONLY 8 EFFECTIVELY USED (look at the binary mask indeed)

        return {
            "vld_header": vld_header,
            "err_bit": err_bit,
            "lr_num": lr_num,
            "orbit_cnt": orbit_cnt,
            "bx_cnt": bx_cnt,
            "n_cand": n_cand
        }
    
    def _unpack_particle(self, line_str):
        data = int.from_bytes(line_str, sys.byteorder)

        pid         = (data >> 37) & 0b111          # Bits 39-37 (3 bits)
        phi_sign    = (data >> 36) & 0b1            # Bit 36 is the sign bit for phi
        phi_payload = (data >> 26) & 0x3FF          # Bits 35-26 (10 bits) are the non-sign bits for phi
        eta_sign    = (data >> 25) & 0b1            # Bit 25 is the sign bit for eta
        eta_payload = (data >> 14) & 0x7FF          # Bits 24-14 (11 bits) are the non-sign bits for eta
        pt          = data & 0x3FFF                 # Bits 13-00 (14 bits) 

        phi = 0
        for ii in range(phi_payload.bit_length()):
            cur_bit = (phi_payload >> ii) & 0b1

            if cur_bit:
                phi += 2 ** ii

        phi += (-1) * phi_sign * 2 ** (11 - 1)

        eta = 0
        for ii in range(eta_payload.bit_length()):
            cur_bit = (eta_payload >> ii) & 0b1

            if cur_bit:
                eta += 2 ** ii

        eta += (-1) * eta_sign * 2 ** (12 - 1)

        match pid:
            case 0: # 000
                pdg_id = 130

            case 1: # 001
                pdg_id = 22

            case 2: # 010
                pdg_id = -211

            case 3: # 011
                pdg_id = 211

            case 4: # 100
                pdg_id = 11

            case 5: # 101
                pdg_id = -11

            case 6: # 110
                pdg_id = 13

            case 7: # 111
                pdg_id = -13

            case _:
                pdg_id = "ERR"

        return {
            "pdg_id": pdg_id, 
            "phi": phi, 
            "eta": eta, 
            "pt": pt
        }
    
    def _get_raw_pdgid(self, pdg_id):
        match pdg_id:
            case 130:
                return 0

            case 22:
                return 1

            case -211:
                return 2

            case 211:
                return 3

            case 11:
                return 4

            case -11:
                return 5

            case 13:
                return 6

            case -13:
                return 7
    
if __name__ == "__main__":
    # file = "Puppi.dump"
    # file = "Puppi_224.dump"
    # file = "Puppi_104_nh.dump"
    # file = "Puppi_208_nh.dump"
    # file = "puppi_WTo3Pion_PU200.dump"
    file = "PuppiSignal_224.dump"
    with PuppiData(file) as myPuppi:
        myPuppi.to_aiecsv64_unpacked()