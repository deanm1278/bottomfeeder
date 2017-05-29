module testbench;
  reg clk = 1;
  reg SCK = 0; //spi clock
  reg SSEL = 1; //CS line
  reg enable = 1;

  wire [15:0] DATA_OUT; //data to be read in from mosi
  wire MISO;
  wire DATA_READY; //flag to indicate full 32 bit command has been received

  reg [15:0] to_send = 16'hA3A4;
  wire MOSI = to_send[15];
  reg clk2 = 1;
  reg RDY_FIRED = 1'b0;

  always #5 clk = ~clk;

  //test fixture to simulate spi data being sent by master w/ a different clock
  always #8 clk2 = ~clk2;

  reg [2:0] SCKr;  always @(posedge clk2) SCKr <= {SCKr[1:0], SCK};
  wire SCK_risingedge = (SCKr[2:1]==2'b01);  // now we can detect SCK rising edges
  wire SCK_fallingedge = (SCKr[2:1]==2'b10);  // and falling edges
  
  always @(posedge clk2) begin
    if(~SSEL) begin
        if(SCK_fallingedge) to_send <= {to_send[14:0], 1'b0}; //shift out on falling edge
    end
  end

  always @(posedge DATA_READY) RDY_FIRED <= ~RDY_FIRED;

  initial begin
	//$dumpfile("test.vcd");
    //$dumpvars(0,testbench);
  
    $display("SPI wave tests 1..1");
    #100;
    //take cs low
    SSEL <= 0;

    #100;

    //shift stuff out
    repeat(16) begin
        #100; 
        SCK <= ~SCK;
        #100;
        SCK <= ~SCK;
    end
	
	if(DATA_OUT == 16'hA3A4) $display("ok 1 - data has been set"); 
    else $display("not ok 1 - data is incorrect: %h", DATA_OUT);
	
    #100;
    //take cs high again
    SSEL <= 1;

    $finish;
  end

    SPI_WAVE uut(
        .clk(clk),
		.enable(enable),
        .SCK(SCK),
        .MOSI(MOSI),
        .SSEL(SSEL),
        .DATA_OUT(DATA_OUT),
        .DATA_READY(DATA_READY)
    );
endmodule