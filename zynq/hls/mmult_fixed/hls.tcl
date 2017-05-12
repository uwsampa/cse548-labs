# Commands to compile your matrix multiply design
set src_dir "."
open_project accel
set_top mmult_hw
add_files $src_dir/mmult_fixed.cpp
add_files -tb $src_dir/mmult_test.cpp
open_solution "solution0"
set_part {xc7z020clg484-1}
create_clock -period 10 -name default
csim_design -clean
csynth_design
close_project
exit