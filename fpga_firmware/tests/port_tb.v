module testbench;
  reg clk = 0;
  reg [15:0] WDATA = {13'h3EC, 3'h3};
  reg [15:0] PORT_STEP = 16'h4;
  reg WE = 0;
  reg WCLK = 0;
  wire [15:0] GEN_OUT;

  always #5 clk = ~clk;

  initial begin
	//$dumpfile("test.vcd");
    //$dumpvars(0,testbench);
  
    $display("1..1");
    repeat (5) @(posedge clk);

    WE <= 1;                //set write enable

    repeat (2) @(posedge clk);

    WCLK <= 1;              //toggle write clock

    repeat (5) @(posedge clk);
	
	WCLK <= 0;
	WDATA = {13'h7C6, 3'h4};
	
	repeat (2) @(posedge clk);
	
	WCLK <= 1;
	
	repeat (200) @(posedge clk);
	

    if(GEN_OUT == {13'h07C6, 3'h4}) $display("ok 1 - data has been set"); 
    else $display("not ok 1 - data is incorrect"); 

    $finish;
  end

  PORT uut (
    .clk(clk),
	.PORT_STEP(PORT_STEP),
    .WE(WE),
    .GEN_OUT(GEN_OUT),
    .WDATA(WDATA),
    .WCLK(WCLK)
  );
endmodule