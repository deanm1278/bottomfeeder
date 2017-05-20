`default_nettype none
`define DATAWIDTH 16
`define ADDRWIDTH 8
`define PWM_DEPTH 12
`define NUM_CHANNELS 21

module WAVETABLE(
  input		clk,
  input         [`DATAWIDTH-1:0] Fs_input,
  input         [2:0] step_input,
  input			enable,
  input         sub_en,
  output 	reg [`ADDRWIDTH-1:0] 	RADDR, //wavetable position (8 bit)
  output	reg [1:0]	rbank,
  input		[`DATAWIDTH-1:0] RDATA,		//value read from the ram
  output	reg RCLK,
  output	    [`DATAWIDTH-1:0] dout,
  output        SUB_OUT
);
  
  wire fire;
  reg [`ADDRWIDTH:0] cnt = 9'b0;
  reg [15:0] dlatch = 16'h0;
  
  reg [15:0] Fs = 16'h0;
  reg [2:0] step = 3'b0;
  
  reg SUB_STATE = 1'b0;
  
  reg [2:0] enabler = 3'b111;  always @(posedge clk) enabler <= {enabler[1:0], enable};
  //detect enable rising edge
  wire      enable_rising = (enabler[2:1]==2'b01);
  
  //shift reg so we can assert write when all our data has settled
  reg [2:0] shft = 3'b0;
  always @(posedge clk) shft <= {shft[1:0], (fire && enable)};
  wire dataRdy = (shft[2:1]==2'b10);
  
  //timer for the sample rate
  TMR t(clk, Fs, enable, fire);
  
  //increment the read position and bank if necessary
  always @(posedge clk)
    begin
      if(~enable) 
        begin
          RADDR <= 0;
          rbank <= 0;
          cnt <= 0;
        end
	  //write Fs and step on rising edge of enable
	  if(enable_rising) begin
		  Fs <= Fs_input;
		  step <= step_input;
	  end
      else if(dataRdy)
        begin
          dlatch <= RDATA;

          if( cnt == (8'hFF >> step) )begin
            rbank <= rbank + 1'b1;
            cnt <= 0;
            RADDR <= 0;
			
			//if a wave is already running, only latch in the new Fs and step on a known even bank
			Fs <= Fs_input;
			step <= step_input;
            if(rbank == 2'b11) SUB_STATE <= ~SUB_STATE;
          end else begin
            cnt <= cnt + 1'b1;
            RADDR <= RADDR + (8'b1 << step);
          end
        end
      end
    
  //set latches
  always @(posedge clk)
    begin
      if(~enable) RCLK <= 1'b0;
      else RCLK <= fire;
    end
	
	assign SUB_OUT = (sub_en == 1'b1 && enable ? SUB_STATE : 1'bz);
	assign dout = (enable ? dlatch : 16'h8000);
  
endmodule

module GEN_REG(
  input     clk,
  output    reg [`DATAWIDTH-1:0] GEN_OUT,
  input     [`DATAWIDTH-1:0] WDATA,
  input     WE,
  input     WCLK
);

  always @(posedge WCLK) begin
    if(WE) GEN_OUT <= WDATA;
  end

endmodule

module PWM(
  input     clk,
  output    reg PWM_OUT,
  input     [`PWM_DEPTH-1:0] dc
);
  
  reg [`PWM_DEPTH-1:0] cnt = 0;

  always@(posedge clk)
    begin
        cnt <= cnt + 1'b1;
        PWM_OUT <= (dc > cnt);
    end
endmodule

module NOISE(
  input clk,
  input [`DATAWIDTH-1:0] prescale,
  input enable,
  output NOISE_OUT
);
  
  reg [31:0] r = 32'h3FA2C6;
  reg noiselatch = 1'b0;
  wire fire;

  TMR t(clk, prescale, enable, fire);

  always @(posedge fire) begin
    r <= {r[30:0], (r[31] ^ r[29] ^ r[25] ^ r[24])};
    noiselatch <= r[0];
  end
  
  assign NOISE_OUT = (enable ? noiselatch : 1'bz);
  
endmodule

module TMR(
  input clk,
  input [15:0] prescale,
  input enable,
  output fire
);
  
  reg [15:0] cnt = 16'b0;
  wire fireD;
  reg f = 1'b0;
  
  always @(posedge clk)
    begin
      if(enable == 1'b0) cnt <= 1'b0;
      else 
        begin
          if(cnt == prescale) cnt <= 1'b0;
          else cnt <= cnt + 1'b1;
        end
    end
  
  always @(posedge clk)
    begin
      if(enable == 1'b0) f <= 1'b0;
      else f <= fireD;
    end
  
  assign fireD = (cnt == prescale);
  assign fire = f;
  
endmodule

module ADSR(
	input clk,
	input [`DATAWIDTH-1:0] A_INTERVAL,
	input [`DATAWIDTH-1:0] D_INTERVAL,
	input [`DATAWIDTH-1:0] R_INTERVAL,
	
	input [6:0] SUS_LVL,
	
	input START,
	
	output reg [6:0] OUTVALUE,
	output RUNNING
);

	//attack starts on START rising edge, release starts on START falling edge
	reg [2:0] STARTr;  always @(posedge clk) STARTr <= {STARTr[1:0], START};
	wire START_risingedge = (STARTr[1:0]==2'b01);  // detect START rising edges
	wire START_fallingedge = (STARTr[2:1]==2'b10);  // and falling edges
	
	reg [3:0] state = 4'b0;
	wire a_enable = (state == 4'b0001);
	wire d_enable = (state == 4'b0010);
	wire s_enable = (state == 4'b0100);
	wire r_enable = (state == 4'b1000);
	
	wire a_fire, d_fire, r_fire;
	
	//timer for attack
	TMR t_attack(clk, A_INTERVAL, a_enable, a_fire);
	TMR t_decay(clk, D_INTERVAL, d_enable, d_fire);
	TMR t_release(clk, R_INTERVAL, r_enable, r_fire);
	
	always @(posedge clk) begin
		if(START_risingedge) begin
			OUTVALUE <= 0;
			state <= 4'b0001; //attack state
		end
		else if(a_fire) begin
			OUTVALUE <= OUTVALUE + 1'b1;
			if(OUTVALUE == 7'b1111110) state <= 4'b0010; //decay state
		end
		else if(d_fire) begin
			//decrement outvalue
			OUTVALUE <= OUTVALUE - 1'b1;
			
			//this means sustain level is set to 0. exit here
			if(OUTVALUE == 7'b1) state <= 4'b0000; //done state
			
			else if(OUTVALUE == SUS_LVL + 1'b1) state <= 4'b0100; //sustain state
		end
		
		else if(START_fallingedge && SUS_LVL != 16'h0) begin
			OUTVALUE <= SUS_LVL;
			state <= 4'b1000; //release state
		end
		
		else if(r_fire) begin
			//decrement output
			OUTVALUE <= OUTVALUE - 1'b1;
			if(OUTVALUE == 7'b1) state <= 4'b0000; //done state
		end
		
		//sustain should always be the sustain value
		else if(state == 4'b0100) OUTVALUE <= SUS_LVL;
		
	end
	
	assign RUNNING = (state != 4'b0000);

endmodule

module PORT(
  input     clk,
  input		[`DATAWIDTH-1:0] PORT_STEP,
  output    reg [`DATAWIDTH-1:0] GEN_OUT,
  input     [`DATAWIDTH-1:0] WDATA,
  input     WE,
  input     WCLK
);

  reg [`DATAWIDTH-1:0] TARGET;
  reg [12:0] fs_latch = 13'h0;
  reg [2:0] step_latch = 3'b0;
  
  wire fire;
  
  wire [12:0] FS_TARGET = TARGET[15:3];
  wire [2:0] step_target = TARGET[2:0];
  wire enable = (TARGET != 16'h0);
  
  TMR t(clk, PORT_STEP, 1'b1, fire);
  
  always @(posedge WCLK) begin
    if(WE) TARGET <= WDATA;
  end
  
  always @(posedge clk) begin
	if((fs_latch == 13'b0 || PORT_STEP == 16'h0) && FS_TARGET != 13'h0) begin
		fs_latch <= FS_TARGET;
		step_latch <= step_target;
	end
	else if(fire) begin
		if( FS_TARGET != fs_latch || step_target != step_latch) begin
			if(step_target > step_latch) begin
				if(fs_latch > 13'h3E7) begin
					fs_latch <= fs_latch - 1'b1;
				end
				else begin 
					step_latch <= step_latch + 1'b1;
					fs_latch <= 13'h7CB;
				end
			end
			
			else if(step_target < step_latch) begin
				if(fs_latch < 13'h7CB) begin
					fs_latch <= fs_latch + 1'b1;
				end
				else begin 
					step_latch <= step_latch - 1'b1;
					fs_latch <= 13'h3E7;
				end
			end
			
			else begin
				if(FS_TARGET > fs_latch) fs_latch <= fs_latch + 1'b1;
				else fs_latch <= fs_latch - 1'b1;
			end
		end
	end
  end
  
  always @(posedge clk) GEN_OUT <= (enable ? {fs_latch[12:0], step_latch[2:0]} : 16'h0);

endmodule

module MIX(
	input clk,
	input [6:0] ENV,
	input [`PWM_DEPTH-1:0] DC_PRE,
	input [4:0] MUL,
	output reg [`DATAWIDTH-1:0] DC_POST
);

	reg [`DATAWIDTH-1:0] tmp = 16'h0;
	wire [`DATAWIDTH-1:0] outval;
	
	always @(posedge clk) begin
		tmp <= DC_PRE + (ENV * MUL);
		DC_POST <= outval;
	end
	
	assign outval = tmp > 16'hFFF ? 16'hFFF : tmp;

endmodule

module DINTERP(
  input 	clk,
  input 	DATA_READY,
  input		[31:0] DATA,
  output 	reg [1:0] wbank,
  output 	reg [`ADDRWIDTH-1:0] WADDR,
  output 	reg [`NUM_CHANNELS-1:0] WE,
  output 	reg [7:0] RE,
  output 	reg WCLK,
  output	reg [`DATAWIDTH-1:0] RDATA
);
  
  //shift reg so we can know when all our data has settled
  reg [1:0] shft = 2'b00;

  always @(posedge clk) shft <= {shft[0], DATA_READY};
  wire latch = (shft[1:0]==2'b10);
  
  always @(posedge clk) begin
	if(DATA_READY) begin
        if(DATA[30]) begin //if this bit is set we are writing a waveform
			wbank <= DATA[25:24]; //set bank
			WADDR <= DATA[23:16]; //set address
			RE <= 0;
			WE <= (1 << DATA[29:26]);
			RDATA <= DATA[15:0]; //set the data
        end
		else begin //this is a register
			if(DATA[31]) begin //we are writing
			  RE <= 0;
			  WE <= (1 << DATA[29:22]);
			end
			else begin //we are reading
			  RE <= DATA[29:22];
			  WE <= 0;
			end
			RDATA <= DATA[15:0]; //set the data
		end
	end

  end
  
  always @(posedge clk) begin
    WCLK <= latch;
  end
  
endmodule

module SPI_SLAVE(
  	input		clk,
	input		SCK,
  	input		MOSI,
  	output		MISO,
  	input		SSEL,
  	output		reg [31:0] DATA_OUT,
  	output		reg DATA_READY,
        input           [`DATAWIDTH-1:0] READ_OUT
);
  
reg [2:0] SCKr;  always @(posedge clk) SCKr <= {SCKr[1:0], SCK};
wire SCK_risingedge = (SCKr[2:1]==2'b01);  // now we can detect SCK rising edges
wire SCK_fallingedge = (SCKr[2:1]==2'b10);  // and falling edges

// same thing for SSEL
reg [2:0] SSELr = 3'b111;  always @(posedge clk) SSELr <= {SSELr[1:0], SSEL};
wire      SSEL_active = ~SSELr[1];  // SSEL is active low

// message starts at falling edge
wire      SSEL_startmessage = (SSELr[2:1]==2'b10);

// message stops at rising edge
wire      SSEL_endmessage = (SSELr[2:1]==2'b01);

// and for MOSI
reg [1:0] MOSIr;  always @(posedge clk) MOSIr <= {MOSIr[0], MOSI};
wire MOSI_data = MOSIr[1];

// we handle SPI in 32-bits format, so we need a 5 bits counter to count the bits as they come in
reg [4:0] bitcnt;

reg byte_received;  // high when a byte has been received
reg [31:0] byte_data_received = 32'b0;

reg [`DATAWIDTH-1:0] byte_data_sent;

always @(posedge clk)
begin
  if(~SSEL_active)
    bitcnt <= 5'b00000;
  else
  if(SCK_risingedge)
  begin
    bitcnt <= bitcnt + 5'b01;

    // implement a shift-left register (since we receive the data MSB first)
    byte_data_received <= {byte_data_received[30:0], MOSI_data};
  end
end

  always @(posedge clk) byte_received <= SSEL_active && SCK_risingedge && (bitcnt==5'b11111);
  always @(posedge clk) begin
    if(byte_received) begin
      DATA_OUT <= byte_data_received;
    end
    DATA_READY <= byte_received;
  end

  always @(posedge clk) begin
     if(SSEL_active)
       begin
	  if(SSEL_startmessage)
	    byte_data_sent <= READ_OUT;  
	  else
	    if(SCK_fallingedge)
	      begin
		   byte_data_sent <= {byte_data_sent[14:0], 1'b0};
	      end
       end
    end
	
  //we don't really need to worry about this since the circuit has a hardware tristate buffer
  assign MISO = (!SSEL) ? byte_data_sent[15] : 1'bz;  // send MSB first
  //assign MISO = byte_data_sent[15];
  
endmodule
