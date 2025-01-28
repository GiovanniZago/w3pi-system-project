# System Project Test on VCK5000

This repository is meant to provide a standard, bare-metal setup to work with the **AMD VCK5000 Versal Development Card** following the Vitis Toolkit Workflow. 

The suggested directory tree structure is the following, in which there are also reported all the files needed to run a **hardware emulation** of the system project. Some of them must be provided (P) while others are automatically generated (G) when running the Vitis commands. 

```
.
├── aie
│   ├── data
|   |   ├── etaphi_11_plio_64_if_16.txt (P)
│   ├── hw
|   |   ├── libadf.a    (G)
|   |   ├── ...
│   ├── src
|   |   ├── graph.cpp   (P)
|   |   ├── graph.h     (P)
|   |   ├── kernels.cpp (P)
|   |   ├── kernels.h   (P)
│   └── x86
├── hls
│   ├── mm2s.cpp        (P)
│   ├── mm2s.hw_emu.xo  (G)
│   ├── s2mm.cpp        (P)
│   ├── s2mm.hw_emu.xo  (G)
│   ├── ...
├── hw-emu
│   ├── emconfig.json   (G)
│   ├── host.o          (G)
│   ├── xrt.ini         (P)
│   ├── ...
├── link
│   ├── out.xsa         (G)
│   ├── system.cfg      (P)
│   └── ...
├── package
│   ├── output.xclbin   (G)
│   ├── ...
├── sw
│   ├── data.h          (P)
│   └── host.cpp        (p)
├── config.py           (P)
└── vitis_handler.py    (P)

```

