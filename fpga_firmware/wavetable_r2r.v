`default_nettype none

`define DATAWIDTH 16
`define ADDRWIDTH 11
`define PWM_DEPTH 12
`define NUM_CHANNELS 20


//there should be one ram arbitrater for each wave bank (4 RAM modules)
module RAM_ARB(
input       clk,
input       [`DATAWIDTH-1:0] WDATA,
input       WE, 
input       [`ADDRWIDTH-1:0] WADDR,
input       WCLK, 
input       [`ADDRWIDTH-1:0] RADDR, 
input       RCLK, 
output      reg [`DATAWIDTH-1:0] RDATA, 
input       [3:0] WE_SUB, 
input       [1:0] rbank
);
  
  wire [`DATAWIDTH-1:0] D0;
  wire [`DATAWIDTH-1:0] D1;
  wire [`DATAWIDTH-1:0] D2;
  wire [`DATAWIDTH-1:0] D3;

  SB_RAM40_4K #(
          .READ_MODE(0),
          .WRITE_MODE(0)
      ) ram0(
      .RDATA(D0),
      .RADDR(RADDR),
      .RCLK(RCLK),
      .RCLKE(1'b1),
      .RE(1'b1),
      .WADDR(WADDR),
      .WCLK(WCLK),
      .WCLKE(1'b1),
      .WDATA(WDATA),
      .WE(WE_SUB[0] && WE),
      .MASK(16'h0)
      );
  SB_RAM40_4K #(
          .READ_MODE(0),
          .WRITE_MODE(0)
      ) ram1(
      .RDATA(D1),
      .RADDR(RADDR),
      .RCLK(RCLK),
      .RCLKE(1'b1),
      .RE(1'b1),
      .WADDR(WADDR),
      .WCLK(WCLK),
      .WCLKE(1'b1),
      .WDATA(WDATA),
      .WE(WE_SUB[1] && WE),
      .MASK(16'h0)
      );
  SB_RAM40_4K #(
          .READ_MODE(0),
          .WRITE_MODE(0)
      ) ram2(
      .RDATA(D2),
      .RADDR(RADDR),
      .RCLK(RCLK),
      .RCLKE(1'b1),
      .RE(1'b1),
      .WADDR(WADDR),
      .WCLK(WCLK),
      .WCLKE(1'b1),
      .WDATA(WDATA),
      .WE(WE_SUB[2] && WE),
      .MASK(16'h0)
      );
  SB_RAM40_4K #(
          .READ_MODE(0),
          .WRITE_MODE(0)
      ) ram3(
      .RDATA(D3),
      .RADDR(RADDR),
      .RCLK(RCLK),
      .RCLKE(1'b1),
      .RE(1'b1),
      .WADDR(WADDR),
      .WCLK(WCLK),
      .WCLKE(1'b1),
      .WDATA(WDATA),
      .WE(WE_SUB[3] && WE),
      .MASK(16'h0)
      );

  always @(posedge clk) begin
       if(rbank == 0) RDATA <= D0;
       else if(rbank == 1) RDATA <= D1;
       else if(rbank == 2) RDATA <= D2;
       else if(rbank == 3) RDATA <= D3;
  end
  
endmodule

module top(
input clk,
input S_MOSI,
output S_MISO,
input S_SCK,
input S_CS,
input R_MOSI,
input R_SCK,
input R_CS,
output [`DATAWIDTH-1:0] OUT0,
output [`DATAWIDTH-1:0] OUT1,
output [`DATAWIDTH-1:0] OUT2,
output NOISE_OUT,
output [4:0] PWM_OUT,
output [2:0] SUB_OUT
);

  wire [`ADDRWIDTH-1:0] WADDR;
  wire [`DATAWIDTH-1:0] RDATA0, RDATA1, RDATA2, WDATA_RAM;
  wire [31:0] WDATA;
  reg [`DATAWIDTH-1:0] READ_OUT = 16'h0;
  
  wire [1:0] RBANK0, RBANK1, RBANK2;
  wire [3:0] WBANK;
  wire [`ADDRWIDTH-1:0] RADDR0, RADDR1, RADDR2;
  
  //for envelope
  wire [6:0] OUTVALUE;

  wire [`DATAWIDTH-1:0] FS0, FS1, FS2;

  wire SPI_RDY, RCLK0, RCLK1, RCLK2, lock, DATA_READY_RAM;
  
  wire ENV_RUNNING = 1'b1;
  
  wire [`DATAWIDTH-1:0] PORT_STEP;
  wire [`DATAWIDTH-1:0] A_INTERVAL_H;
  wire [`DATAWIDTH-1:0] A_INTERVAL_L; 
  wire [`DATAWIDTH-1:0] D_INTERVAL_H;
  wire [`DATAWIDTH-1:0] D_INTERVAL_L;
  wire [`DATAWIDTH-1:0] R_INTERVAL_H; 
  wire [`DATAWIDTH-1:0] R_INTERVAL_L;
  wire [`DATAWIDTH-1:0] PORT_TARGET0;
  wire [`DATAWIDTH-1:0] PORT_TARGET1;
  wire [`DATAWIDTH-1:0] PORT_TARGET2;
	
  wire [`PWM_DEPTH-1:0] PWM_DC1;
  wire [`PWM_DEPTH-1:0] PWM_DC3;
  wire [`PWM_DEPTH-1:0] PWM_DC4;
  wire [`PWM_DEPTH-1:0] PWM_DC_PRE_0;
  wire [`PWM_DEPTH-1:0] PWM_DC_PRE_2;
  
  wire [`PWM_DEPTH-1:0] PWM_DC0;
  wire [`PWM_DEPTH-1:0] PWM_DC2;
	
  wire [8:0] EN_REG;
  wire [8:0] VOL;
  wire [6:0] SUS_LVL;
  
  wire [4:0] ENV0_PWM0_MUL;
  wire [4:0] ENV0_PWM2_MUL;
  
  reg [`DATAWIDTH-1:0] WD;
  
`ifdef SIM
  wire clkout = clk;
`else
  wire clkout;
  
  SB_PLL40_CORE #(
        .FEEDBACK_PATH("SIMPLE"),
        .PLLOUT_SELECT("GENCLK"),
        .DIVR(4'b0000),
        .DIVF(7'b0010111),
        .DIVQ(3'b010),
        .FILTER_RANGE(3'b010)
    ) uut (
        .LOCK(lock),
        .RESETB(1'b1),
        .BYPASS(1'b0),
        .REFERENCECLK(clk),
        .PLLOUTCORE(clkout)
    );
`endif

  
  wire [7:0] write_addr = WDATA[29:22];
  reg WCLK = 1'b0;
  reg [`NUM_CHANNELS-1:0] WE = `NUM_CHANNELS'h0;
  
  reg [2:0] write_shft = 2'b00;
  always @(posedge clk) write_shft <= {write_shft[0], SPI_RDY};
  wire write_latch = (write_shft[1:0]==2'b10);
  
  //decode read enable and set spi out
  always @(posedge clk) begin
	if(SPI_RDY) begin
		if(WDATA[31]) begin
			WE <= (1 << write_addr);
			WD <= WDATA[16:0];
		end
		
		else begin
			WE <= `NUM_CHANNELS'h0;
		end
	end
	WCLK <= write_latch;
  end
  
  always @(*) begin
	case(write_addr)
	8'h00: READ_OUT <= FS0;
	8'h01: READ_OUT <= FS1;
	8'h02: READ_OUT <= FS2;
	8'h03: READ_OUT <= PWM_DC_PRE_0;
	8'h04: READ_OUT <= PWM_DC1;
	8'h05: READ_OUT <= PWM_DC_PRE_2;
	8'h06: READ_OUT <= PWM_DC3;
	8'h07: READ_OUT <= PWM_DC4;
	8'h08: READ_OUT <= VOL;
	8'h09: READ_OUT <= EN_REG;
	8'h0a: READ_OUT <= A_INTERVAL_H;
	8'h0b: READ_OUT <= A_INTERVAL_L;
	8'h0c: READ_OUT <= D_INTERVAL_H;
	8'h0d: READ_OUT <= D_INTERVAL_L;
	8'h0e: READ_OUT <= R_INTERVAL_H;
	8'h0f: READ_OUT <= R_INTERVAL_L;
	8'h10: READ_OUT <= SUS_LVL;
	8'h11: READ_OUT <= ENV0_PWM0_MUL;
	8'h12: READ_OUT <= ENV0_PWM2_MUL;
	8'h13: READ_OUT <= PORT_STEP;
	endcase
  end
  
  REG_16 fs0(clkout, FS0, WD, WE[0], WCLK);
  REG_16 fs1(clkout, FS1, WD, WE[1], WCLK);
  REG_16 fs2(clkout, FS2, WD, WE[2], WCLK);
  
  REG_12 dc0(clkout, PWM_DC0, WD[11:0], WE[3], WCLK);
  REG_12 dc1(clkout, PWM_DC1, WD[11:0], WE[4], WCLK);
  REG_12 dc2(clkout, PWM_DC2, WD[11:0], WE[5], WCLK);
  REG_12 dc3(clkout, PWM_DC3, WD[11:0], WE[6], WCLK);
  REG_12 dc4(clkout, PWM_DC4, WD[11:0], WE[7], WCLK);
  
  REG_9 vol(clkout, VOL, WD[8:0], WE[8], WCLK);
  REG_9 en(clkout, EN_REG, WD[8:0], WE[9], WCLK);
  
  REG_16 attack_h(clkout, A_INTERVAL_H, WD, WE[10], WCLK);
  REG_16 attack_l(clkout, A_INTERVAL_L, WD, WE[11], WCLK);
  REG_16 decay_h(clkout, D_INTERVAL_H, WD, WE[12], WCLK);
  REG_16 decay_l(clkout, D_INTERVAL_L, WD, WE[13], WCLK);
  REG_16 release_h(clkout, R_INTERVAL_H, WD, WE[14], WCLK);
  REG_16 release_l(clkout, R_INTERVAL_L, WD, WE[15], WCLK);
  
  REG_7 sus(clkout, SUS_LVL, WD[6:0], WE[16], WCLK);
  
  REG_5 env_mul0(clkout, ENV0_PWM0_MUL, WD[4:0], WE[17], WCLK);
  REG_5 env_mul2(clkout, ENV0_PWM2_MUL, WD[4:0], WE[18], WCLK);
  
  REG_16 port_step(clkout, PORT_STEP, WD[15:0], WE[19], WCLK);

  //receives SPI data
  SPI_SLAVE spi(clkout, S_SCK, S_MOSI, S_MISO, S_CS, WDATA, SPI_RDY, READ_OUT);
  
  //for setting subosc active
  //PORT sub_fs0(clkout, PORT_STEP, FS0, PORT_TARGET0);
  //PORT sub_fs1(clkout, PORT_STEP, FS1, PORT_TARGET1);
  //PORT sub_fs2(clkout, PORT_STEP, FS2, PORT_TARGET2);
  
  
  //****** ENVELOPE 0 ******//
  
  //ADSR env0(clkout, {A_INTERVAL_H, A_INTERVAL_L}, {D_INTERVAL_H, D_INTERVAL_L}, {R_INTERVAL_H, R_INTERVAL_L}, SUS_LVL, EN_REG[8], OUTVALUE, ENV_RUNNING);
  
  //mix for cutoff
  //MIX mix_env0_pwm0(clkout, OUTVALUE, PWM_DC_PRE_0, ENV0_PWM0_MUL, PWM_DC0);
  
  //mix for VCA
  //MIX mix_env0_pwm2(clkout, OUTVALUE, PWM_DC_PRE_2, ENV0_PWM2_MUL, PWM_DC2);
  
  //************************//

  //wavetable module reads table and sends data out to R2R DAC
  WAVETABLE w0(clkout, FS0[15:3], FS0[2:0], (ENV_RUNNING && FS0 != 16'h0), EN_REG[0], RADDR0, RBANK0, RDATA0, RCLK0, vlink0, SUB_OUT[0]);
  WAVETABLE w1(clkout, FS1[15:3], FS1[2:0], (ENV_RUNNING && FS1 != 16'h0), EN_REG[1], RADDR1, RBANK1, RDATA1, RCLK1, vlink1, SUB_OUT[1]);
  WAVETABLE w2(clkout, FS2[15:3], FS2[2:0], (ENV_RUNNING && FS2 != 16'h0), EN_REG[2], RADDR2, RBANK2, RDATA2, RCLK2, vlink2, SUB_OUT[2]);
  
  wire [`DATAWIDTH-1:0] vlink0, vlink1, vlink2;
  
  VOLUME v0(clkout, VOL[2:0], vlink0, OUT0);
  VOLUME v1(clkout, VOL[5:3], vlink1, OUT1);
  VOLUME v2(clkout, VOL[8:6], vlink2, OUT2);
  
  //SPI_WAVE sw(clkout, EN_REG[7], R_SCK, R_MOSI, R_CS, WDATA_RAM, DATA_READY_RAM);
  
  //DINTERP_WAVE dwave(clkout, EN_REG[7], DATA_READY_RAM, WBANK, WADDR);
  
  //ram arbitrators
  RAM_ARB r0 (clkout, WDATA_RAM, EN_REG[4], WADDR, DATA_READY_RAM, RADDR0, RCLK0, RDATA0, WBANK, RBANK0);
  RAM_ARB r1 (clkout, WDATA_RAM, EN_REG[5], WADDR, DATA_READY_RAM, RADDR1, RCLK1, RDATA1, WBANK, RBANK1);
  RAM_ARB r2 (clkout, WDATA_RAM, EN_REG[6], WADDR, DATA_READY_RAM, RADDR2, RCLK2, RDATA2, WBANK, RBANK2);

  PWM p0 (clkout, PWM_OUT[0], PWM_DC0);
  PWM p1 (clkout, PWM_OUT[1], PWM_DC1);
  PWM p2 (clkout, PWM_OUT[2], PWM_DC2);
  PWM p3 (clkout, PWM_OUT[3], PWM_DC3);
  PWM p4 (clkout, PWM_OUT[4], PWM_DC4);
  
  NOISE n (clkout, {3'b000, ( FS0[15:3] >> (3'h7 - FS0[2:0]) )}, (EN_REG[3] && ENV_RUNNING), NOISE_OUT);

endmodule