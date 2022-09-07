// Lag correlator, which performs the following functions:
// - Filtering of PDM microphone output from 3 MHz to 46875 Hz
// - Storing bit-range selected audio samples in BRAM for each mic
// - Feeding audio samples from each mic BRAM into baseline-based lag
// correlators
// - After integration, feeding the correlation products into a transfer
// manager for transmission over ethernet.

// NOTE: Fix the mic clk in wiring! We only have one clk pin now, so we will
// have to split it over all mics in wiring.

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
	    input wire mic_data_7,
	    input wire mic_data_8,
            output wire uart_tx,
	    output wire mic_clk_1,
	    output wire mic_clk_2,
	    output wire mic_clk_3,
	    output wire mic_clk_4,
	    output wire mic_clk_5,
	    output wire mic_clk_6,
	    output wire mic_clk_7,
	    output wire mic_clk_8,
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

genvar i;
genvar j;

//parameter NUMLAGS = 128;
//parameter NUMMICS = 8;
parameter NUMLAGS = 128;
parameter NUMMICS = 8;
parameter NUMBASELINES = (NUMMICS * (NUMMICS - 1)) / 2;

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

wire micdata_to_CIC [NUMMICS - 1 : 0];

// Assign our mic inputs to an array for easier instantiation of module arrays
assign micdata_to_CIC[7] = mic_data_8;
assign micdata_to_CIC[6] = mic_data_7;
assign micdata_to_CIC[5] = mic_data_6;
assign micdata_to_CIC[4] = mic_data_5;
assign micdata_to_CIC[3] = mic_data_4;
assign micdata_to_CIC[2] = mic_data_3;
assign micdata_to_CIC[1] = mic_data_2;
assign micdata_to_CIC[0] = mic_data_1;


wire [17:0] CIC_to_samplemanager [NUMMICS - 1 : 0];

wire [17:0] sample_out_up [NUMMICS - 1 : 0];
wire [17:0] sample_out_dn [NUMMICS - 1 : 0];

wire load_data;
wire locked;

wire data_out;

// --- Comment out if using sim ---
wire clk_12MHz;
wire clk_120MHz;

//-- PDM clock
wire clk_3MHz;

reg [27:0] long_counter = 0;

wire [31:0] lag_data_out [NUMBASELINES - 1 : 0];
wire [NUMBASELINES * 32 - 1 : 0] lag_data_all;

// Assign our lag outputs to parts of our 'big' data input bus, for feeding
// into our transfer manager

for (i = 0; i < NUMBASELINES; i++) begin
  assign lag_data_all[(i+1) * 32 - 1 : i * 32] = lag_data_out[i];
end

wire export_flag;
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

// Separate mic clk pins for now, try shared pin later
assign mic_clk_1 = clk_3MHz;
assign mic_clk_2 = clk_3MHz;
assign mic_clk_3 = clk_3MHz;
assign mic_clk_4 = clk_3MHz;
assign mic_clk_5 = clk_3MHz;
assign mic_clk_6 = clk_3MHz;
assign mic_clk_7 = clk_3MHz;
assign mic_clk_8 = clk_3MHz;

assign uart_tx = serial_data;

assign split_out_1 = mic_data_1;
assign split_out_2 = mic_data_2;
assign split_out_3 = mic_data_3;

// Change when using sim
pll_100_12 PLL_SLOW (.clk_100(clk_100MHz), .clk_12(clk_12MHz));
// Clock divider for PDM 3 MHz
divider #(.M(4)) PDM1 ( .clk_in(clk_12MHz), .clk_out(clk_3MHz));

// CIC filters, 1 per mic. Only the 1st mic gives the 'load_data' output signal.
for (i = 0; i < NUMMICS; i++) begin
  if (i == 0) begin
    CICfilter #(.width(64)) CIC ( .clk_in(clk_3MHz), .d_in(micdata_to_CIC[i]), .d_out(CIC_to_samplemanager[i]), .load(load_data));
  end else begin
    CICfilter #(.width(64)) CIC ( .clk_in(clk_3MHz), .d_in(micdata_to_CIC[i]), .d_out(CIC_to_samplemanager[i]), .load());
  end
end  

// Sample managers, 1 for each mic.
for (i = 0; i < NUMMICS; i++) begin
  samplemanager #(.numlags(NUMLAGS)) sample (.d_in(CIC_to_samplemanager[i]),
                                             .clk_in(clk_100MHz), 
                                             .sample_ready(load_data),
                                             .d_out_1(sample_out_up[i]),
                                             .d_out_2(sample_out_dn[i]));
end

// Lag managers, 1 per baseline. Only the first gives an export flag.

for (i = 0; i < (NUMMICS - 1); i++) begin
  for (j = (i + 1); j < NUMMICS; j++) begin
    // The following expression keeps correct numbering for all baselines as
    // a function of i and j.
    if (i == 0 && j == 1) begin
    lagmanager #(.numlags(NUMLAGS)) lags (.d_in_1(sample_out_up[i]),
                                          .d_in_2(sample_out_dn[j]),
                                          .clk_in(clk_100MHz),
                                          .sample_ready(load_data),
                                          .d_out(lag_data_out[(i * (2 * NUMMICS - 3 - i) + 2 * j - 2) / 2]),
                                          .export_active(export_flag));
    end else begin
    lagmanager #(.numlags(NUMLAGS)) lags (.d_in_1(sample_out_up[i]),
                                          .d_in_2(sample_out_dn[j]),
                                          .clk_in(clk_100MHz),
                                          .sample_ready(load_data),
                                          .d_out(lag_data_out[(i * (2 * NUMMICS - 3 - i) + 2 * j - 2) / 2]),
                                          .export_active());
    end
  end
end

// Transfer manager. Use a parametrized width data bus for input, scaling with
// number of mics.
transfermanager #(.numlags(NUMLAGS), .numbaselines(NUMBASELINES)) transfer (.d_in_all(lag_data_all),
                                                                            .clk_in(clk_100MHz),
                                                                            .clk_in_2(clk_100MHz),
                                                                            .export_active(export_flag),
                                                                            .d_out(serial_data),
				                                            .eth_data_valid(liteeth_udp_tx_valid),
				                                            .eth_data_last(liteeth_udp_tx_last),
				                                            .eth_data(liteeth_udp_tx_data));

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

module transfermanager #(parameter numlags = 256,
                         parameter numbaselines = 15)
                       (input wire signed [32 * numbaselines - 1: 0 ] d_in_all,
                        input wire clk_in,
                        input wire clk_in_2,
                        input wire export_active,
                        output wire d_out,
			output eth_data_valid,
			output eth_data_last,
			output [31:0] eth_data
		);
genvar i;
integer j;

// These registers are fully internally administered
reg [$clog2(numlags)-1:0] address_read;
reg [$clog2(numlags)-1:0] address_write;

reg [11:0] state; // Goes up to 4095, should be plenty

reg signed [31:0] write_data [numbaselines - 1 : 0];
reg signed [31:0] read_data [numbaselines - 1 : 0];

reg write_enable;

reg transfer_state;
reg transfer_complete;
reg [$clog2(numlags) + 4:0] lag_idx;
reg [7:0] baseline; // We use 8 bits for simplicity when transferring the dummy code

initial begin
  // Set all params to their initial values here
  state = 0;
  transfer_state = 0;
  transfer_complete = 0;

  write_enable = 0;
  address_read = 0;
  address_write = 0;

  for (j = 0; j < numbaselines; j++) begin
    write_data[j] = 0;
  end

  lag_idx = 0;
  baseline = 0;
end

generate
  for (i = 0; i < numbaselines; i++) begin
    TRANSFER_BRAM #(.numlags(numlags)) lagmem  ( .a_clk(clk_in_2), .a_addr(address_read), .a_dout(read_data[i]), .b_clk(clk_in), .b_wr(write_enable), .b_addr(address_write), .b_din(write_data[i]));
  end
endgenerate

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
    for (j = 0; j < numbaselines; j++) begin
      write_data[j] <= d_in_all[32 * (j+1) - 1 : 32 * j];
    end
    write_enable <= 1;
    address_write <= 0;
    state <= state + 1;
  end else if (state > 0 && state < numlags && transfer_state == 0) begin
    // Keep accepting data and storing it
    for (j = 0; j < numbaselines; j++) begin
      write_data[j] <= d_in_all[32 * (j+1) - 1 : 32 * j];
    end
    address_write <= address_write + 1;
    state <= state + 1;
  end else if (state == numlags && transfer_state == 0) begin
    // We have all the data! Now to start sending it.
    write_enable <= 0;
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
      data_to_send <= {4{baseline}};
      data_valid <= 1;
      data_last <= 0;
      address_read <= 0; // Prepare address for lag values next cycle
      lag_idx <= lag_idx + 1;
    end else if (lag_idx > 0 && lag_idx < (numlags - 1)) begin
      // Regular transmission
      data_valid <= 1;
      data_last <= 0;
      data_to_send <= read_data[baseline];
      address_read <= address_read + 1;
      lag_idx <= lag_idx + 1;
    end else if (lag_idx == (numlags - 1)) begin
      // end transmission, de-activate data input for PHY
      data_valid <= 1;
      data_last <= 1;
      data_to_send <= read_data[baseline];
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
      if (baseline < (numbaselines - 1)) begin
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

integer i;

// Shared memory
reg [17:0] mem [numlags/2-1:0];

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

integer i;

// Shared memory
reg [31:0] mem [numlags-1:0];

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

integer i;

// Shared memory
reg [31:0] mem [numlags-1:0];

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

endmodule // TRANSFER_BRAM
