#yosys -p "read_verilog -lib +/ice40/cells_sim.v; hierarchy; proc; flatten; opt_clean; check;" modules.v wavetable_r2r.v

yosys -p "read_verilog -lib +/ice40/cells_sim.v; setattr -set keep 1 r:\\* w:\\*; hierarchy -check -top top; 
				synth_ice40 -run flatten: -blif bf100.blif" modules.v wavetable_r2r.v
arachne-pnr -d 8k -P tq144:4k -p wavetable.pcf -w assignments.pcf bf100.blif -o bf100.txt
icepack bf100.txt bf100.bin
#iceprog test.bin

Read-Host -Prompt "Press Enter to exit"
