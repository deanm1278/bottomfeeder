module testbench;
  reg clk = 1;
  reg [6:0] ENV = 2;
  reg [`PWM_DEPTH-1:0] DC_PRE = 5;
  reg [4:0] MUL = 4;
  
  wire [`DATAWIDTH-1:0] DC_POST;

  always #1 clk = ~clk;

  initial begin
  
	//$dumpfile("test.vcd");
    //$dumpvars(0,testbench);
  
    $display("1..2");
    repeat (4) @(posedge clk);
    if(DC_POST == 5 + 2*4) $display("ok 1 - output is correct");
    else $display("not ok 1 - output is incorrect: %d", DC_POST);
	
	//now make sure it limits to 4095
	ENV <= 125;
	DC_PRE <= 255;
	MUL <= 31;
	
	repeat (4) @(posedge clk);
    if(DC_POST == 4095) $display("ok 2 - output is limited correctly");
    else $display("not ok 2 - output not limited correctly: %d", DC_POST);
	
    $finish;
  end

  MIX uut (
    .clk(clk),
	.ENV(ENV),
	.DC_PRE(DC_PRE),
	.MUL(MUL),
	.DC_POST(DC_POST)
  );
endmodule