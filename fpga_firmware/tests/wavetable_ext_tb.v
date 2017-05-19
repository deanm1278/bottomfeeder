module testbench;
  reg clk = 1;

  reg         [`DATAWIDTH-1:0] Fs = 16'h0;
  reg         [2:0] step = 3'b0;
  reg         sub_en = 0;
  reg         [`DATAWIDTH-1:0] RDATA = 16'h0;		//value read from the ram
  reg         EXT_READ = 0;
  reg         EXT_READ_ENABLE = 0;

  wire        [`ADDRWIDTH-1:0] 	RADDR; //wavetable position (8 bit)
  wire        [1:0] rbank;
  wire        RCLK;
  wire        [`DATAWIDTH-1:0] dout;
  wire        SUB_OUT;

  always #5 clk = ~clk;

  always @(posedge RCLK) RDATA <= RDATA + 1'b1;

  initial begin
    $display("1..N");
    repeat (3) @(posedge clk); //a few rando cycles
    EXT_READ_ENABLE <= 1;
    EXT_READ <= 1;
    repeat (2) @(posedge clk);
    
    if(RDATA == 16'h0) $display("ok 1 - RDATA is correct");
    else $display("not ok 1 - RDATA is incorrect: %b", RDATA);
    if(RADDR == 8'h0) $display("ok 1 - RADDR is correct");
    else $display("not ok 1 - RADDR is incorrect: %b", RADDR);
    if(rbank == 2'b0) $display("ok 1 - rbank is correct");
    else $display("not ok 1 - rbank is incorrect: %b", rbank);

    repeat (4) @(posedge clk);

    //dout should not change
    if(dout == 16'h8000) $display("ok 1 - dout is correct");
    else $display("not ok 1 - dout is incorrect: %b", dout);
    
    //RDATA should change
    if(RDATA == 16'h1) $display("ok 1 - RDATA is correct");
    else $display("not ok 1 - RDATA is incorrect: %b", RDATA);
    if(RADDR == 8'h1) $display("ok 1 - RADDR is correct");
    else $display("not ok 1 - RADDR is incorrect: %b", RADDR);
    if(rbank == 2'b0) $display("ok 1 - rbank is correct");
    else $display("not ok 1 - rbank is incorrect: %b", rbank);

    EXT_READ <= 0;
    repeat (4) @(posedge clk);
    EXT_READ <= 1;
    repeat (6) @(posedge clk);

    //dout should not change
    if(dout == 16'h8000) $display("ok 1 - dout is correct");
    else $display("not ok 1 - dout is incorrect: %b", dout);
    
    //RDATA should change
    if(RDATA == 16'h2) $display("ok 1 - RDATA is correct");
    else $display("not ok 1 - RDATA is incorrect: %b", RDATA);
    if(RADDR == 8'h2) $display("ok 1 - RADDR is correct");
    else $display("not ok 1 - RADDR is incorrect: %b", RADDR);

    $finish;
  end

  WAVETABLE uut (
    .clk(clk),
    .Fs(Fs),
    .step(step),
    .RADDR(RADDR),
    .rbank(rbank),
    .RDATA(RDATA),
    .RCLK(RCLK),
    .dout(dout),
    .SUB_OUT(SUB_OUT),
    .EXT_READ(EXT_READ),
    .EXT_READ_ENABLE(EXT_READ_ENABLE)
  );
endmodule