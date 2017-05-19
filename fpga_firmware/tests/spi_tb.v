module testbench;
  reg clk = 1;
  reg SCK = 0; //spi clock
  reg SSEL = 1; //CS line
  reg [`DATAWIDTH-1:0] READ_OUT = 16'hA3A3; //data to be shifted out on MISO

  wire [31:0] DATA_OUT; //data to be read in from mosi
  wire MISO;
  wire DATA_READY; //flag to indicate full 32 bit command has been received

  reg [31:0] to_send = 32'h8C8C8C8A;
  wire MOSI = to_send[31];
  reg clk2 = 1;
  reg [15:0] read = 16'h0;
  reg RDY_FIRED = 1'b0;

  always #5 clk = ~clk;

  //test fixture to simulate spi data being sent by master w/ a different clock
  always #8 clk2 = ~clk2;

  reg [2:0] SCKr;  always @(posedge clk2) SCKr <= {SCKr[1:0], SCK};
  wire SCK_risingedge = (SCKr[2:1]==2'b01);  // now we can detect SCK rising edges
  wire SCK_fallingedge = (SCKr[2:1]==2'b10);  // and falling edges
  reg [1:0] MISOr;  always @(posedge clk2) MISOr <= {MISOr[0], MISO};
  wire MISO_data = MISOr[1];

  always @(posedge clk2) begin
    if(~SSEL) begin
        if(SCK_fallingedge) to_send <= {to_send[30:0], 1'b0}; //shift out on falling edge
        else if(SCK_risingedge) read <= {read[14:0], MISO_data}; //read on rising edge
    end
  end

  always @(posedge DATA_READY) RDY_FIRED <= ~RDY_FIRED;

  initial begin
	$dumpfile("test.vcd");
    $dumpvars(0,testbench);
  
    $display("1..3");
    #100;
    //take cs low
    SSEL <= 0;

    #100;

    //shift stuff out
    repeat(32) begin
        #100; 
        SCK <= ~SCK;
        #100;
        SCK <= ~SCK;
    end

    #100;
    //take cs high again
    SSEL <= 1;

    if(DATA_OUT == 32'h8C8C8C8A) $display("ok 1 - data has been set"); 
    else $display("not ok 1 - data is incorrect: %h", DATA_OUT);

    if(RDY_FIRED == 1'b1 && DATA_OUT == 32'h8C8C8C8A) $display("ok 2 - data rdy has been set"); 
    else $display("not ok 2 - data rdy has not fired");

    //now lets test a read
    #100;
    //take cs low
    SSEL <= 0;

    #100;

    //reeeeead
    repeat(16) begin
        #100; 
        SCK <= ~SCK;
        #100;
        SCK <= ~SCK;
    end

    #100;
    //take cs high again
    SSEL <= 1;

    if(read == 16'hA3A3) $display("ok 3 - data has been read"); 
    else $display("not ok 3 - data is incorrect: %h", read);

    $finish;
  end

    SPI_SLAVE uut(
        .clk(clk),
        .SCK(SCK),
        .MOSI(MOSI),
        .MISO(MISO),
        .SSEL(SSEL),
        .DATA_OUT(DATA_OUT),
        .DATA_READY(DATA_READY),
        .READ_OUT(READ_OUT)
    );
endmodule