#yosys -p "proc; check" modules.v wavetable_r2r.v

yosys -p "synth_ice40 -blif bf100.blif" modules.v wavetable_r2r.v
arachne-pnr -d 8k -P tq144:4k -p wavetable.pcf -w assignments.pcf bf100.blif -o bf100.txt
icepack bf100.txt bf100.bin
#iceprog test.bin

Read-Host -Prompt "Press Enter to exit"
