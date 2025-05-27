module player2_src 
  #(
    parameter CD        = 12,         // color depth (RGB: 12 bits)
              ADDR      = 11,         // now 11 bits so we can index 2·1024 words
              KEY_COLOR= 12'h000     // transparent color
  )
  (
    input  logic            clk,
    input  logic  [10:0]    x, y,    // current pixel (hcount/vcount)
    input  logic  [10:0]    x0, y0,  // sprite origin
    input  logic            sel,     // 0 = normal, 1 = kicking
    // sprite RAM write interface
    input  logic            we,
    input  logic  [ADDR-1:0] addr_w,
    input  logic  [2:0]     pixel_in,
    // output
    output logic  [CD-1:0]  sprite_rgb
  );

  // constants
  localparam H_SIZE     = 32;
  localparam V_SIZE     = 32;
  localparam FRAME_SIZE = H_SIZE * V_SIZE;  // 1024


  // signals
  logic signed [11:0]    xr, yr;
  logic [ADDR-1:0]       addr_r;
  logic [2:0]            plt_code;
  logic                  in_region;
  logic [CD-1:0]         full_rgb, out_rgb, out_rgb_d1;

  // instantiate your existing RAM
  player2_ram_lut #(
    .ADDR_WIDTH(ADDR),
    .DATA_WIDTH(3)
  ) ram_unit (
    .clk    (clk),
    .we     (we),
    .addr_w (addr_w),
    .din    (pixel_in),
    .addr_r (addr_r),
    .dout   (plt_code)
  );

  // compute relative coordinates
  assign xr = $signed({1'b0, x})  - $signed({1'b0, x0});
  assign yr = $signed({1'b0, y})  - $signed({1'b0, y0});
  assign in_region = (xr >= 0 && xr < H_SIZE && yr >= 0 && yr < V_SIZE);

  // build the 10-bit base index within a single 32×32 frame
  // (yr << 5) + xr = yr*32 + xr
  logic [ADDR-2:0] base_idx = {yr[4:0], xr[4:0]};

  // prepend sel as the MSB to pick frame 0 or frame 1
  // addr_r = sel·1024 + base_idx
  assign addr_r = { sel, base_idx };

//palete decode
  always_comb begin
    case (plt_code)
      3'd0: full_rgb = KEY_COLOR;
      3'd1: full_rgb = 12'h100;
      3'd2: full_rgb = 12'hFFF;
      3'd3: full_rgb = 12'h543;
      3'd4: full_rgb = 12'h976;
      3'd5: full_rgb = 12'hFDA;
      3'd6: full_rgb = 12'h4AD;
      default: full_rgb = KEY_COLOR;
    endcase
  end
   // Output RGB
   assign out_rgb = in_region ? full_rgb : KEY_COLOR;

   // 1-cycle delay to sync with VGA pixel timing
   always_ff @(posedge clk)
      out_rgb_d1 <= out_rgb;

   assign sprite_rgb = out_rgb_d1;

endmodule
