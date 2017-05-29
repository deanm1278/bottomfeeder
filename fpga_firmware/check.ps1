yosys -p "read_verilog -lib +/ice40/cells_sim.v; hierarchy; proc; flatten; opt_clean; check;" modules.v wavetable_r2r.v

Read-Host -Prompt "Press Enter to exit"
