import os
import h5py
import numpy as np
import subprocess
import config

TO_FIXED_FACTORS = {"pt": 1 / 0.25, "eta": 1 / config.F_CONV_11, "phi": 1 / config.F_CONV_11, "pdg_id": 1, "gen_pi_idx": 1}

def get_raw_pdgid(pdg_id):
    if pdg_id == 130:
        return 0

    elif pdg_id == 22:
        return 1

    elif pdg_id == -211:
        return 2
    
    elif pdg_id == 211:
        return 3
    
    elif pdg_id == 11:
        return 4
    
    elif pdg_id == -11:
        return 5
    
    elif pdg_id == 13:
        return 6

    elif pdg_id == -13:
        return 7
    
    else:
        return "ERR"

def get_xilinx_environment():
    command = f'bash -c "source {config.XILINX_SETUP} && env"'
    result = subprocess.run(command, capture_output=True, text=True, shell=True)

    env = {}
    for line in result.stdout.splitlines():
        key, _, value = line.partition("=")
        env[key] = value

    return env

def generate_txts(input_file: str, events: list, plio_width: int, interface_width: int, out_files: list):
    with h5py.File(config.DATA + "/" + input_file, 'r') as f:
        assert plio_width % interface_width == 0, "plio_width is not an integer multiple of interface_width"
        n_cols = np.int16(plio_width / interface_width)

        assert len(out_files) == 2, "Expected two out files"
        f_0 = open(config.AIE_DATA + "/" + out_files[0] + ".txt", "w")
        f_1 = open(config.AIE_DATA + "/" + out_files[1] + ".txt", "w")

        for event in events:
            event_data = {}

            for i, var in enumerate(["pt", "eta", "phi", "pdg_id"]):
                var_data = f[str(event)][var][:] * TO_FIXED_FACTORS[var]

                if var == "pdg_id":
                    var_data = np.array([get_raw_pdgid(el) for el in var_data])

                var_data = np.pad(var_data, (0, 224 - len(var_data)), mode="constant", constant_values=0).astype(np.int16)
                var_data = var_data.reshape((-1, n_cols))
                event_data[var] = var_data
        
            pts = f[str(event)]["pt"][:]
            pdg_ids = f[str(event)]["pdg_id"][:]
            is_filter_pt = pts > 7
            is_filter_pdg_id = (np.abs(pdg_ids) == 211) | (np.abs(pdg_ids) == 11)
            is_filter = np.logical_and(is_filter_pt, is_filter_pdg_id)
            is_filter_idx = np.argwhere(is_filter).squeeze(1)
            is_filter_idx += 1
            is_filter_idx = np.pad(is_filter_idx, (0, 16 - len(is_filter_idx)), mode="constant", constant_values=0)
            is_filter_idx = is_filter_idx.reshape(-1, n_cols)
            event_data["is_filter_idx"] = is_filter_idx

            pt_phi_filteridx = np.vstack((event_data["pt"], event_data["phi"], event_data["is_filter_idx"]))
            for row in pt_phi_filteridx:
                f_0.write(" ".join(map(str, row)) + "\n")

            eta_pdg_id = np.vstack((event_data["eta"], event_data["pdg_id"]))
            for row in eta_pdg_id:
                f_1.write(" ".join(map(str, row)) + "\n")

        f_0.close()
        f_1.close()


    
def aie_compile_x86(env):
    os.chdir(config.AIE_X86)

    subprocess.run(["aiecompiler", 
                    "--target=x86sim", 
                    "-I", 
                    config.AIE_SRC,
                    config.AIE_SRC + "/graph.cpp", 
                    "-workdir", 
                    config.WORK_X86], 
                    env=env)
    
def aie_compile_baseline_x86(env):
    os.chdir(config.AIE_X86_BASELINE)

    subprocess.run(["aiecompiler", 
                    "--target=x86sim", 
                    "-I", 
                    config.AIE_SRC_BASELINE,
                    config.AIE_SRC_BASELINE + "/graph.cpp", 
                    "-workdir", 
                    config.WORK_X86_BASELINE], 
                    env=env)

def aie_compile_hw(env):
    os.chdir(config.AIE_HW)

    subprocess.run(["aiecompiler", # no debug flag needed here
                    "-target",
                    "hw", 
                    "--platform",
                    f"{config.XILINX_VCK5000_GEN4X8_XDMA}", 
                    "-I", 
                    f"{config.AIE_SRC}", 
                    f"{config.AIE_SRC}/graph.cpp",
                    "-workdir", 
                    f"{config.WORK_HW}", 
                    "--stacksize=16384"], 
                    env=env)
    
def aie_compile_baseline_hw(env):
    os.chdir(config.AIE_HW_BASELINE)

    subprocess.run(["aiecompiler", # no debug flag needed here
                    "-target",
                    "hw", 
                    "--platform",
                    f"{config.XILINX_VCK5000_GEN4X8_XDMA}", 
                    "-I", 
                    f"{config.AIE_SRC_BASELINE}", 
                    f"{config.AIE_SRC_BASELINE}/graph.cpp",
                    "-workdir", 
                    f"{config.WORK_HW_BASELINE}", 
                    "--stacksize=4800", 
                    "--pl-freq=360", 
                    "--Xchess=main:darts.xargs=-nb"], 
                    env=env)
    
def run_x86_simulator(env):
    os.chdir(config.AIE_X86)

    subprocess.run(["x86simulator", 
                    "--dump",
                    f"--pkg-dir={config.WORK_X86}", 
                    f"--input-dir={config.AIE_DATA}",
                    f"--output-dir={config.OUT_SIM_X86}"], 
                    env=env)
    
def run_baseline_x86_simulator(env):
    os.chdir(config.AIE_X86_BASELINE)

    subprocess.run(["x86simulator", 
                    "--dump",
                    f"--pkg-dir={config.WORK_X86_BASELINE}", 
                    f"--input-dir={config.AIE_DATA_BASELINE}",
                    f"--output-dir={config.OUT_SIM_X86_BASELINE}"], 
                    env=env)
    
def run_aiesimulator(env):
    os.chdir(config.AIE_HW)

    subprocess.run(["aiesimulator", 
                    "--dump-vcd=foo", 
                    f"--pkg-dir={config.WORK_HW}",
                    f"--input-dir={config.AIE_DATA}", 
                    f"--output-dir={config.OUT_SIM_AIE}"], 
                    env=env)
    
def run_baseline_aiesimulator(env):
    os.chdir(config.AIE_HW_BASELINE)

    subprocess.run(["aiesimulator", 
                    "--dump-vcd=foo", 
                    f"--pkg-dir={config.WORK_HW_BASELINE}",
                    f"--input-dir={config.AIE_DATA_BASELINE}", 
                    f"--output-dir={config.OUT_SIM_AIE_BASELINE}"], 
                    env=env)
    
def hls_compile(env, kernel_name):
    os.chdir(config.HLS)

    subprocess.run(["v++", 
                    "-g", # debug flag
                    "-c", # compile flag
                    "-t", 
                    config.TARGET, 
                    "--platform", 
                    f"{config.XILINX_VCK5000_GEN4X8_XDMA}",
                    "-k",
                    f"{kernel_name}",
                    "-o", 
                    f"{config.HLS}/{kernel_name}/{config.TARGET}/{kernel_name}.{config.TARGET}.xo", 
                    f"{config.HLS}/{kernel_name}/src/{kernel_name}.cpp"], 
                    env=env)

def link_system(env):
    os.chdir(config.LINK)

    aie_dir = "x86" if config.TARGET == "sw_emu" else "hw"

    subprocess.run(["v++", 
                    "-g", # debug flag
                    "-l", # link flag
                    "-t", 
                    config.TARGET, 
                    "--platform", 
                    f"{config.XILINX_VCK5000_GEN4X8_XDMA}", 
                    f"{config.HLS}/mm2s/{config.TARGET}/mm2s.{config.TARGET}.xo", 
                    f"{config.HLS}/s2mm/{config.TARGET}/s2mm.{config.TARGET}.xo", 
                    f"{config.AIE}/{aie_dir}/libadf.a", 
                    "-o", 
                    f"out.{config.TARGET}.xsa", 
                    "--save-temps",
                    "-j", # to speed up computation
                    "4",
                    "--config", 
                    "./system.cfg"], 
                    env=env)

def package_system(env):
    os.chdir(config.PACKAGE)

    aie_dir = "x86" if config.TARGET == "sw_emu" else "hw"

    subprocess.run(["v++",
                    "-g", # debug flag
                    "--package", # package flag
                    "-t",
                    config.TARGET, 
                    "--platform", 
                    f"{config.XILINX_VCK5000_GEN4X8_XDMA}", 
                    f"{config.LINK}/out.{config.TARGET}.xsa", 
                    f"{config.AIE}/{aie_dir}/libadf.a", 
                    "--package.boot_mode=ospi",
                    "-o", 
                    "output.xclbin"], 
                    env=env)

def sw_compile(env):
    os.chdir(config.SW)
    
    if config.TARGET == "sw_emu":
        exec_path = config.SW_EMU

    elif config.TARGET == "hw_emu":
        exec_path = config.HW_EMU

    elif config.TARGET == "hw":
        exec_path = config.HW_RUN

    else:
        print("Please configure target")

    subprocess.run(["g++", 
                    "-std=c++17", 
                    "-g", # debug flag
                    f"-I{config.XRT_INCLUDE}",
                    f"-I{config.HLS_INCLUDE}",
                    "-I./", 
                    f"host_full_dataset.cpp", 
                    "-o", 
                    f"{exec_path}/host.o", 
                    f"-L{config.XRT_LIB}", 
                    f"-L{config.HLS_LIB}",
                    "-lxrt_coreutil", 
                    "-pthread"],
                    env=env)

def run_hw_emu(env):
    os.chdir(config.HW_EMU)

    subprocess.run(["emconfigutil", 
                    "--platform" ,
                    f"{config.XILINX_VCK5000_GEN4X8_XDMA}"], 
                    env=env)
    
    env["XCL_EMULATION_MODE"] = "hw_emu"

    subprocess.run(["./host.o",
                    f"{config.PACKAGE}/output.xclbin"],
                    env=env)
    
def run_hw(env):
    os.chdir(config.HW_RUN)

    subprocess.run(["./host.o",
                    f"{config.PACKAGE}/output.xclbin"],
                    env=env)
    
if __name__ == "__main__":
    env = get_xilinx_environment()

    # generate data for aie simulation
    # input_file = "l1Nano_WTo3Pion_PU200.hdf5"
    # generate_txts(input_file, [0, 1, 2, 3, 4, 5, 6], 32, 16, ["boo0", "boo1"])
    # generate_txts(input_file, [10554], 32, 16, ["zoo0", "zoo1"])

    # aie_compile_x86(env)
    # run_x86_simulator(env)

    aie_compile_hw(env)
    run_aiesimulator(env)

    # hls_compile(env, "mm2s")
    # hls_compile(env, "s2mm")

    # link_system(env)

    # package_system(env)

    # sw_compile(env)

    # run_hw_emu(env)

    # run_hw(env)