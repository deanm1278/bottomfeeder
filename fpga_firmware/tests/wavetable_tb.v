`define DATAWIDTH 16
`define ADDRWIDTH 8

module testbench;
  reg clk = 1;

  reg         [`DATAWIDTH-1:0] Fs = 16'h0;
  reg         [2:0] step = 3'b0;
  reg         sub_en = 0;
  reg		  enable = 0;
  reg		  [4:0] VOL = 5'h1;
  reg         [`DATAWIDTH-1:0] RDATA = 16'h0;		//value read from the ram

  wire        [`ADDRWIDTH-1:0] 	RADDR; //wavetable position (8 bit)
  wire        [1:0] RBANK;
  wire        RCLK;
  wire        [`DATAWIDTH-1:0] dout;
  wire        SUB_OUT;

  always #5 clk = ~clk;

  always @(posedge RCLK) RDATA <= RDATA + 2'b11;

  /*
  initial begin
    $dumpfile("test.vcd");
    $dumpvars(0,testbench);
  end
  */
  
  initial begin
 
    $display("Wavetable_tb 1..8");
    repeat (3) @(posedge clk); //a few rando cycles
    Fs <= 16'hF;
	enable <= 1;
    repeat (15 + 2) @(posedge clk);
    
    if(RDATA == 16'h0) $display("ok 1 - RDATA is correct");
    else $display("not ok 1 - RDATA is incorrect: %b", RDATA);
    if(RADDR == 8'h0) $display("ok 2 - RADDR is correct");
    else $display("not ok 2 - RADDR is incorrect: %b", RADDR);
    if(RBANK == 2'b0) $display("ok 3 - rbank is correct");
    else $display("not ok 3 - rbank is incorrect: %b", RBANK);

    repeat (16) @(posedge clk);

    if(dout == 16'h001) $display("ok 4 - dout is correct");
    else $display("not ok 4 - dout is incorrect: %b", dout);
    if(RADDR == 8'h1) $display("ok 5 - RADDR is correct");
    else $display("not ok 5 - RADDR is incorrect: %b", RADDR);
    if(RBANK == 2'b0) $display("ok 6 - rbank is correct");
    else $display("not ok 6 - rbank is incorrect: %b", RBANK);

    repeat (15 + 1) @(posedge clk);
	enable <= 1'b0;

    if(dout == 16'h003) $display("ok 7 - dout is correct");
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
	.enable(enable),
    .RCLK(RCLK),
    .SUB_OUT(SUB_OUT),
`ifdef POST_SYNTHESIS
	.  \Fs_input[0] (Fs[0]),
	.  \Fs_input[1] (Fs[1]),
	.  \Fs_input[2] (Fs[2]),
	.  \Fs_input[3] (Fs[3]),
	.  \Fs_input[4] (Fs[4]),
	.  \Fs_input[5] (Fs[5]),
	.  \Fs_input[6] (Fs[6]),
	.  \Fs_input[7] (Fs[7]),
	.  \Fs_input[8] (Fs[8]),
	.  \Fs_input[9] (Fs[9]),
	.  \Fs_input[10] (Fs[10]),
	.  \Fs_input[11] (Fs[11]),
	.  \Fs_input[12] (Fs[12]),
	.  \Fs_input[13] (Fs[13]),
	.  \Fs_input[14] (Fs[14]),
	.  \Fs_input[15] (Fs[15]),
	
	.	\step_input[0] (step[0]),
	.	\step_input[1] (step[1]),
	.	\step_input[2] (step[2]),
	
	.	\RADDR[0] (RADDR[0]),
	.	\RADDR[1] (RADDR[1]),
	.	\RADDR[2] (RADDR[2]),
	.	\RADDR[3] (RADDR[3]),
	.	\RADDR[4] (RADDR[4]),
	.	\RADDR[5] (RADDR[5]),
	.	\RADDR[6] (RADDR[6]),
	.	\RADDR[7] (RADDR[7]),
	
	.	\RBANK[0] (RBANK[0]),
	.	\RBANK[1] (RBANK[1]),
	
	.	\RDATA[0] (RDATA[0]),
	.	\RDATA[1] (RDATA[1]),
	.	\RDATA[2] (RDATA[2]),
	.	\RDATA[3] (RDATA[3]),
	.	\RDATA[4] (RDATA[4]),
	.	\RDATA[5] (RDATA[5]),
	.	\RDATA[6] (RDATA[6]),
	.	\RDATA[7] (RDATA[7]),
	.	\RDATA[8] (RDATA[8]),
	.	\RDATA[9] (RDATA[9]),
	.	\RDATA[10] (RDATA[10]),
	.	\RDATA[11] (RDATA[11]),
	.	\RDATA[12] (RDATA[12]),
	.	\RDATA[13] (RDATA[13]),
	.	\RDATA[14] (RDATA[14]),
	.	\RDATA[15] (RDATA[15]),
	
	.	\dout[0] (dout[0]),
	.	\dout[1] (dout[1]),
	.	\dout[2] (dout[2]),
	.	\dout[3] (dout[3]),
	.	\dout[4] (dout[4]),
	.	\dout[5] (dout[5]),
	.	\dout[6] (dout[6]),
	.	\dout[7] (dout[7]),
	.	\dout[8] (dout[8]),
	.	\dout[9] (dout[9]),
	.	\dout[10] (dout[10]),
	.	\dout[11] (dout[11]),
	.	\dout[12] (dout[12]),
	.	\dout[13] (dout[13]),
	.	\dout[14] (dout[14]),
	.	\dout[15] (dout[15])
`else
    .Fs_input(Fs),
    .step_input(step),
    .RADDR(RADDR),
    .RBANK(RBANK),
    .RDATA(RDATA),
    .dout(dout)
`endif
  );
endmodule