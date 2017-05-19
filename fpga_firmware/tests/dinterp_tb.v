module testbench;
  reg clk = 1;

  reg [31:0] DATA = 32'hCA33A3A3;

  // write | wave | WE number | bank | address
  // 1     | 1    | 0010      | 10   | 0011 0011

  reg DATA_READY = 0;

  wire [1:0] wbank;
  wire [`ADDRWIDTH-1:0] WADDR;
  wire [`NUM_CHANNELS-1:0] WE;
  wire [7:0] RE;
  wire WCLK;
  output	wire [`DATAWIDTH-1:0] RDATA;

  always #1 clk = ~clk;

  initial begin
	//$dumpfile("test.vcd");
    //$dumpvars(0,testbench);
  
    $display("1..13");
    DATA_READY <= 1;
    repeat (1) @(posedge clk);
    DATA_READY <= 0; //WCLK goes high 3 cycles after DATA_READY falling edge to make sure data has settled
    repeat (3) @(posedge clk);
    
    if(WCLK == 1'b1) $display("ok 1 - WCLK is correct");
    else $display("not ok 1 - WCLK is incorrect: %b", WCLK);

    if(WE == 16'h4) $display("ok 2 - WE is correct");
    else $display("not ok 2 - WE is incorrect: %b", WE);

    if(WADDR == 8'h33) $display("ok 3 - WADDR is correct");
    else $display("not ok 3 - WADDR is incorrect: %b", WADDR);

    if(RE == 16'h0) $display("ok 4 - RE is correct");
    else $display("not ok 4 - RE is incorrect: %b", RE);

    if(wbank == 2'b10) $display("ok 5 - wbank is correct");
    else $display("not ok 5 - wbank is incorrect: %b", wbank);

    if(RDATA == 16'hA3A3) $display("ok 6 - RDATA is correct");
    else $display("not ok 6 - RDATA is incorrect: %b", RDATA);

    //register write operation
    DATA <= 32'h8040A4A4;
    // write | reg | WE number 
    // 1     | 0   | 00 0000 01      

    DATA_READY <= 1;
    repeat (1) @(posedge clk);
    DATA_READY <= 0; //WCLK goes high 3 cycles after DATA_READY falling edge to make sure data has settled
    repeat (3) @(posedge clk);

    if(WCLK == 1'b1) $display("ok 7 - WCLK is correct");
    else $display("not ok 7 - WCLK is incorrect: %b", WCLK);

    if(WE == 16'h2) $display("ok 8 - WE is correct");
    else $display("not ok 8 - WE is incorrect: %b", WE);

    if(RE == 16'h0) $display("ok 9 - RE is correct");
    else $display("not ok 9 - RE is incorrect: %b", RE);

    if(RDATA == 16'hA4A4) $display("ok 10 - RDATA is correct");
    else $display("not ok 10 - RDATA is incorrect: %b", RDATA);

    //register read operation
    DATA <= 32'h0040A4A4;
    // read | reg | WE number 
    // 0     | 0   | 00 0000 01      

    DATA_READY <= 1;
    repeat (1) @(posedge clk);
    DATA_READY <= 0; //WCLK goes high 3 cycles after DATA_READY falling edge to make sure data has settled
    repeat (3) @(posedge clk);

    if(WCLK == 1'b1) $display("ok 11 - WCLK is correct");
    else $display("not ok 11 - WCLK is incorrect: %b", WCLK);

    if(WE == 16'h0) $display("ok 12 - WE is correct");
    else $display("not ok 12 - WE is incorrect: %b", WE);

    if(RE == 16'h1) $display("ok 13 - RE is correct");
    else $display("not ok 13 - RE is incorrect: %b", RE);

    $finish;
  end

  DINTERP uut (
    .clk(clk),
    .DATA_READY(DATA_READY),
    .DATA(DATA),
    .wbank(wbank),
    .WADDR(WADDR),
    .WE(WE),
    .RE(RE),
    .WCLK(WCLK),
    .RDATA(RDATA)
  );
endmodule