module testbench;
  reg clk = 1;
  reg [`DATAWIDTH-1:0] dc = 16'h00FF;
  wire PWM_OUT;

  always #1 clk = ~clk;

  initial begin
    $display("1..5");
    repeat (5) @(posedge clk);
    if(PWM_OUT == 1'b1) $display("ok 1 - output is correct");
    else $display("not ok 1 - output is incorrect");

    repeat (250) @(posedge clk);
    if(PWM_OUT == 1'b1) $display("ok 2 - output is correct");
    else $display("not ok 1 - output is incorrect");

    repeat (1) @(posedge clk); //should flip when cnt > 0xFF
    if(PWM_OUT == 1'b0) $display("ok 3 - output is correct");
    else $display("not ok 1 - output is incorrect");

    repeat (3840) @(posedge clk);
    if(PWM_OUT == 1'b0) $display("ok 4 - output is correct");
    else $display("not ok 1 - output is not correct");

    repeat (1) @(posedge clk); //should reset when 12 bit counter overflows
    if(PWM_OUT == 1'b1) $display("ok 5 - output has reset");
    else $display("not ok 1 - output has not reset");

    $finish;
  end

  PWM uut (
    .clk(clk),
    .PWM_OUT(PWM_OUT),
    .dc(dc[11:0])
  );
endmodule