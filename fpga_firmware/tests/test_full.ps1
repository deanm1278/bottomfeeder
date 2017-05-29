#yosys -p "synth_ice40 -top WAVETABLE -blif test.blif" ../modules.v
#yosys -o test_syn.v test.blif
#iverilog -o test_post -D POST_SYNTHESIS wavetable_tb.v test_syn.v /icestorm/share/ice40/cells_sim.v

iverilog -o full_test -DSIM ../wavetable_r2r.v ../modules.v full_tb.v /icestorm/share/ice40/cells_sim.v 

vvp full_test -lxt2

gtkwave test.vcd

Read-Host -Prompt "Press Enter to exit"
