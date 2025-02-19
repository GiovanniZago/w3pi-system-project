import os
import h5py
import numpy as np
import subprocess
import config

def get_xilinx_environment():
    command = f'bash -c "source {config.XILINX_SETUP} && env"'
    result = subprocess.run(command, capture_output=True, text=True, shell=True)

    env = {}
    for line in result.stdout.splitlines():
        key, _, value = line.partition("=")
        env[key] = value

    return env

def generate_txts(input_file, f_conv, plio_width, interface_width, stacked=False):
    with h5py.File(config.AIE_DATA + "/" + input_file, 'r') as f:
        pt = f["0"]["pt"][:]
        eta = f["0"]["eta"][:]
        phi = f["0"]["phi"][:]
        pdg_id = f["0"]["phi"][:]

        # filter out values that have values of |eta| > 2.4 and |phi| > pi
        # eta_mask = np.abs(eta) <= 2.4
        # phi_mask = np.abs(phi) <= np.pi
        # tot_mask = eta_mask & phi_mask
        # eta = eta[tot_mask]
        # phi = phi[tot_mask]
        # eta = eta / f_conv
        # phi = phi / f_conv

        # zero pad to reach 224 particles
        if len(eta) < 224:
            eta = np.pad(eta, (0, 224 - len(eta)), 'constant')

        if len(phi) < 224:
            phi = np.pad(phi, (0, 224 - len(phi)), 'constant')

        # transform lists into numpy arrays
        n_cols = np.int16(plio_width / interface_width)
        
        if (128 % n_cols) != 0:
            raise ValueError("128 / n_cols is not integer")
        
        n_rows = np.int16(128 / n_cols)

        eta = eta.astype(np.int16).reshape(n_rows, n_cols)
        phi = phi.astype(np.int16).reshape(n_rows, n_cols)

        # write to txt files

        if stacked:
            eta_phi = np.vstack([eta, phi])
            np.savetxt(config.AIE_DATA + "/" + f'etaphi_11_plio_{plio_width}_if_{interface_width}.txt', eta_phi, delimiter=" ", fmt="%d")

        else:
            np.savetxt(config.AIE_DATA + "/" + f'eta_11_plio_{plio_width}_if_{interface_width}.txt', eta, delimiter=" ", fmt="%d")
            np.savetxt(config.AIE_DATA + "/" + f'phi_11_plio_{plio_width}_if_{interface_width}.txt', phi, delimiter=" ", fmt="%d")
    
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
                    f"{kernel_name}.hw_emu.xo", 
                    f"{config.HLS}/{kernel_name}.cpp"], 
                    env=env)

def link_system(env):
    os.chdir(config.LINK)

    subprocess.run(["v++", 
                    "-g", # debug flag
                    "-l", # link flag
                    "-t", 
                    config.TARGET, 
                    "--platform", 
                    f"{config.XILINX_VCK5000_GEN4X8_XDMA}", 
                    f"{config.HLS}/mm2s.hw_emu.xo", 
                    f"{config.HLS}/s2mm.hw_emu.xo", 
                    f"{config.AIE_HW}/libadf.a", 
                    "-o", 
                    "out.xsa", 
                    "--save-temps",
                    "-j", # to speed up computation
                    "4",
                    "--config", 
                    "./system.cfg"], 
                    env=env)

def package_system(env):
    os.chdir(config.PACKAGE)

    subprocess.run(["v++",
                    "-g", # debug flag
                    "-p", # package flag
                    "-t",
                    config.TARGET, 
                    "--platform", 
                    config.XILINX_VCK5000_GEN4X8_XDMA, 
                    "--package.boot_mode=ospi",
                    f"{config.LINK}/out.xsa", 
                    f"{config.AIE_HW}/libadf.a", 
                    "-o", 
                    "output.xclbin"], 
                    env=env)

def sw_compile(env):
    os.chdir(config.SW)

    exec_path = config.HW_EMU if config.TARGET == "hw_emu" else config.HW_RUN

    subprocess.run(["g++", 
                    "-std=c++17", 
                    "-g", # debug flag
                    f"-I{config.XRT_INCLUDE}",
                    "-I./", 
                    f"host.cpp", 
                    "-o", 
                    f"{exec_path}/host.o", 
                    f"-L{config.XRT_LIB}", 
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
    # generate_txts(input_file, config.F_CONV_11, 64, 16, True)

    # aie_compile_x86(env)
    run_x86_simulator(env)

    # aie_compile_hw(env)
    # run_aiesimulator(env)

    # aie_compile_baseline_hw(env)
    # run_baseline_aiesimulator(env)

    # hls_compile(env, "mm2s")
    # hls_compile(env, "s2mm")

    # link_system(env)

    # package_system(env)

    # sw_compile(env)

    # run_hw_emu(env)

    # run_hw(env)