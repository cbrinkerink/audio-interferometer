// This lag correlator builds upon the elementary correlator, which was used
// to test the functionality for 2 microphone inputs. As we now want to read
// out 8 microphones and use a larger number of lags, the performance will
// have to be upgraded considerably. We also need to add some form of data
// output.
// To change or add:
// - DONE: Increase clock speed from 12 MHz to 120 MHz. This gives us more time per
// audio sample (640 clock cycles instead of 64) to process all the
// correlations.
// - IN PROGRESS: Divide the tasks to be done over the clock cycles in a different way: we
// will want to shift one audio sample buffer at a time, then calculate all
// correlations of that microphone with the other 7. We will therefore get
// a particular structure which is repeated, and hopefully allows us to code
// it in some form of shorthand.
// - Note that each time we correlate one microphone with the other 7, we need
// to keep track of the even/odd lag bins we are updating. This pattern
// changes for every microphone input we process.
// - DONE: We will have to work out how to transmit all the accumulator buffer data
// once an integration period is complete. We should set aside dedicated
// registers that hold these values while the next integration is already
// underway. Then, a separate communications block can take care of
// transmission.
//
// With 56 multipliers on the ECP5, we need to structure our multiplications.
// As we will need to shift the audio samples in order (1 mic at a time),
// we will need to process all the lags from one newly shifted microphone
// before moving on to the next one. It is therefore convenient to have
// a suitable number of lag bins. Note that we need to process the even or odd
// lags at one time only, so we should work with half the lag bins.
// One sample is 7.3mm of traveled distance for sound, so 56 lags is 40.98 cm.
// We can start with 112 lags (81.96 cm), so that we can process half of those in one
// clock cycle.
// We will do all correlations of mic 1 with mics 2 thru 8, taking 7 clock
// cycles. Then the other 7 mics are up, taking 7 * 7 = 49 more clock cycles.
// All correlation is then done in 56 clock cycles.
//
// We will also need to take care of packetising the output lag buffer!
// Currently, it spits out all its contents in one long series of bits. We
// need to adopt this to use packets of 10 bits, with 1 start bit, 8 payload
// bits and 1 stop bit so that the serial interface can read it properly.

`timescale 1ns/1ns
`default_nettype none

module top (input wire clk_100MHz, // Change when using sim
	    //input wire clk_120MHz, // Change when using sim
	    //input wire clk_12MHz, // Change when using sim
	    input wire mic_data_1,
	    input wire mic_data_2,
	    input wire mic_data_3,
	    input wire mic_data_4,
	    input wire mic_data_5,
	    input wire mic_data_6,
            output wire uart_tx,
	    output wire mic_clk_1,
	    output wire mic_clk_2,
	    output wire mic_clk_3,
	    output wire mic_clk_4,
	    output wire mic_clk_5,
	    output wire mic_clk_6,
	    output wire split_out_1,
	    output wire split_out_2,
	    output wire split_out_3,
            output wire LED_1,
            output wire LED_2,
            output wire LED_3,
            output wire LED_4,
            // Inputs
            input i_versa_rst_n, // reset
            input clk_125MHz, // PHY output clock, CDB added for debugging
            // liteeth inputs (output of PHY)
            input i_liteeth_rgmii_eth_clocks_rx,
            input i_liteeth_rgmii_eth_rx_ctl,
            input [3:0] i_liteeth_rgmii_eth_rx_data,
            // liteeth outputs (input for PHY)
            output o_liteeth_rgmii_eth_clocks_tx,
            output o_liteeth_rgmii_eth_tx_ctl,
            output [3:0] o_liteeth_rgmii_eth_tx_data,
            output o_liteeth_rgmii_eth_rst_n,
	    // PHY management 
            output o_liteeth_rgmii_eth_mdc,
            inout i_liteeth_rgmii_eth_mdio
    );

parameter NUMLAGS = 128;

// LiteEth connections for sending data
wire liteeth_udp_tx_valid;
wire liteeth_udp_tx_last;
wire [31:0] liteeth_udp_tx_data;

wire liteeth_udp_tx_ready;

// LiteEth clock wires
wire i_liteeth_rgmii_eth_clocks_rx;
wire o_liteeth_rgmii_eth_clocks_tx;

// Unused LiteEth connections
wire liteeth_udp_rx_valid;
wire liteeth_udp_rx_last;
wire liteeth_udp_rx_ready;
wire [31:0] liteeth_udp_rx_data;
wire liteeth_udp_rx_error;

wire [17:0] CIC_to_correlator_1;
wire [17:0] CIC_to_correlator_2;
wire [17:0] CIC_to_correlator_3;
wire [17:0] CIC_to_correlator_4;
wire [17:0] CIC_to_correlator_5;
wire [17:0] CIC_to_correlator_6;

wire [17:0] d_test_1;
wire [17:0] d_test_2;
wire [17:0] d_test_3;
wire [17:0] d_test_4;
wire [17:0] d_test_5;
wire [17:0] d_test_6;
wire [17:0] d_test_7;
wire [17:0] d_test_8;
wire [17:0] d_test_9;
wire [17:0] d_test_10;
wire [17:0] d_test_11;
wire [17:0] d_test_12;

wire load_data_1;
wire locked;

(* keep="soft" *)
wire load_data_2;
wire load_data_3;
wire load_data_4;
wire load_data_5;
wire load_data_6;

wire data_out;

// --- Comment out if using sim ---
wire clk_12MHz;
wire clk_120MHz;

//-- PDM clock
wire clk_3MHz;

reg [27:0] long_counter = 0;

wire [31:0] lag_data_12;
wire [31:0] lag_data_23;
wire [31:0] lag_data_31;
wire [31:0] lag_data_14;
wire [31:0] lag_data_24;
wire [31:0] lag_data_34;
wire [31:0] lag_data_15;
wire [31:0] lag_data_25;
wire [31:0] lag_data_35;
wire [31:0] lag_data_45;
wire [31:0] lag_data_16;
wire [31:0] lag_data_26;
wire [31:0] lag_data_36;
wire [31:0] lag_data_46;
wire [31:0] lag_data_56;
wire export_flag_12;
wire export_flag_23;
wire export_flag_31;
wire export_flag_14;
wire export_flag_24;
wire export_flag_34;
wire export_flag_15;
wire export_flag_25;
wire export_flag_35;
wire export_flag_45;
wire export_flag_16;
wire export_flag_26;
wire export_flag_36;
wire export_flag_46;
wire export_flag_56;
wire serial_data;

reg LED_1_state = 0;
reg LED_2_state = 0;
reg LED_3_state = 0;
reg LED_4_state = 0;

//always @ (posedge clk_100MHz) begin
//  long_counter = long_counter + 1;
//  if (long_counter[24] == 1) LED_1_state = ~ LED_1_state;
//  if (long_counter[23] == 1) LED_2_state = ~ LED_2_state;
//  if (long_counter[22] == 1) LED_3_state = ~ LED_3_state;
//  if (long_counter[21] == 1) LED_4_state = ~ LED_4_state;
//end

assign LED_1 = LED_1_state;
assign LED_2 = LED_2_state;
assign LED_3 = LED_3_state;
assign LED_4 = LED_4_state;

assign mic_clk_1 = clk_3MHz;
assign mic_clk_2 = clk_3MHz;
assign mic_clk_3 = clk_3MHz;
assign mic_clk_4 = clk_3MHz;
assign mic_clk_5 = clk_3MHz;
assign mic_clk_6 = clk_3MHz;

assign uart_tx = serial_data;
assign split_out_1 = mic_data_1;
assign split_out_2 = mic_data_2;
assign split_out_3 = mic_data_3;

// Change when using sim
pll_100_12 PLL_SLOW (.clk_100(clk_100MHz), .clk_12(clk_12MHz));

// 6 microphones

samplemanager #(.numlags(NUMLAGS)) sample1 (.d_in(CIC_to_correlator_1),
                                            .clk_in(clk_100MHz), 
	                                    .sample_ready(load_data_1),
                                            .d_out_1(d_test_1),
	                                    .d_out_2(d_test_2));

samplemanager #(.numlags(NUMLAGS)) sample2 (.d_in(CIC_to_correlator_2),
                                            .clk_in(clk_100MHz), 
	                                    .sample_ready(load_data_1),
                                            .d_out_1(d_test_3),
	                                    .d_out_2(d_test_4));

samplemanager #(.numlags(NUMLAGS)) sample3 (.d_in(CIC_to_correlator_3),
                                            .clk_in(clk_100MHz), 
	                                    .sample_ready(load_data_1),
                                            .d_out_1(d_test_5),
	                                    .d_out_2(d_test_6));

samplemanager #(.numlags(NUMLAGS)) sample4 (.d_in(CIC_to_correlator_4),
                                            .clk_in(clk_100MHz), 
	                                    .sample_ready(load_data_1),
                                            .d_out_1(d_test_7),
	                                    .d_out_2(d_test_8));

samplemanager #(.numlags(NUMLAGS)) sample5 (.d_in(CIC_to_correlator_5),
                                            .clk_in(clk_100MHz), 
	                                    .sample_ready(load_data_1),
                                            .d_out_1(d_test_9),
	                                    .d_out_2(d_test_10));

samplemanager #(.numlags(NUMLAGS)) sample6 (.d_in(CIC_to_correlator_6),
                                            .clk_in(clk_100MHz), 
	                                    .sample_ready(load_data_1),
                                            .d_out_1(d_test_11),
	                                    .d_out_2(d_test_12));

// 15 baselines

lagmanager #(.numlags(NUMLAGS)) lags12 (.d_in_1(d_test_1),
                                        .d_in_2(d_test_4),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_12),
                                        .export_active(export_flag_12));

lagmanager #(.numlags(NUMLAGS)) lags23 (.d_in_1(d_test_3),
                                        .d_in_2(d_test_6),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_23),
                                        .export_active(export_flag_23));

lagmanager #(.numlags(NUMLAGS)) lags31 (.d_in_1(d_test_5),
                                        .d_in_2(d_test_2),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_31),
                                        .export_active(export_flag_31));

lagmanager #(.numlags(NUMLAGS)) lags14 (.d_in_1(d_test_1),
                                        .d_in_2(d_test_8),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_14),
                                        .export_active(export_flag_14));

lagmanager #(.numlags(NUMLAGS)) lags24 (.d_in_1(d_test_3),
                                        .d_in_2(d_test_8),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_24),
                                        .export_active(export_flag_24));

lagmanager #(.numlags(NUMLAGS)) lags34 (.d_in_1(d_test_5),
                                        .d_in_2(d_test_8),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_34),
                                        .export_active(export_flag_34));

lagmanager #(.numlags(NUMLAGS)) lags15 (.d_in_1(d_test_1),
                                        .d_in_2(d_test_10),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_15),
                                        .export_active(export_flag_15));

lagmanager #(.numlags(NUMLAGS)) lags25 (.d_in_1(d_test_3),
                                        .d_in_2(d_test_10),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_25),
                                        .export_active(export_flag_25));

lagmanager #(.numlags(NUMLAGS)) lags35 (.d_in_1(d_test_5),
                                        .d_in_2(d_test_10),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_35),
                                        .export_active(export_flag_35));

lagmanager #(.numlags(NUMLAGS)) lags45 (.d_in_1(d_test_7),
                                        .d_in_2(d_test_10),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_45),
                                        .export_active(export_flag_45));

lagmanager #(.numlags(NUMLAGS)) lags16 (.d_in_1(d_test_1),
                                        .d_in_2(d_test_12),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_16),
                                        .export_active(export_flag_16));

lagmanager #(.numlags(NUMLAGS)) lags26 (.d_in_1(d_test_3),
                                        .d_in_2(d_test_12),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_26),
                                        .export_active(export_flag_26));

lagmanager #(.numlags(NUMLAGS)) lags36 (.d_in_1(d_test_5),
                                        .d_in_2(d_test_12),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_36),
                                        .export_active(export_flag_36));

lagmanager #(.numlags(NUMLAGS)) lags46 (.d_in_1(d_test_7),
                                        .d_in_2(d_test_12),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_46),
                                        .export_active(export_flag_46));

lagmanager #(.numlags(NUMLAGS)) lags56 (.d_in_1(d_test_9),
                                        .d_in_2(d_test_12),
                                        .clk_in(clk_100MHz),
                                        .sample_ready(load_data_1),
                                        .d_out(lag_data_56),
                                        .export_active(export_flag_56));

transfermanager #(.numlags(NUMLAGS)) transfer (.d_in_1(lag_data_12),
                                               .d_in_2(lag_data_23),
                                               .d_in_3(lag_data_31),
                                               .d_in_4(lag_data_14),
                                               .d_in_5(lag_data_24),
                                               .d_in_6(lag_data_34),
                                               .d_in_7(lag_data_15),
                                               .d_in_8(lag_data_25),
                                               .d_in_9(lag_data_35),
                                               .d_in_10(lag_data_45),
                                               .d_in_11(lag_data_16),
                                               .d_in_12(lag_data_26),
                                               .d_in_13(lag_data_36),
                                               .d_in_14(lag_data_46),
                                               .d_in_15(lag_data_56),
                                               .clk_in(clk_100MHz),
                                               .clk_in_2(clk_100MHz),
                                               .export_active(export_flag_12),
                                               .d_out(serial_data),
				               .eth_data_valid(liteeth_udp_tx_valid),
				               .eth_data_last(liteeth_udp_tx_last),
				               .eth_data(liteeth_udp_tx_data));


// Stuff for ethernet debugging

//reg [24 * 8 - 1:0] all_data = 192'h17161514131211100F0E0D0C0B0A09080706050403020100;
//reg [31:0] data_to_send = 32'h00000000;
//reg data_valid = 0;
//reg data_last = 0;
//reg [23:0] counter = 0;

//assign liteeth_udp_tx_valid = data_valid;
//assign liteeth_udp_tx_last = data_last;
//assign liteeth_udp_tx_data = data_to_send;

// Instantiate liteeth core
liteeth_core liteeth_core_inst
       // liteeth inputs
    ( .sys_clock                       (clk_100MHz) // Trying the 100 MHz clock here
    , .sys_reset                       (0)
    , .rgmii_eth_clocks_rx             (i_liteeth_rgmii_eth_clocks_rx)
    , .rgmii_eth_clocks_tx             (o_liteeth_rgmii_eth_clocks_tx)
    , .rgmii_eth_rst_n                 (o_liteeth_rgmii_eth_rst_n)
    , .rgmii_eth_int_n                 (1)
    , .rgmii_eth_mdio                  (i_liteeth_rgmii_eth_mdio)
    , .rgmii_eth_mdc                   (o_liteeth_rgmii_eth_mdc)
    , .rgmii_eth_rx_ctl                (i_liteeth_rgmii_eth_rx_ctl)
    , .rgmii_eth_rx_data               (i_liteeth_rgmii_eth_rx_data)
    , .rgmii_eth_tx_ctl                (o_liteeth_rgmii_eth_tx_ctl)
    , .rgmii_eth_tx_data               (o_liteeth_rgmii_eth_tx_data)
    // 'our' UDP inputs and outputs
    , .udp0_udp_port                   (16'd6000)
    , .udp0_ip_address                 (32'hc0_a8_01_64)
    , .udp0_sink_valid                 (liteeth_udp_tx_valid)
    , .udp0_sink_last                  (liteeth_udp_tx_last)
    , .udp0_sink_ready                 (liteeth_udp_tx_ready)
    , .udp0_sink_data                  (liteeth_udp_tx_data)

    , .udp0_source_valid               (liteeth_udp_rx_valid)
    , .udp0_source_last                (liteeth_udp_rx_last)
    , .udp0_source_ready               (liteeth_udp_rx_ready)
    , .udp0_source_data                (liteeth_udp_rx_data)
    , .udp0_source_error               (liteeth_udp_rx_error)
    );

//always @(posedge clk_100MHz) begin
//  counter <= counter + 1;
//  if (counter == 0) begin
//    LED_1_state <= ~LED_1_state;
//    data_valid <= 1;
//    data_last <= 0;
//    data_to_send <= all_data[31:0];
//  end else if (counter == 1) begin
//    data_valid <= 1;
//    data_last <= 0;
//    data_to_send <= all_data[63:32];
//  end else if (counter == 2) begin
//    data_valid <= 1;
//    data_last <= 0;
//    data_to_send <= all_data[95:64];
//  end else if (counter == 3) begin
//    data_valid <= 1;
//    data_last <= 0;
//    data_to_send <= all_data[127:96];
//  end else if (counter == 4) begin
//    data_valid <= 1;
//    data_last <= 0;
//    data_to_send <= all_data[159:128];
//  end else if (counter == 5) begin
//    data_valid <= 1;
//    data_last <= 1;
//    data_to_send <= all_data[191:160];
//  end else if (counter == 6) begin
//    data_valid <= 0;
//    data_last <= 0;
//  end
//end

// Clock divider for PDM 3 MHz
divider #(.M(4)) PDM1 ( .clk_in(clk_12MHz), .clk_out(clk_3MHz));

// CIC filters accepting 1-bit PDM input and 18-bit filtered and decimated
// output
CICfilter #(.width(64)) CIC1 ( .clk_in(clk_3MHz), .d_in(mic_data_1), .d_out(CIC_to_correlator_1), .load(load_data_1));
CICfilter #(.width(64)) CIC2 ( .clk_in(clk_3MHz), .d_in(mic_data_2), .d_out(CIC_to_correlator_2), .load(load_data_2));
CICfilter #(.width(64)) CIC3 ( .clk_in(clk_3MHz), .d_in(mic_data_3), .d_out(CIC_to_correlator_3), .load(load_data_3));
CICfilter #(.width(64)) CIC4 ( .clk_in(clk_3MHz), .d_in(mic_data_4), .d_out(CIC_to_correlator_4), .load(load_data_4));
CICfilter #(.width(64)) CIC5 ( .clk_in(clk_3MHz), .d_in(mic_data_5), .d_out(CIC_to_correlator_5), .load(load_data_5));
CICfilter #(.width(64)) CIC6 ( .clk_in(clk_3MHz), .d_in(mic_data_6), .d_out(CIC_to_correlator_6), .load(load_data_6));

endmodule

// I still need to take a critical look at the required width parameter here.
// We have 64 bits, but this is more than needed.
module CICfilter #(parameter width = 64)
		       (input wire               clk_in,
			input wire               d_in,
			output reg signed [17:0] d_out,
		        output wire              load);

parameter decimation_ratio = 64; // From 3 MHz to 46875 Hz

reg signed [width-1:0] d_tmp   = {width{1'b0}};
reg signed [width-1:0] d_d_tmp = {width{1'b0}};

// Integrator stage registers

reg signed [width-1:0] d_hold = {width{1'b0}};
reg signed [width-1:0] d1 = {width{1'b0}};
reg signed [width-1:0] d2 = {width{1'b0}};
reg signed [width-1:0] d3 = {width{1'b0}};
reg signed [width-1:0] d4 = {width{1'b0}};
reg signed [width-1:0] d5 = {width{1'b0}};

// Comb stage registers

reg signed [width-1:0] d6, d_d6 = {width{1'b0}};
reg signed [width-1:0] d7, d_d7 = {width{1'b0}};
reg signed [width-1:0] d8, d_d8 = {width{1'b0}};
reg signed [width-1:0] d9, d_d9 = {width{1'b0}};
reg signed [width-1:0] d10 =      {width{1'b0}};

reg [15:0] count = 16'b0;
reg v_comb = 0;  // Valid signal for comb section running at output rate
reg load_s = 1; // Internal register that controls the output wire of the same name

assign load = load_s;

function [17:0] trunc_64_to_18(input [63:0] val64);
  trunc_64_to_18 = {val64[63], val64[35:19]}; // Working with signed data!
endfunction

initial begin
  d_hold <= 0;
  d1 <= 0;
  d2 <= 0;
  d3 <= 0;
  d4 <= 0;
  d5 <= 0;
  d6 <= 0;
  d7 <= 0;
  d8 <= 0;
  d9 <= 0;
  d10 <= 0;
  d_tmp <= 0;
  d_d_tmp <= 0;
  d_d6 <= 0;
  d_d7 <= 0;
  d_d8 <= 0;
  d_d9 <= 0;
  load_s <= 1'b1;
  d_out <= 18'b0;
end

always @(posedge clk_in)
begin
  // Integrator section
  if (d_in == 0) begin
    d_hold <= -1;
  end else begin
    d_hold <= 1;
  end
  d1 <= d1 + d_hold;
  d2 <= d1 + d2;
  d3 <= d2 + d3;
  d4 <= d3 + d4;
  d5 <= d4 + d5;
  
  // Decimation
  
  if (count == decimation_ratio - 1)
  begin
    count <= 0;
    d_tmp <= d5;
    v_comb <= 1;
  end else
  begin
    count <= count + 1;
    v_comb <= 0;
  end
end

always @(posedge clk_in)  // Comb section running at output rate
begin
  begin
    if (v_comb) begin
      // Comb section
      d_d_tmp <= d_tmp;
      d6 <= d_tmp - d_d_tmp;
      d_d6 <= d6;
      d7 <= d6 - d_d6;
      d_d7 <= d7;
      d8 <= d7 - d_d7;
      d_d8 <= d8;
      d9 <= d8 - d_d8;
      d_d9 <= d9;
      d10 <= d9 - d_d9;
      load_s <= 0; // Briefly set the data trigger to 0, to pass output to the correlator

      //d_out <= {d10[63], d10[30:14]}; // Used for sim testbench, where
      //signal takes large values
      //
      d_out <= trunc_64_to_18(d10); // Used for real microphones, where signal is weaker
      //d_out <= 18'b000000000000000001; // Used for real microphones, where signal is weaker
    end else begin
      load_s <= 1;
    end
  end
end

endmodule

// The sample manager takes care of putting audio samples into block ram when
// they become available, and of reading out sample values from memory in
// two rapid sequences in between.
module samplemanager #(parameter numlags = 256)
                     (input wire signed [17:0] d_in,
                      input wire clk_in, 
	              input wire sample_ready,
	              output wire signed [17:0] d_out_1,
	              output wire signed [17:0] d_out_2);

// These registers are fully internally administered
reg [$clog2(numlags)-2:0] address_a;
reg [$clog2(numlags)-2:0] address_b;
reg [11:0] state; // Goes up to 4095, should be plenty
reg [7:0] meta_counter;
reg [17:0] write_data;

// Flags that tell us if we can read or write, also internally administered
reg writeenable_a;
reg writeenable_b;

initial begin
  state = 0;
  meta_counter = 0;
  address_a = 0;
  address_b = 0;
  writeenable_a = 0;
  writeenable_b = 0;
  write_data <= 18'b0;
end

SAMPLE_BRAM #(.numlags(numlags)) blockram (.a_clk(clk_in), .a_wr(writeenable_a), .a_addr(address_a), .a_din(d_in), .a_dout(d_out_1), .b_clk(clk_in), .b_wr(writeenable_b), .b_addr(address_b), .b_din(d_in), .b_dout(d_out_2));

always @(posedge clk_in) begin
  if (sample_ready == 0 && state == 0) begin
    // Store the new sample in the current memory address. The sample data is
    // already loaded through d_in.
    writeenable_a <= 1;
    meta_counter <= meta_counter + 1;
    address_a <= meta_counter + 1;
    address_b <= meta_counter + 2;
    //write_data <= meta_counter;
    // Increment our counter so we don't trigger a write twice in a row
    state <= 1;
  end else if (state > 0 && state < numlags) begin // Careful: we only shift addresses every other step!
    // Here, we need to loop over all addresses in opposite orders, repeat
    // each address twice and have them shifted by one step.
    state <= state + 1;
    writeenable_a <= 0;
    if (state[0] == 0) begin
      // Update our incrementing address by one each even step
      address_a <= address_a - 1;
    end else begin
      // Update our decrementing address by one each odd step
      address_b <= address_b + 1;
    end
  end else begin
    state <= 0;
    //write_data <= write_data + 1;
  end
end

endmodule // samplemanager

module lagmanager #(parameter numlags = 256)
                  (input wire signed [17:0] d_in_1,
                   input wire signed [17:0] d_in_2,
                   input wire clk_in,
                   input wire sample_ready,
                   output wire signed [31:0] d_out,
                   output reg export_active);

 // These registers are fully internally administered
reg [$clog2(numlags)-1:0] address_read;
reg [$clog2(numlags)-1:0] address_write;
reg [11:0] state; // Goes up to 4095, should be plenty
reg [17:0] meta_counter;
//reg signed [31:0] read_data;
reg signed [31:0] write_buffer;
reg signed [31:0] write_data;
reg write_enable;
//reg export_active;
reg signed [17:0] d_1_buf;
reg signed [17:0] d_2_buf;

wire signed [31:0] read_data;

assign d_out = read_data;

initial begin
  // Set all params to their initial values here
  state = 0;
  write_enable = 0;
  address_read = 0;
  address_write = 0;
  meta_counter = 0;
  write_data = 0;
  export_active = 0;
end

LAG_BRAM #(.numlags(numlags)) lagmem ( .a_clk(clk_in), .a_addr(address_read), .a_dout(read_data), .b_clk(clk_in), .b_wr(write_enable), .b_addr(address_write), .b_din(write_data));

//assign export_active = sample_ready;

// States for the lagmanager:
// - 0: waiting for the next sample. Do not do anything.
// - 0 < state < 512: receiving sample values from the sample managers in rapid
// succession.
// - 512 < state < 1024 and meta_counter == 3125: outputting cumulative values

always @(posedge clk_in) begin
  if (sample_ready == 0 && state == 0) begin
    // The new sample data will start to come out at the next clock cycle! So,
    // we now need to fetch the corresponding lag accumulator value from
    // memory as we will be adding to it before storing it again.
    // The lag order is always the same, so we always start at the same memory
    // address!
    address_read <= 1;
    address_write <= 0;
    state <= state + 1;
  end else if (state > 0 && state < numlags - 1) begin
    // While the sample data keeps coming in, we keep calculating the products
    // and adding them to the current accumulation product.
    //d_1_buf <= d_in_1;
    //d_2_buf <= d_in_2;
    if (meta_counter == 0) begin
      // Restart the accumulation for all lag bins if this is the first sample
      // in an integration period
      //write_data <= d_1_buf * d_2_buf;
      write_data <= d_in_1 * d_in_2;
      //write_data <= 1;
    end else begin
      // Use the previous accumulated value to add to during integration for
      // all further samples
      //write_data <= read_data + d_1_buf * d_2_buf;
      write_data <= read_data + d_in_1 * d_in_2;
      //write_data <= 1;
    end
    address_read <= address_read + 1;
    if (state > 1) address_write <= address_write + 1;
    write_enable <= 1;
    state <= state + 1;
  end else if (state == numlags - 1) begin
    // We should stop writing to the lag buffer now!
    write_enable <= 0;
    // Reset the read address to be ready for sample copying if needed
    address_read <= 0;
    state <= state + 1;
    // Time to start copying lags to transmission buffer if we have completed
    // an integration period
    if (meta_counter == 3125) begin
      //export_active <= ~export_active; // test
      export_active <= 1;
    end
  end else if (state > numlags - 1 && state < 2 * numlags && meta_counter == 3125) begin
    // Go through our addresses for outputting
    address_read <= address_read + 1;
    state <= state + 1;
  end else if (state == 2 * numlags && meta_counter == 3125) begin
    meta_counter <= 0;
    export_active <= 0;
    state <= 0;
  end else if (state == 2 * numlags) begin
    meta_counter <= meta_counter + 1;
    state <= 0;
  end else if (state > 0 && state < 2 * numlags) begin
    state <= state + 1;
  end
end

endmodule

// TODO: Include a UDP transfer block in this transfermanager, working in
// parallel to the serial output. It will have to run at a different clock
// speed though, so I may only be able to have one of them active at a time as
// they read from the same block RAMs.
// Alternative: clone this module and adapt it to UDP transmission.
// Strategy:
// - I need to be able to offer 4 bytes at a time to the UDP core in its
// current configuration. I currently retrieve 32 bits at a time from BRAM, so
// that works out nicely. I keep reading 32 bits at a time and feeding these
// to the UDP core for a single baseline, then close the packet and have it
// transmitted. I will likely need to implement some kind of pause in between
// packets as I will otherwise choke the connection (time needs to be
// available for other frame components to be added in between UDP payloads).

module transfermanager #(parameter numlags = 256)
                       (input wire signed [31:0] d_in_1,
                        input wire signed [31:0] d_in_2,
                        input wire signed [31:0] d_in_3,
                        input wire signed [31:0] d_in_4,
                        input wire signed [31:0] d_in_5,
                        input wire signed [31:0] d_in_6,
                        input wire signed [31:0] d_in_7,
                        input wire signed [31:0] d_in_8,
                        input wire signed [31:0] d_in_9,
                        input wire signed [31:0] d_in_10,
                        input wire signed [31:0] d_in_11,
                        input wire signed [31:0] d_in_12,
                        input wire signed [31:0] d_in_13,
                        input wire signed [31:0] d_in_14,
                        input wire signed [31:0] d_in_15,
                        input wire clk_in,
                        input wire clk_in_2,
                        input wire export_active,
                        output wire d_out,
			output eth_data_valid,
			output eth_data_last,
			output [31:0] eth_data
		);

// These registers are fully internally administered
reg [$clog2(numlags)-1:0] address_read;

reg [$clog2(numlags)-1:0] address_write_1;
reg [$clog2(numlags)-1:0] address_write_2;
reg [$clog2(numlags)-1:0] address_write_3;
reg [$clog2(numlags)-1:0] address_write_4;
reg [$clog2(numlags)-1:0] address_write_5;
reg [$clog2(numlags)-1:0] address_write_6;
reg [$clog2(numlags)-1:0] address_write_7;
reg [$clog2(numlags)-1:0] address_write_8;
reg [$clog2(numlags)-1:0] address_write_9;
reg [$clog2(numlags)-1:0] address_write_10;
reg [$clog2(numlags)-1:0] address_write_11;
reg [$clog2(numlags)-1:0] address_write_12;
reg [$clog2(numlags)-1:0] address_write_13;
reg [$clog2(numlags)-1:0] address_write_14;
reg [$clog2(numlags)-1:0] address_write_15;

reg [11:0] state; // Goes up to 4095, should be plenty

reg signed [31:0] write_data_1;
reg signed [31:0] write_data_2;
reg signed [31:0] write_data_3;
reg signed [31:0] write_data_4;
reg signed [31:0] write_data_5;
reg signed [31:0] write_data_6;
reg signed [31:0] write_data_7;
reg signed [31:0] write_data_8;
reg signed [31:0] write_data_9;
reg signed [31:0] write_data_10;
reg signed [31:0] write_data_11;
reg signed [31:0] write_data_12;
reg signed [31:0] write_data_13;
reg signed [31:0] write_data_14;
reg signed [31:0] write_data_15;

reg signed [31:0] read_data_1;
reg signed [31:0] read_data_2;
reg signed [31:0] read_data_3;
reg signed [31:0] read_data_4;
reg signed [31:0] read_data_5;
reg signed [31:0] read_data_6;
reg signed [31:0] read_data_7;
reg signed [31:0] read_data_8;
reg signed [31:0] read_data_9;
reg signed [31:0] read_data_10;
reg signed [31:0] read_data_11;
reg signed [31:0] read_data_12;
reg signed [31:0] read_data_13;
reg signed [31:0] read_data_14;
reg signed [31:0] read_data_15;

reg write_enable_1;
reg write_enable_2;
reg write_enable_3;
reg write_enable_4;
reg write_enable_5;
reg write_enable_6;
reg write_enable_7;
reg write_enable_8;
reg write_enable_9;
reg write_enable_10;
reg write_enable_11;
reg write_enable_12;
reg write_enable_13;
reg write_enable_14;
reg write_enable_15;

reg transfer_state;
reg transfer_complete;
reg [$clog2(numlags) + 4:0] lag_idx;
reg [4:0] baseline;
reg [31:0] test_pattern_1;
reg [31:0] test_pattern_2;
reg [31:0] test_pattern_3;
reg [31:0] test_pattern_4;
reg [31:0] test_pattern_5;
reg [31:0] test_pattern_6;
reg [31:0] test_pattern_7;
reg [31:0] test_pattern_8;
reg [31:0] test_pattern_9;
reg [31:0] test_pattern_10;
reg [31:0] test_pattern_11;
reg [31:0] test_pattern_12;
reg [31:0] test_pattern_13;
reg [31:0] test_pattern_14;
reg [31:0] test_pattern_15;

reg[7:0] example_byte;

initial begin
  // Set all params to their initial values here
  state = 0;
  transfer_state = 0;
  transfer_complete = 0;

  write_enable_1 = 0;
  write_enable_2 = 0;
  write_enable_3 = 0;
  write_enable_4 = 0;
  write_enable_5 = 0;
  write_enable_6 = 0;
  write_enable_7 = 0;
  write_enable_8 = 0;
  write_enable_9 = 0;
  write_enable_10 = 0;
  write_enable_11 = 0;
  write_enable_12 = 0;
  write_enable_13 = 0;
  write_enable_14 = 0;
  write_enable_15 = 0;

  address_read = 0;

  address_write_1 = 0;
  address_write_2 = 0;
  address_write_3 = 0;
  address_write_4 = 0;
  address_write_5 = 0;
  address_write_6 = 0;
  address_write_7 = 0;
  address_write_8 = 0;
  address_write_9 = 0;
  address_write_10 = 0;
  address_write_11 = 0;
  address_write_12 = 0;
  address_write_13 = 0;
  address_write_14 = 0;
  address_write_15 = 0;

  write_data_1 = 0;
  write_data_2 = 0;
  write_data_3 = 0;
  write_data_4 = 0;
  write_data_5 = 0;
  write_data_6 = 0;
  write_data_7 = 0;
  write_data_8 = 0;
  write_data_9 = 0;
  write_data_10 = 0;
  write_data_11 = 0;
  write_data_12 = 0;
  write_data_13 = 0;
  write_data_14 = 0;
  write_data_15 = 0;

  lag_idx = 0;
  baseline = 0;
  test_pattern_1  = 32'b01000001_01000010_01000011_01000100; // ABCD in ASCII                    
  test_pattern_2  = 32'b01000101_01000110_01000111_01001000; // EFGH in ASCII
  test_pattern_3  = 32'b01001001_01001010_01001011_01001100; // IJKL in ASCII
  test_pattern_4  = 32'b01001101_01001110_01001111_01010000; // MNOP in ASCII
  test_pattern_5  = 32'b01010001_01010010_01010011_01010100; // QRST in ASCII
  test_pattern_6  = 32'b01010101_01010110_01010111_01011000; // UVWX in ASCII
  test_pattern_7  = 32'b00111001_00111001_00111001_00111001; // 9999 in ASCII
  test_pattern_8  = 32'b00110001_00110001_00110001_00110001; // 1111 in ASCII
  test_pattern_9  = 32'b00110010_00110010_00110010_00110010; // 2222 in ASCII
  test_pattern_10 = 32'b00110011_00110011_00110011_00110011; // 3333 in ASCII
  test_pattern_11 = 32'b00110100_00110100_00110100_00110100; // 4444 in ASCII
  test_pattern_12 = 32'b00110101_00110101_00110101_00110101; // 5555 in ASCII
  test_pattern_13 = 32'b00110110_00110110_00110110_00110110; // 6666 in ASCII
  test_pattern_14 = 32'b00110111_00110111_00110111_00110111; // 7777 in ASCII
  test_pattern_15 = 32'b00111000_00111000_00111000_00111000; // 8888 in ASCII
end

TRANSFER_BRAM #(.numlags(numlags)) lagmem_1  ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_1), .b_clk(clk_in), .b_wr(write_enable_1), .b_addr(address_write_1), .b_din(write_data_1));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_2  ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_2), .b_clk(clk_in), .b_wr(write_enable_2), .b_addr(address_write_2), .b_din(write_data_2));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_3  ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_3), .b_clk(clk_in), .b_wr(write_enable_3), .b_addr(address_write_3), .b_din(write_data_3));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_4  ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_4), .b_clk(clk_in), .b_wr(write_enable_4), .b_addr(address_write_4), .b_din(write_data_4));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_5  ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_5), .b_clk(clk_in), .b_wr(write_enable_5), .b_addr(address_write_5), .b_din(write_data_5));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_6  ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_6), .b_clk(clk_in), .b_wr(write_enable_6), .b_addr(address_write_6), .b_din(write_data_6));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_7  ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_7), .b_clk(clk_in), .b_wr(write_enable_7), .b_addr(address_write_7), .b_din(write_data_7));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_8  ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_8), .b_clk(clk_in), .b_wr(write_enable_8), .b_addr(address_write_8), .b_din(write_data_8));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_9  ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_9), .b_clk(clk_in), .b_wr(write_enable_9), .b_addr(address_write_9), .b_din(write_data_9));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_10 ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_10), .b_clk(clk_in), .b_wr(write_enable_10), .b_addr(address_write_10), .b_din(write_data_10));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_11 ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_11), .b_clk(clk_in), .b_wr(write_enable_11), .b_addr(address_write_11), .b_din(write_data_11));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_12 ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_12), .b_clk(clk_in), .b_wr(write_enable_12), .b_addr(address_write_12), .b_din(write_data_12));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_13 ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_13), .b_clk(clk_in), .b_wr(write_enable_13), .b_addr(address_write_13), .b_din(write_data_13));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_14 ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_14), .b_clk(clk_in), .b_wr(write_enable_14), .b_addr(address_write_14), .b_din(write_data_14));
TRANSFER_BRAM #(.numlags(numlags)) lagmem_15 ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data_15), .b_clk(clk_in), .b_wr(write_enable_15), .b_addr(address_write_15), .b_din(write_data_15));

// What does the transfer manager do?
// - Sit around waiting for the export_active flag to become active
// - Once it is, eat up all the data that is being fed to it, 512 values in
// a row
// - Once this is complete, switch to transmission mode and start outputting
// packets using the data clock.
// - Once finished, go back to sleep and wait for the next export_active flag.

always @(posedge clk_in) begin
  if (export_active == 1 && state == 0 && transfer_state == 0) begin
    // We start receiving lags! Time to store them.
    write_data_1 <= d_in_1;
    write_data_2 <= d_in_2;
    write_data_3 <= d_in_3;
    write_data_4 <= d_in_4;
    write_data_5 <= d_in_5;
    write_data_6 <= d_in_6;
    write_data_7 <= d_in_7;
    write_data_8 <= d_in_8;
    write_data_9 <= d_in_9;
    write_data_10 <= d_in_10;
    write_data_11 <= d_in_11;
    write_data_12 <= d_in_12;
    write_data_13 <= d_in_13;
    write_data_14 <= d_in_14;
    write_data_15 <= d_in_15;

    write_enable_1 <= 1;
    write_enable_2 <= 1;
    write_enable_3 <= 1;
    write_enable_4 <= 1;
    write_enable_5 <= 1;
    write_enable_6 <= 1;
    write_enable_7 <= 1;
    write_enable_8 <= 1;
    write_enable_9 <= 1;
    write_enable_10 <= 1;
    write_enable_11 <= 1;
    write_enable_12 <= 1;
    write_enable_13 <= 1;
    write_enable_14 <= 1;
    write_enable_15 <= 1;

    address_write_1 <= 0;
    address_write_2 <= 0;
    address_write_3 <= 0;
    address_write_4 <= 0;
    address_write_5 <= 0;
    address_write_6 <= 0;
    address_write_7 <= 0;
    address_write_8 <= 0;
    address_write_9 <= 0;
    address_write_10 <= 0;
    address_write_11 <= 0;
    address_write_12 <= 0;
    address_write_13 <= 0;
    address_write_14 <= 0;
    address_write_15 <= 0;

    state <= state + 1;
  end else if (state > 0 && state < numlags && transfer_state == 0) begin
    // Keep accepting data and storing it
    write_data_1 <= d_in_1;
    write_data_2 <= d_in_2;
    write_data_3 <= d_in_3;
    write_data_4 <= d_in_4;
    write_data_5 <= d_in_5;
    write_data_6 <= d_in_6;
    write_data_7 <= d_in_7;
    write_data_8 <= d_in_8;
    write_data_9 <= d_in_9;
    write_data_10 <= d_in_10;
    write_data_11 <= d_in_11;
    write_data_12 <= d_in_12;
    write_data_13 <= d_in_13;
    write_data_14 <= d_in_14;
    write_data_15 <= d_in_15;

    address_write_1 <= address_write_1 + 1;
    address_write_2 <= address_write_2 + 1;
    address_write_3 <= address_write_3 + 1;
    address_write_4 <= address_write_4 + 1;
    address_write_5 <= address_write_5 + 1;
    address_write_6 <= address_write_6 + 1;
    address_write_7 <= address_write_7 + 1;
    address_write_8 <= address_write_8 + 1;
    address_write_9 <= address_write_9 + 1;
    address_write_10 <= address_write_10 + 1;
    address_write_11 <= address_write_11 + 1;
    address_write_12 <= address_write_12 + 1;
    address_write_13 <= address_write_13 + 1;
    address_write_14 <= address_write_14 + 1;
    address_write_15 <= address_write_15 + 1;

    state <= state + 1;
  end else if (state == numlags && transfer_state == 0) begin
    // We have all the data! Now to start sending it.
    write_enable_1 <= 0;
    write_enable_2 <= 0;
    write_enable_3 <= 0;
    write_enable_4 <= 0;
    write_enable_5 <= 0;
    write_enable_6 <= 0;
    write_enable_7 <= 0;
    write_enable_8 <= 0;
    write_enable_9 <= 0;
    write_enable_10 <= 0;
    write_enable_11 <= 0;
    write_enable_12 <= 0;
    write_enable_13 <= 0;
    write_enable_14 <= 0;
    write_enable_15 <= 0;

    transfer_state <= 1;
    state <= 0;
  end else if (transfer_complete == 1) begin
    transfer_state <= 0;
  end
end

// This is the section that I will update for ethernet.
// We need to interface with the UDP core, so first thing is to add an
// instantiation of that module here.

reg [31:0] data_to_send = 32'h00000000;
reg data_valid = 0;
reg data_last = 0;

assign eth_data_valid = data_valid;
assign eth_data_last = data_last;
assign eth_data = data_to_send;

// We will use the tx_valid, tx_last and tx_data connections to send data.
// Try for a single baseline first! Sends numlags * 4 bytes.
// We currently use 128 lags, so that gives 512 bytes. The FIFO buffer size
// should therefore be 512 - 4 = 508 bytes, which is 127 words. However, the
// FCS is not calculated correctly when I use that value. Using 128 words the
// full payload is succesfully delivered but with extra bytes tacked on for
// some reason.

always @ (posedge clk_in_2) begin
  if (transfer_state == 1 && transfer_complete == 0) begin
    if (lag_idx == 0) begin
      // Send dummy byte sequence
      if (baseline == 0) begin
        data_to_send <= test_pattern_1;
      end else if (baseline == 1) begin
        data_to_send <= test_pattern_2;
      end else if (baseline == 2) begin
        data_to_send <= test_pattern_3;
      end else if (baseline == 3) begin
        data_to_send <= test_pattern_4;
      end else if (baseline == 4) begin
        data_to_send <= test_pattern_5;
      end else if (baseline == 5) begin
        data_to_send <= test_pattern_6;
      end else if (baseline == 6) begin
        data_to_send <= test_pattern_7;
      end else if (baseline == 7) begin
        data_to_send <= test_pattern_8;
      end else if (baseline == 8) begin
        data_to_send <= test_pattern_9;
      end else if (baseline == 9) begin
        data_to_send <= test_pattern_10;
      end else if (baseline == 10) begin
        data_to_send <= test_pattern_11;
      end else if (baseline == 11) begin
        data_to_send <= test_pattern_12;
      end else if (baseline == 12) begin
        data_to_send <= test_pattern_13;
      end else if (baseline == 13) begin
        data_to_send <= test_pattern_14;
      end else if (baseline == 14) begin
        data_to_send <= test_pattern_15;
      end
      data_valid <= 1;
      data_last <= 0;
      address_read <= 0; // Prepare address for lag values next cycle
      lag_idx <= lag_idx + 1;
    end else if (lag_idx > 0 && lag_idx < (numlags - 1)) begin
      // Regular transmission
      data_valid <= 1;
      data_last <= 0;
      if (baseline == 0) begin
        data_to_send <= read_data_1;
      end else if (baseline == 1) begin
        data_to_send <= read_data_2;
      end else if (baseline == 2) begin
        data_to_send <= read_data_3;
      end else if (baseline == 3) begin
        data_to_send <= read_data_4;
      end else if (baseline == 4) begin
        data_to_send <= read_data_5;
      end else if (baseline == 5) begin
        data_to_send <= read_data_6;
      end else if (baseline == 6) begin
        data_to_send <= read_data_7;
      end else if (baseline == 7) begin
        data_to_send <= read_data_8;
      end else if (baseline == 8) begin
        data_to_send <= read_data_9;
      end else if (baseline == 9) begin
        data_to_send <= read_data_10;
      end else if (baseline == 10) begin
        data_to_send <= read_data_11;
      end else if (baseline == 11) begin
        data_to_send <= read_data_12;
      end else if (baseline == 12) begin
        data_to_send <= read_data_13;
      end else if (baseline == 13) begin
        data_to_send <= read_data_14;
      end else if (baseline == 14) begin
        data_to_send <= read_data_15;
      end
      address_read <= address_read + 1;
      lag_idx <= lag_idx + 1;
    end else if (lag_idx == (numlags - 1)) begin
      // end transmission, de-activate data input for PHY
      data_valid <= 1;
      data_last <= 1;
      if (baseline == 0) begin
        data_to_send <= read_data_1;
      end else if (baseline == 1) begin
        data_to_send <= read_data_2;
      end else if (baseline == 2) begin
        data_to_send <= read_data_3;
      end else if (baseline == 3) begin
        data_to_send <= read_data_4;
      end else if (baseline == 4) begin
        data_to_send <= read_data_5;
      end else if (baseline == 5) begin
        data_to_send <= read_data_6;
      end else if (baseline == 6) begin
        data_to_send <= read_data_7;
      end else if (baseline == 7) begin
        data_to_send <= read_data_8;
      end else if (baseline == 8) begin
        data_to_send <= read_data_9;
      end else if (baseline == 9) begin
        data_to_send <= read_data_10;
      end else if (baseline == 10) begin
        data_to_send <= read_data_11;
      end else if (baseline == 11) begin
        data_to_send <= read_data_12;
      end else if (baseline == 12) begin
        data_to_send <= read_data_13;
      end else if (baseline == 13) begin
        data_to_send <= read_data_14;
      end else if (baseline == 14) begin
        data_to_send <= read_data_15;
      end
      lag_idx <= lag_idx + 1;
      address_read <= 0;
    end else if (lag_idx == numlags) begin
      // Dummy word needed to get LiteEth to send the packet
      data_valid <= 1;
      data_last <= 0;
      lag_idx <= lag_idx + 1;
      data_to_send <= 32'hFFFFFFFF;
    end else if (lag_idx < (10 * numlags)) begin
      // Stop data for this packet
      data_valid <= 0;
      data_last <= 0;
      lag_idx <= lag_idx + 1;
    end else if (lag_idx == (10 * numlags)) begin
      lag_idx <= 0;
      if (baseline < 14) begin
	// Go to next baseline
        baseline <= baseline + 1;
      end else begin
        // End transmission after having completed the last packet sent
        transfer_complete <= 1;
        baseline <= 0;
      end
    end
  end else if (transfer_complete == 1) begin
    transfer_complete <= 0;
  end
end

endmodule

// Divider module
module divider #(parameter M = 104)
                (input wire clk_in, output wire clk_out);

localparam N = $clog2(M);
reg [N-1:0] divcounter;

initial begin
  divcounter <= 0;
end

always @(posedge clk_in)
  divcounter <= (divcounter == M - 1) ? 0 : divcounter + 1;

assign clk_out = divcounter[N-1];

endmodule

// PLLs deactivated for simulation in iverilog:
// their signals are passed as clock signals in the (adapted) top module for
// now.

// --- Change when using sim ---



module pll_100_12
(
    input clk_100, // 100 MHz, 0 deg
    output clk_12, // 12 MHz, 0 deg
    output locked
);
(* FREQUENCY_PIN_CLKI="100" *)
(* FREQUENCY_PIN_CLKOP="12" *)
(* ICP_CURRENT="12" *) (* LPF_RESISTOR="8" *) (* MFG_ENABLE_FILTEROPAMP="1" *) (* MFG_GMCREF_SEL="2" *)
EHXPLLL #(
        .PLLRST_ENA("DISABLED"),
        .INTFB_WAKE("DISABLED"),
        .STDBY_ENABLE("DISABLED"),
        .DPHASE_SOURCE("DISABLED"),
        .OUTDIVIDER_MUXA("DIVA"),
        .OUTDIVIDER_MUXB("DIVB"),
        .OUTDIVIDER_MUXC("DIVC"),
        .OUTDIVIDER_MUXD("DIVD"),
        .CLKI_DIV(25),
        .CLKOP_ENABLE("ENABLED"),
        .CLKOP_DIV(50),
        .CLKOP_CPHASE(24),
        .CLKOP_FPHASE(0),
        .FEEDBK_PATH("CLKOP"),
        .CLKFB_DIV(3)
    ) pll_i (
        .RST(1'b0),
        .STDBY(1'b0),
        .CLKI(clk_100),
        .CLKOP(clk_12),
        .CLKFB(clk_12),
        .CLKINTFB(),
        .PHASESEL0(1'b0),
        .PHASESEL1(1'b0),
        .PHASEDIR(1'b1),
        .PHASESTEP(1'b1),
        .PHASELOADREG(1'b1),
        .PLLWAKESYNC(1'b0),
        .ENCLKOP(1'b0),
        .LOCK(locked)
	);
endmodule

module SAMPLE_BRAM #(parameter numlags = 256)
   (
    // Port A
    input   wire            a_clk,
    input   wire            a_wr,
    input   wire    [$clog2(numlags)-2:0]   a_addr,
    input   wire    [17:0]  a_din,
    output  reg     [17:0]  a_dout,

    // Port B
    input   wire            b_clk,
    input   wire            b_wr,
    input   wire    [$clog2(numlags)-2:0]   b_addr,
    input   wire    [17:0]  b_din,
    output  reg     [17:0]  b_dout
);

// Shared memory
reg [17:0] mem [numlags/2-1:0];

integer i;

initial begin
  for (i=0; i < numlags/2; i = i + 1) begin
    mem[i] <= 18'b0;
  end
end

// Port A
always @(posedge a_clk) begin
    if(a_wr) begin
        //a_dout      <= a_din;
        mem[a_addr] <= a_din;
    end else begin
      a_dout <= mem[a_addr];
    end
end

// Port B
always @(posedge b_clk) begin
    if(b_wr) begin
        //b_dout      <= b_din;
        mem[b_addr] <= b_din;
    end else begin
      b_dout <= mem[b_addr];
    end
end

endmodule // SAMPLE_BRAM

module LAG_BRAM #(parameter numlags = 256)
    (
    // Port A, the read port
    input   wire            a_clk,
    input   wire    [$clog2(numlags)-1:0]   a_addr,
    output  reg     [31:0]  a_dout,

    // Port B, the write port
    input   wire            b_clk,
    input   wire            b_wr,
    input   wire    [$clog2(numlags)-1:0]   b_addr,
    input   wire    [31:0]  b_din
);

// Shared memory
reg [31:0] mem [numlags-1:0];

integer i;

initial begin
  for (i=0; i < numlags; i = i + 1) begin
    mem[i] <= 32'b0;
  end
end

// Port A
always @(posedge a_clk) begin
  a_dout <= mem[a_addr];
end

// Port B
always @(posedge b_clk) begin
  if(b_wr) begin
    mem[b_addr] <= b_din;
  end
end

endmodule // LAG_BRAM

module TRANSFER_BRAM #(parameter numlags = 256)
    (
    // Port A, the read port
    input   wire            a_clk,
    input   wire    [$clog2(numlags)-1:0]   a_addr,
    output  reg     [31:0]  a_dout,

    // Port B, the write port
    input   wire            b_clk,
    input   wire            b_wr,
    input   wire    [$clog2(numlags)-1:0]   b_addr,
    input   wire    [31:0]  b_din
);

// Shared memory
reg [31:0] mem [numlags-1:0];

integer i;

initial begin
  for (i=0; i < numlags; i = i + 1) begin
    mem[i] <= 32'b01010101011011000011001101100110;
  end
end

// Port A
always @(posedge a_clk) begin
  a_dout <= mem[a_addr];
end

// Port B
always @(posedge b_clk) begin
  if(b_wr) begin
    mem[b_addr] <= b_din;
  end
end

endmodule // TRANSFER_BRAM
