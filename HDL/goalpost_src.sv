module goalpost_src #(
  parameter CD        = 12,
            ADDR      = 10,            // 2^8=512 entries for 64*8
            KEY_COLOR = 12'h000
) (
  input  logic        clk,
  input  logic [10:0] x, y,
  input  logic [10:0] x0, y0,
  input  logic        we,
  input  logic [ADDR-1:0] addr_w,
  input  logic [1:0]  pixel_in,
  output logic [CD-1:0] sprite_rgb
);

  localparam H_SIZE = 8, V_SIZE = 80;

  // compute position within the square
  logic signed [11:0] xr = x - x0, yr = y - y0;
  logic in_region = xr>=0 && xr<H_SIZE && yr>=0 && yr<V_SIZE;

  // --- address for 8×8 RAM: 3 bits of Y, 3 bits of X ---
  logic [ADDR-1:0] addr_r;
  assign addr_r = { yr[6:0], xr[2:0] };

  // read 2-bit code
  logic [1:0] plt_code;
  goalpost_ram_lut #(.ADDR_WIDTH(ADDR), .DATA_WIDTH(2)) ram_unit (
    .clk(clk), .we(we), .addr_w(addr_w),
    .din(pixel_in),
    .addr_r(addr_r),
    .dout(plt_code)
  );

  // palette ? 12-bit RGB
  logic [11:0] full_rgb;
  always_comb begin
    case (plt_code)
      2'b00: full_rgb = KEY_COLOR;
      2'b01: full_rgb = 12'h100;  // Black Outline
      2'b10: full_rgb = 12'hFFF;  // white
      default:  full_rgb = 12'h000;
    endcase
  end

  // only draw inside the 64×8 square
  logic [11:0] out_rgb = in_region ? full_rgb : KEY_COLOR;
  always_ff @(posedge clk) sprite_rgb <= out_rgb;

endmodule