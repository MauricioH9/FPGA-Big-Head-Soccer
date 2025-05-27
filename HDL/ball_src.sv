module ball_src #(
  parameter CD        = 12,
            ADDR      = 8,            // 2^8=256 entries for 16×16
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

  // sprite dimensions: 16×16
  localparam H_SIZE = 16;
  localparam V_SIZE = 16;

  // compute relative position
  logic signed [11:0] xr = x - x0;
  logic signed [11:0] yr = y - y0;
  logic in_region = (xr >= 0 && xr < H_SIZE && yr >= 0 && yr < V_SIZE);

  // build 8-bit read address: 4 bits of Y (yr), 4 bits of X (xr)
  logic [ADDR-1:0] addr_r;
  assign addr_r = { yr[3:0], xr[3:0] };

  // read 2-bit code from RAM
  logic [1:0] plt_code;
  ball_ram_lut #(
    .ADDR_WIDTH(ADDR),
    .DATA_WIDTH(2)
  ) ram_unit (
    .clk(clk),
    .we(we),
    .addr_w(addr_w),
    .din(pixel_in),
    .addr_r(addr_r),
    .dout(plt_code)
  );

  // palette decode
  logic [CD-1:0] full_rgb;
  always_comb begin
    case (plt_code)
      2'b00: full_rgb = KEY_COLOR; // transparent
      2'b01: full_rgb = 12'hFFF;   // white fill
      2'b10: full_rgb = 12'h100;   // black outline
      2'b11: full_rgb = 12'hF0F;   // purple accent
      default: full_rgb = KEY_COLOR;
    endcase
  end

  // final output: inside region
  logic [CD-1:0] out_rgb = in_region ? full_rgb : KEY_COLOR;
  always_ff @(posedge clk) sprite_rgb <= out_rgb;

endmodule
