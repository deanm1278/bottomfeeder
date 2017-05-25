`default_nettype none

`define DATAWIDTH 16
`define ADDRWIDTH 8
`define PWM_DEPTH 12
`define NUM_CHANNELS 24


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
input       [1:0] wbank, 
input       [1:0] rbank
);
  
  reg [3:0] WE_SUB = 4'b0; //write enables
  reg [`DATAWIDTH-1:0] D0;
  reg [`DATAWIDTH-1:0] D1;
  reg [`DATAWIDTH-1:0] D2;
  reg [`DATAWIDTH-1:0] D3;

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
      .WE(WE_SUB[0]),
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
      .WE(WE_SUB[1]),
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
      .WE(WE_SUB[2]),
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
      .WE(WE_SUB[3]),
      .MASK(16'h0)
      );

  always @(posedge clk) begin
       if(rbank == 0) RDATA <= D0;
       else if(rbank == 1) RDATA <= D1;
       else if(rbank == 2) RDATA <= D2;
       else if(rbank == 3) RDATA <= D3;
  end
  always @(posedge clk) begin
    if(~WE) WE_SUB <= 4'h0;
    else WE_SUB <= (4'b01 << wbank);
  end
  
endmodule

module top(
input clk,
input S_MOSI,
output S_MISO,
input S_SCK,
input S_CS,
output [`DATAWIDTH-1:0] OUT0,
output [`DATAWIDTH-1:0] OUT1,
output [`DATAWIDTH-1:0] OUT2,
output NOISE_OUT,
output [4:0] PWM_OUT,
output [2:0] SUB_OUT
);

  wire [`ADDRWIDTH-1:0] WADDR;
  wire [`DATAWIDTH-1:0] WDATA, RDATA0, RDATA1, RDATA2;
  wire [31:0] SPI_OUT;
  wire [`DATAWIDTH:0] READ_OUT;
  
  wire [1:0] RBANK0, RBANK1, RBANK2, WBANK;
  wire [`ADDRWIDTH-1:0] RADDR0, RADDR1, RADDR2;
  wire [`NUM_CHANNELS-1:0] WE;
  wire [7:0] RE;

  wire [3:0] EN_REG;
  wire KEY_PRESSED;
  wire [`DATAWIDTH-1:0] FS0, FS1, FS2, PORT_STEP, VOL;
  wire NOISE_EN;

  wire [`DATAWIDTH-1:0] ENV0_PWM0_MUL, ENV0_PWM2_MUL;
  
  wire [`DATAWIDTH-1:0] PWM_DC0, PWM_DC1, PWM_DC2, PWM_DC3, PWM_DC4;
  
  //unscaled duty cycle
  wire [`DATAWIDTH-1:0] PWM_DC_PRE_0, PWM_DC_PRE_2;
  
  //for envelope
  wire [`DATAWIDTH-1:0] A_INTERVAL_H, A_INTERVAL_L, D_INTERVAL_H, D_INTERVAL_L, SUS_LVL, R_INTERVAL_H, R_INTERVAL_L;
  wire [`DATAWIDTH-1:0] OUTVALUE;

  wire SPI_RDY, WCLK, RCLK0, RCLK1, RCLK2, clkout, lock, ENV_RUNNING;

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

  //decode read enable and set spi out
  always @(posedge clk) begin
  
	case(RE)
	//8'h00: READ_OUT <= RDATA0;
	//8'h01: READ_OUT <= RDATA1;
	//8'h02: READ_OUT <= RDATA2;
	8'h03: READ_OUT <= FS0;
	8'h04: READ_OUT <= FS1;
	8'h05: READ_OUT <= FS2;
	8'h06: READ_OUT <= PWM_DC_PRE_0;
	8'h07: READ_OUT <= PWM_DC1;
	8'h08: READ_OUT <= PWM_DC_PRE_2;
	8'h09: READ_OUT <= PWM_DC3;
	8'h0A: READ_OUT <= PWM_DC4;
	8'h0B: READ_OUT <= VOL;
	8'h0C: READ_OUT <= EN_REG;
	8'h0D: READ_OUT <= A_INTERVAL_H;
	8'h0E: READ_OUT <= A_INTERVAL_L;
	8'h0F: READ_OUT <= D_INTERVAL_H;
	8'h10: READ_OUT <= D_INTERVAL_L;
	8'h11: READ_OUT <= R_INTERVAL_H;
	8'h12: READ_OUT <= R_INTERVAL_L;
	8'h13: READ_OUT <= SUS_LVL;
	8'h14: READ_OUT <= ENV0_PWM0_MUL;
	8'h15: READ_OUT <= ENV0_PWM2_MUL;
	8'h16: READ_OUT <= PORT_STEP;
	endcase
  end

  //receives SPI data
  SPI_SLAVE spi(clkout, S_SCK, S_MOSI, S_MISO, S_CS, SPI_OUT, SPI_RDY, READ_OUT);
  
  //interprests and decodes SPI data, routes to correct RAM module and address
  DINTERP interp(clkout, SPI_RDY, SPI_OUT, WBANK, WADDR, WE, RE, WCLK, WDATA);
  
  //for setting subosc active
  GEN_REG en_reg(clkout, EN_REG, WDATA, WE[12], WCLK);
  PORT sub_fs0(clkout, PORT_STEP, FS0, WDATA, WE[3], WCLK);
  PORT sub_fs1(clkout, PORT_STEP, FS1, WDATA, WE[4], WCLK);
  PORT sub_fs2(clkout, PORT_STEP, FS2, WDATA, WE[5], WCLK);
  GEN_REG pwm_dc0(clkout, PWM_DC_PRE_0, WDATA, WE[6], WCLK);
  GEN_REG pwm_dc1(clkout, PWM_DC1, WDATA, WE[7], WCLK);
  GEN_REG pwm_dc2(clkout, PWM_DC_PRE_2, WDATA, WE[8], WCLK);
  GEN_REG pwm_dc3(clkout, PWM_DC3, WDATA, WE[9], WCLK);
  GEN_REG pwm_dc4(clkout, PWM_DC4, WDATA, WE[10], WCLK);
  GEN_REG vol(clkout, VOL, WDATA, WE[11], WCLK);
  GEN_REG key_pressed(clkout, KEY_PRESSED, WDATA, WE[23], WCLK);
  GEN_REG port_step(clkout, PORT_STEP, WDATA, WE[22], WCLK);
  
  
  //****** ENVELOPE 0 ******//
  
  GEN_REG env0_attack_interval_high(clkout, A_INTERVAL_H, WDATA, WE[13], WCLK);
  GEN_REG env0_attack_interval_low(clkout, A_INTERVAL_L, WDATA, WE[14], WCLK);
  
  GEN_REG env0_decay_interval_high(clkout, D_INTERVAL_H, WDATA, WE[15], WCLK);
  GEN_REG env0_decay_interval_low(clkout, D_INTERVAL_L, WDATA, WE[16], WCLK);
  
  GEN_REG env0_release_interval_high(clkout, R_INTERVAL_H, WDATA, WE[17], WCLK);
  GEN_REG env0_release_interval_low(clkout, R_INTERVAL_L, WDATA, WE[18], WCLK);
  
  GEN_REG env0_sustain(clkout, SUS_LVL, WDATA, WE[19], WCLK);
  
  ADSR env0(clkout, {A_INTERVAL_H, A_INTERVAL_L}, {D_INTERVAL_H, D_INTERVAL_L}, {R_INTERVAL_H, R_INTERVAL_L}, SUS_LVL, KEY_PRESSED, OUTVALUE, ENV_RUNNING);
  
  //mix for cutoff
  GEN_REG env0_pwm0_mul(clkout, ENV0_PWM0_MUL, WDATA, WE[20], WCLK);
  MIX mix_env0_pwm0(clk, OUTVALUE, PWM_DC_PRE_0[`PWM_DEPTH-1:0], ENV0_PWM0_MUL, PWM_DC0);
  
  //mix for VCA
  GEN_REG env0_pwm2_mul(clkout, ENV0_PWM2_MUL, WDATA, WE[21], WCLK);
  MIX mix_env0_pwm2(clk, OUTVALUE, PWM_DC_PRE_2[`PWM_DEPTH-1:0], ENV0_PWM2_MUL, PWM_DC2);
  
  //************************//

  //wavetable module reads table and sends data out to R2R DAC
  WAVETABLE w0(clkout, FS0[15:3], FS0[2:0], (ENV_RUNNING && FS0 != 16'h0), EN_REG[0], RADDR0, RBANK0, RDATA0, RCLK0, vlink0, SUB_OUT[0]);
  WAVETABLE w1(clkout, FS1[15:3], FS1[2:0], (ENV_RUNNING && FS1 != 16'h0), EN_REG[1], RADDR1, RBANK1, RDATA1, RCLK1, vlink1, SUB_OUT[1]);
  WAVETABLE w2(clkout, FS2[15:3], FS2[2:0], (ENV_RUNNING && FS2 != 16'h0), EN_REG[2], RADDR2, RBANK2, RDATA2, RCLK2, vlink2, SUB_OUT[2]);
  
  wire [`DATAWIDTH-1:0] vlink0, vlink1, vlink2;
  
  VOLUME v0(clkout, VOL[2:0], vlink0, OUT0);
  VOLUME v1(clkout, VOL[5:3], vlink1, OUT1);
  VOLUME v2(clkout, VOL[8:6], vlink2, OUT2);
  
  //ram arbitrators
  RAM_ARB r0 (clkout, WDATA, WE[0], WADDR, WCLK, RADDR0, RCLK0, RDATA0, WBANK, RBANK0);
  RAM_ARB r1 (clkout, WDATA, WE[1], WADDR, WCLK, RADDR1, RCLK1, RDATA1, WBANK, RBANK1);
  RAM_ARB r2 (clkout, WDATA, WE[2], WADDR, WCLK, RADDR2, RCLK2, RDATA2, WBANK, RBANK2);

  PWM p0 (clkout, PWM_OUT[0], PWM_DC0[`PWM_DEPTH-1:0]);
  PWM p1 (clkout, PWM_OUT[1], PWM_DC1[`PWM_DEPTH-1:0]);
  PWM p2 (clkout, PWM_OUT[2], PWM_DC2[`PWM_DEPTH-1:0]);
  PWM p3 (clkout, PWM_OUT[3], PWM_DC3[`PWM_DEPTH-1:0]);
  PWM p4 (clkout, PWM_OUT[4], PWM_DC4[`PWM_DEPTH-1:0]);
  
  //NOISE n (clkout, ( FS0[15:3] >> (3'h7 - FS0[2:0]) ), (EN_REG[3] && ENV_RUNNING), NOISE_OUT);

endmodule