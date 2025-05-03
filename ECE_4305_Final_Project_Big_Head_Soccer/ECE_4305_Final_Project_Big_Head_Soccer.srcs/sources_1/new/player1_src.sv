module player1_src 
   #(
    parameter CD = 12,      // color depth (RGB: 12 bits)
              ADDR = 10,    // address bits for 1024 entries (32x32)
              KEY_COLOR = 12'h000  // transparent color
   )
   (
    input  logic clk,
    input  logic [10:0] x, y,    // current pixel position (from hcount/vcount)
    input  logic [10:0] x0, y0,  // sprite top-left position (from registers)
    
    // sprite RAM write interface 
    input  logic we,
    input  logic [ADDR-1:0] addr_w,
    input  logic [1:0] pixel_in,

    // output
    output logic [CD-1:0] sprite_rgb
   );

   // Localparams
   localparam H_SIZE = 32;
   localparam V_SIZE = 32;

   // Signals
   logic signed [11:0] xr, yr;
   logic [ADDR-1:0] addr_r;
   logic [1:0] plt_code;
   logic in_region;
   logic [CD-1:0] full_rgb;
   logic [CD-1:0] out_rgb, out_rgb_d1;

   // Instantiate RAM
   player1_ram_lut #(.ADDR_WIDTH(ADDR), .DATA_WIDTH(2)) ram_unit (
      .clk(clk),
      .we(we),
      .addr_w(addr_w),
      .din(pixel_in),
      .addr_r(addr_r),
      .dout(plt_code)
   );

   // Relative position
   assign xr = $signed({1'b0, x}) - $signed({1'b0, x0});
   assign yr = $signed({1'b0, y}) - $signed({1'b0, y0});
   assign addr_r = {yr[4:0], xr[4:0]};  // 32 x 32 = 1024 entries

   // In-bounds check
   assign in_region = (xr >= 0 && xr < H_SIZE && yr >= 0 && yr < V_SIZE);

   // Palette decode
   always_comb begin
      case (plt_code)
         2'b00: full_rgb = KEY_COLOR;   // transparent
         2'b01: full_rgb = 12'hF00;     // red
         2'b10: full_rgb = 12'h0F0;     // green
         2'b11: full_rgb = 12'h00F;     // blue
         default: full_rgb = 12'h000;
      endcase
   end

   // Output RGB
   assign out_rgb = in_region ? full_rgb : KEY_COLOR;

   // 1-cycle delay to sync with VGA pixel timing
   always_ff @(posedge clk)
      out_rgb_d1 <= out_rgb;

   assign sprite_rgb = out_rgb_d1;

endmodule
