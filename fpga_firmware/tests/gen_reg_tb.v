module testbench;
  reg clk = 1;
  reg [15:0] WDATA = 16'h0;
  reg WE = 0;
  reg WCLK = 0;
  wire [15:0] GEN_OUT;

  always #5 clk = ~clk;

  initial begin
    $display("1..1");
    repeat (5) @(posedge clk);

    WE <= 1;                //set write enable
    WDATA <= 16'hA3A3;      //set data

    repeat (2) @(posedge clk);

    WCLK <= 1;              //toggle write clock

    repeat (5) @(posedge clk);

    if(WDATA == 16'hA3A3) $display("ok 1 - data has been set"); 
    else $display("not ok 1 - data is incorrect"); 

    $finish;
  end

  GEN_REG uut (
    .clk(clk),
    .WE(WE),
`ifdef POST_SYNTHESIS
    . \GEN_OUT[0] (GEN_OUT[0]),
    . \GEN_OUT[1] (GEN_OUT[1]),
    . \GEN_OUT[2] (GEN_OUT[2]),
    . \GEN_OUT[3] (GEN_OUT[3])
`else
    .GEN_OUT(GEN_OUT),
    .WDATA(WDATA),
    .WCLK(WCLK)
`endif
  );
endmodule