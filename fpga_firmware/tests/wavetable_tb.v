module testbench;
  reg clk = 1;

  reg         [`DATAWIDTH-1:0] Fs = 16'h0;
  reg         [2:0] step = 3'b0;
  reg         sub_en = 0;
  reg		  enable = 0;
  reg         [`DATAWIDTH-1:0] RDATA = 16'h0;		//value read from the ram

  wire        [`ADDRWIDTH-1:0] 	RADDR; //wavetable position (8 bit)
  wire        [1:0] rbank;
  wire        RCLK;
  wire        [`DATAWIDTH-1:0] dout;
  wire        SUB_OUT;

  always #5 clk = ~clk;

  always @(posedge RCLK) RDATA <= RDATA + 1'b1;

  initial begin
    $dumpfile("test.vcd");
    $dumpvars(0,testbench);
  
    $display("Wavetable_tb 1..8");
    repeat (3) @(posedge clk); //a few rando cycles
    Fs <= 16'h000F;
	enable <= 1;
    repeat (15 + 2) @(posedge clk);
    
    if(RDATA == 16'h0) $display("ok 1 - RDATA is correct");
    else $display("not ok 1 - RDATA is incorrect: %b", RDATA);
    if(RADDR == 8'h0) $display("ok 2 - RADDR is correct");
    else $display("not ok 2 - RADDR is incorrect: %b", RADDR);
    if(rbank == 2'b0) $display("ok 3 - rbank is correct");
    else $display("not ok 3 - rbank is incorrect: %b", rbank);

    repeat (4) @(posedge clk);

    if(dout == 16'h1) $display("ok 4 - dout is correct");
    else $display("not ok 4 - dout is incorrect: %b", dout);
    if(RADDR == 8'h1) $display("ok 5 - RADDR is correct");
    else $display("not ok 5 - RADDR is incorrect: %b", RADDR);
    if(rbank == 2'b0) $display("ok 6 - rbank is correct");
    else $display("not ok 6 - rbank is incorrect: %b", rbank);

    repeat (15 + 1) @(posedge clk);

    if(dout == 16'h2) $display("ok 7 - dout is correct");
    else $display("not ok 7 - dout is incorrect: %b", dout);
    if(RADDR == 8'h2) $display("ok 8 - RADDR is correct");
    else $display("not ok 8 - RADDR is incorrect: %b", RADDR);
	
	Fs <= 16'h000A;
	repeat (1000) @(posedge clk);

	step <= 3'b1;
	
	repeat (200) @(posedge clk);
	
    $finish;
  end

  WAVETABLE uut (
    .clk(clk),
    .Fs_input(Fs),
    .step_input(step),
	.enable(enable),
    .RADDR(RADDR),
    .rbank(rbank),
    .RDATA(RDATA),
    .RCLK(RCLK),
    .dout(dout),
    .SUB_OUT(SUB_OUT)
  );
endmodule