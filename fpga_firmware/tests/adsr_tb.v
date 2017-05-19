module testbench;
  reg clk = 1;

  reg [`DATAWIDTH-1:0] A_INTERVAL = 16'h1;
  reg [`DATAWIDTH-1:0] D_INTERVAL = 16'h1;
  reg [6:0] SUS_LVL = 16'h8;
  reg [`DATAWIDTH-1:0] R_INTERVAL = 16'h1;
  
  reg START = 1'b0;
  
  wire [6:0] OUTVALUE;
  wire RUNNING;
  
  always #5 clk = ~clk;

  initial begin
	
    //$dumpfile("test.vcd");
    //$dumpvars(0,testbench);
  
    $display("1..8");
    repeat (5) @(posedge clk);
	
	START <= 1;
	
	repeat (4) @(posedge clk);
	
    if(OUTVALUE == 7'h0) $display("ok 1 - data has been set"); 
    else $display("not ok 1 - data is incorrect: %b", OUTVALUE);
	
	if(RUNNING) $display("ok 2 - running has been set"); 
    else $display("not ok 2 - running is incorrect");
	
	//cycles / ( A_INTERVAL + 1)
	repeat (110) @(posedge clk);
	if(OUTVALUE == 55) $display("ok 3 - data has been set"); 
    else $display("not ok 3 - data is incorrect: %d", OUTVALUE);
	
	repeat (144) @(posedge clk);
	if(OUTVALUE == 127) $display("ok 4 - data has been set"); 
    else $display("not ok 4 - data is incorrect: %d", OUTVALUE);
	
	//we should now be in the decay state
	//127 - (cycles - 1 ) / ( D_INTERVAL + 1)
	repeat (25) @(posedge clk);
	if(OUTVALUE == 115) $display("ok 5 - decay state entered, data set"); 
    else $display("not ok 5 - data is incorrect: %d", OUTVALUE);
	
	//get to the end of decay
	repeat (235) @(posedge clk);
	if(OUTVALUE == 8) $display("ok 6 - sustain state entered, data set"); 
    else $display("not ok 6 - data is incorrect: %d", OUTVALUE);
	
	START <= 0;
	
	//4 cycles until t_release begins, r_interval + 1 cycles until fire
	repeat (7) @(posedge clk);
	if(OUTVALUE == 8 - 1) $display("ok 7 - release state entered, data set"); 
    else $display("not ok 7 - data is incorrect: %d", OUTVALUE);
	
	//get to the end
	repeat (300) @(posedge clk);
	
	if(!RUNNING) $display("ok 8 - running has been cleared"); 
    else $display("not ok 8 - running is incorrect");
	
    $finish;
  end

  ADSR uut (
    .clk(clk),
	.A_INTERVAL(A_INTERVAL),
	.D_INTERVAL(D_INTERVAL),
	.SUS_LVL(SUS_LVL),
	.R_INTERVAL(R_INTERVAL),
	.START(START),
	.OUTVALUE(OUTVALUE),
	.RUNNING(RUNNING)
  );
endmodule