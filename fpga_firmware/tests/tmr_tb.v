module testbench;
  reg clk = 1;
  reg [15:0] prescale = 16'h0005;
  reg enable = 0;
  wire fire;

  always #1 clk = ~clk;

  initial begin
    $display("1..5");
    repeat (5) @(posedge clk);
    if(fire == 1'b0) $display("ok 1 - output is correct");
    else $display("not ok 1 - output is incorrect");

    repeat (1) @(posedge clk);
    if(fire == 1'b0) $display("ok 2 - timer has fired");
    else $display("not ok 2 - timer has not fired");

    enable = 1; //now lets enable. The timer should fire after 5 cycles

    repeat (5) @(posedge clk);
    if(fire == 1'b0) $display("ok 3 - output is correct");
    else $display("not ok 3 - output is incorrect");

    repeat (1) @(posedge clk);
    if(fire == 1'b1) $display("ok 4 - timer has fired");
    else $display("not ok 4 - timer has not fired");

    repeat (1) @(posedge clk);
    if(fire == 1'b0) $display("ok 5 - output has reset");
    else $display("not ok 5 - output did not reset");

    $finish;
  end

  TMR uut (
    .clk(clk),
    .prescale(prescale),
    .enable(enable),
    .fire(fire)
  );
endmodule