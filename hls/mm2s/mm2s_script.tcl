open_project -reset "proj"
set_top mm2s

add_files src/mm2s.cpp
add_files -tb mm2s_tb.cpp

open_solution -reset -flow_target vitis "solution"
set_part {xcvc1902-vsvd1760-2MP-e-S}
create_clock -period 2.777 

csynth_design
export_design -format xo
exit