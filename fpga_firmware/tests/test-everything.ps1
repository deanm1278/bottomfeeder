iverilog -o gen_reg_test ../modules.v gen_reg_tb.v
iverilog -o pwm_test ../modules.v pwm_tb.v
iverilog -o tmr_test ../modules.v tmr_tb.v
iverilog -o dinterp_test ../modules.v dinterp_tb.v
iverilog -o spi_test ../modules.v spi_tb.v

#yosys -p "synth_ice40 -top WAVETABLE -blif test.blif" ../modules.v
#yosys -o test_syn.v test.blif
#iverilog -o test_post -D POST_SYNTHESIS wavetable_tb.v test_syn.v /icestorm/share/ice40/cells_sim.v

iverilog -o wavetable_test ../modules.v wavetable_tb.v


iverilog -o adsr_test ../modules.v adsr_tb.v
iverilog -o mix_test ../modules.v mix_tb.v
iverilog -o port_test ../modules.v port_tb.v

vvp gen_reg_test
vvp pwm_test
vvp tmr_test
vvp dinterp_test
vvp spi_test
vvp wavetable_test -lxt2
vvp adsr_test 
vvp mix_test
vvp port_test

gtkwave test.vcd

Read-Host -Prompt "Press Enter to exit"
