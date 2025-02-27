open_project -reset "proj"
set_top s2mm

add_files src/s2mm.cpp
add_files -tb s2mm_tb.cpp

open_solution -reset -flow_target vitis "solution"
set_part {xcvc1902-vsvd1760-2MP-e-S}
create_clock -period 2.777 

csim_design -clean 
# csynth_design -dump_post_cfg
# export_design -format xo
exit