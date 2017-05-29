module testbench;
  reg clk = 1;

  reg DATA_READY = 0;
  
  wire [3:0] WBANK;
  wire [7:0] WADDR;
  
  reg enable = 0;
  
  always #5 clk = ~clk;

  initial begin
	//$dumpfile("test.vcd");
    //$dumpvars(0,testbench);
  
    $display("dinterp wagve test 1..2");
	
	#10;
	
	enable <= 1;
	repeat(261) begin
		repeat (5) @(posedge clk);
		DATA_READY <= 1;
		repeat (1) @(posedge clk);
		DATA_READY <= 0;
	end
	
    if(WADDR == 8'h0) $display("ok 1 - WADDR is correct");
    else $display("not ok 1 - WADDR is incorrect: %b", WADDR);
	
	if(WBANK == 3'h2) $display("ok 2 - WBANK is correct");
    else $display("not ok 2 - WBANK is incorrect: %b", WADDR);

    $finish;
  end

  DINTERP_WAVE uut (
    .clk(clk),
	.enable(enable),
    .DATA_READY(DATA_READY),
	.WBANK(WBANK),
	.WADDR(WADDR)
  );
endmodule