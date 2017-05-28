`default_nettype none
`define DATAWIDTH 16
`define ADDRWIDTH 11
`define PWM_DEPTH 12
`define NUM_CHANNELS 20

module VOLUME(
	input 		clk,
	input		[2:0] VOL,
	input 		[`DATAWIDTH-1:0] in,
	output		reg [`DATAWIDTH-1:0] out
);

	always @(posedge clk) begin
		if(in[15] == 1'b1) out <= 16'h8000 - (in[14:0] >> VOL);
		else  out <= 16'h8000 + (in[14:0] >> VOL);
	end

endmodule

module WAVETABLE(
  input			clk,
  input         [12:0] Fs_input,
  input         [2:0] step_input,
  input			enable,
  input         sub_en,
  output 		[`ADDRWIDTH-1:0] 	RADDR, //wavetable position (8 bit)
  output		[1:0]	RBANK,
  input			[`DATAWIDTH-1:0] RDATA,		//value read from the ram
  output		RCLK,
  output	    [`DATAWIDTH-1:0] dout,
  output        SUB_OUT
);
  
  wire fire;
  reg [`ADDRWIDTH:0] cnt = 9'b0;
  reg [15:0] dlatch = 16'h0;
  reg latch_on = 1'b0;
  
  reg [15:0] Fs = 16'h0;
  reg [2:0] step = 3'b0;
  
  reg SUB_STATE = 1'b0;
  
  reg [`ADDRWIDTH-1:0] raddr;
  reg [1:0] rbank;
  reg rclk;
  
  reg [2:0] enabler = 3'b111;  always @(posedge clk) enabler <= {enabler[1:0], enable};
  //detect enable rising edge
  wire      enable_rising = (enabler[2:1]==2'b01);
  
  //shift reg so we can assert write when all our data has settled
  reg [2:0] shft = 3'b0;
  always @(posedge clk) shft <= {shft[1:0], (fire && latch_on)};
  wire dataRdy = (shft[2:1]==2'b10);
  
  //timer for the sample rate
  TMR t(clk, Fs, latch_on, fire);
  
  //increment the read position and bank if necessary
  always @(posedge clk)
    begin
	  //write Fs and step on rising edge of enable
	  if(enable_rising) begin
		  Fs <= Fs_input;
		  step <= step_input;
          raddr <= 0;
          rbank <= 0;
          cnt <= 0;
		  latch_on <= 1'b1;
	  end
      else if(dataRdy) begin
		  dlatch <= RDATA;

          if( cnt == (8'hFF >> step) )begin
            rbank <= rbank + 1'b1;
            cnt <= 0;
            raddr <= 0;
			
            if(rbank == 2'b11) begin
				SUB_STATE <= ~SUB_STATE;
				if(~enable) latch_on <= 1'b0;
				else begin
					Fs <= Fs_input;
					step <= step_input;
				end
			end
          end else begin
            cnt <= cnt + 1'b1;
            raddr <= raddr + (8'b1 << step);
          end
        end
      end
    
  //set latches
  always @(posedge clk)
    begin
      if(~latch_on) rclk <= 1'b0;
      else rclk <= fire;
    end
	
	assign SUB_OUT = (sub_en && latch_on ? SUB_STATE : 1'bz);
	
	//FOR SIMULATION ONLY
	//assign SUB_OUT = (sub_en && latch_on ? SUB_STATE : 1'b0);
	assign dout = (latch_on ? dlatch : 16'h8000);
	
	assign RBANK = rbank;
	assign RADDR = raddr;
	assign RCLK = rclk;
  
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

module TMR32(
  input clk,
  input [31:0] prescale,
  input enable,
  output fire
);
  
  reg [31:0] cnt = 32'b0;
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
	input [31:0] A_INTERVAL,
	input [31:0] D_INTERVAL,
	input [31:0] R_INTERVAL,
	
	input [6:0] SUS_LVL,
	
	input START,
	
	output [6:0] OUTVALUE,
	output RUNNING
);

	//attack starts on START rising edge, release starts on START falling edge
	reg [2:0] STARTr = 3'b000;  always @(posedge clk) STARTr <= {STARTr[1:0], START};
	wire START_risingedge = (STARTr[1:0]==2'b01);  // detect START rising edges
	wire START_fallingedge = (STARTr[2:1]==2'b10);  // and falling edges
	
	reg [3:0] state = 4'b0;
	wire a_enable = (state == 4'b0001);
	wire d_enable = (state == 4'b0010);
	wire s_enable = (state == 4'b0100);
	wire r_enable = (state == 4'b1000);
	
	wire a_fire, d_fire, r_fire;
	
	reg [6:0] outvalue = 7'h0;
	
	//timer for attack
	TMR32 t_attack(clk, A_INTERVAL, a_enable, a_fire);
	TMR32 t_decay(clk, D_INTERVAL, d_enable, d_fire);
	TMR32 t_release(clk, R_INTERVAL, r_enable, r_fire);
	
	always @(posedge clk) begin
		if(START_risingedge) begin
			outvalue <= 0;
			state <= 4'b0001; //attack state
		end
		else if(a_fire) begin
			outvalue <= outvalue + 1'b1;
			if(outvalue == 7'b1111110) state <= 4'b0010; //decay state
		end
		else if(d_fire) begin
			//decrement outvalue
			outvalue <= outvalue - 1'b1;
			
			//this means sustain level is set to 0. exit here
			if(outvalue == 7'b1) state <= 4'b0000; //done state
			
			else if(outvalue == SUS_LVL + 1'b1) state <= 4'b0100; //sustain state
		end
		
		else if(START_fallingedge && SUS_LVL != 7'h0 && RUNNING) begin
			state <= 4'b1000; //release state
		end
		
		else if(r_fire) begin
			//decrement output
			outvalue <= outvalue - 1'b1;
			if(outvalue == 7'b1) state <= 4'b0000; //done state
		end
		
		//sustain should always be the sustain value
		else if(s_enable) outvalue <= SUS_LVL;
		
	end
	
	assign RUNNING = (state != 4'b0000);
	assign OUTVALUE = outvalue;

endmodule

module PORT(
  input     clk,
  input		[`DATAWIDTH-1:0] PORT_STEP,
  output    [`DATAWIDTH-1:0] GEN_OUT,
  input     [`DATAWIDTH-1:0] TARGET
);

  reg [12:0] fs_latch = 13'h0;
  reg [2:0] step_latch = 3'b0;
  
  reg [`DATAWIDTH-1:0] gen_out = 16'h0;
  
  wire fire;
  
  wire [12:0] FS_TARGET = TARGET[15:3];
  wire [2:0] step_target = TARGET[2:0];
  wire enable = (TARGET != 16'h0);
  
  TMR t(clk, PORT_STEP, 1'b1, fire);
  
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
  
  always @(posedge clk) gen_out <= (enable ? {fs_latch[12:0], step_latch[2:0]} : 16'h0);
  
  assign GEN_OUT = gen_out;

endmodule

module MIX(
	input clk,
	input [6:0] ENV,
	input [`PWM_DEPTH-1:0] DC_PRE,
	input [4:0] MUL,
	output reg [`PWM_DEPTH-1:0] DC_POST
);

	reg [`DATAWIDTH-1:0] tmp = 16'h0;
	wire [`PWM_DEPTH-1:0] outval;
	
	always @(posedge clk) begin
		tmp <= DC_PRE + (ENV * MUL);
		DC_POST <= outval;
	end
	
	assign outval = tmp > 16'hFFF ? 12'hFFF : tmp[11:0];

endmodule

module REG_16(
  input     clk,
  output    reg [15:0] GEN_OUT,
  input     [15:0] WDATA,
  input     WE,
  input     WCLK
);

  always @(posedge WCLK) begin
    if(WE) GEN_OUT <= WDATA;
  end

endmodule

module REG_12(
  input     clk,
  output    reg [11:0] GEN_OUT,
  input     [11:0] WDATA,
  input     WE,
  input     WCLK
);

  always @(posedge WCLK) begin
    if(WE) GEN_OUT <= WDATA;
  end

endmodule

module REG_9(
  input     clk,
  output    reg [8:0] GEN_OUT,
  input     [8:0] WDATA,
  input     WE,
  input     WCLK
);

  always @(posedge WCLK) begin
    if(WE) GEN_OUT <= WDATA;
  end

endmodule

module REG_7(
  input     clk,
  output    reg [6:0] GEN_OUT,
  input     [6:0] WDATA,
  input     WE,
  input     WCLK
);

  always @(posedge WCLK) begin
    if(WE) GEN_OUT <= WDATA;
  end

endmodule

module REG_5(
  input     clk,
  output    reg [4:0] GEN_OUT,
  input     [4:0] WDATA,
  input     WE,
  input     WCLK
);

  always @(posedge WCLK) begin
    if(WE) GEN_OUT <= WDATA;
  end

endmodule

module DINTERP_WAVE(
	input	clk,
	input	enable,
	input	DATA_READY,
	output  reg [3:0] WBANK,
	output 	reg [`ADDRWIDTH-1:0] WADDR
);

  reg [1:0] dummyCnt = 2'b0;
  
  always @(posedge clk) begin
	if(~enable) begin
		WBANK <= 4'b1;
		WADDR <= 8'b0;
		dummyCnt <= 2'b0;
	end
	else if(DATA_READY) begin
		if(dummyCnt !== 2'h3) dummyCnt <= dummyCnt + 1'b1;
		
		else begin
			if( WADDR == 8'hFF )begin
				WBANK <= {WBANK[2:0], 1'b0};
				WADDR <= 0;
			end else begin
				WADDR <= WADDR + 1'b1;
			end
		end
	end
  end
	
endmodule
	
module SPI_WAVE(
	input		clk,
	input		enable,
	input		SCK,
	input		MOSI,
	input		SSEL,
	output		reg [`DATAWIDTH-1:0] DATA_OUT,
	output		reg DATA_READY
);

	reg [2:0] SCKr;  always @(posedge clk) SCKr <= {SCKr[1:0], SCK};
	wire SCK_risingedge = (SCKr[2:1]==2'b01);  // now we can detect SCK rising edges
	wire SCK_fallingedge = (SCKr[2:1]==2'b10);  // and falling edges

	// and for MOSI
	reg [1:0] MOSIr = 2'b00;  always @(posedge clk) MOSIr <= {MOSIr[0], MOSI};
	wire MOSI_data = MOSIr[1];

	// we handle SPI in 16-bits format, so we need a 4 bits counter to count the bits as they come in
	reg [3:0] bitcnt = 4'h0;

	reg byte_received = 1'b0;  // high when a byte has been received
	reg [`DATAWIDTH-1:0] byte_data_received = 16'b0;

	always @(posedge clk)
	begin
	  if(SSEL || ~enable)
		bitcnt <= 4'b0000;
		
	  else if(SCK_risingedge) begin
		bitcnt <= bitcnt + 1'b1;

		// implement a shift-left register (since we receive the data MSB first)
		byte_data_received <= {byte_data_received[14:0], MOSI_data};
	  end
	end

	always @(posedge clk) byte_received <= ~SSEL && SCK_risingedge && (bitcnt==4'b1111);
	always @(posedge clk) begin
		if(byte_received) begin
		  DATA_OUT <= byte_data_received;
		end
		DATA_READY <= byte_received;
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
	input       [`DATAWIDTH-1:0] READ_OUT
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
