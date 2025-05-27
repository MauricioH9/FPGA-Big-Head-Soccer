module vga_sprite_ball_core 
  #( parameter CD         = 12,
               ADDR_WIDTH = 8,       //  entries
               KEY_COLOR  = 12'h000
   )
   (
    input  logic           clk, reset,
    input  logic  [10:0]   x, y,
    input  logic           cs,
    input  logic           write,
    input  logic  [13:0]   addr,
    input  logic  [31:0]   wr_data,
    input  logic  [CD-1:0] si_rgb,
    output logic  [CD-1:0] so_rgb
   );

   // decode writes
   logic wr_en    = write & cs;
   logic wr_ram   = ~addr[13] & wr_en;
   logic wr_reg   =  addr[13] & wr_en;
   logic wr_bypass= wr_reg && (addr[1:0]==2'b00);
   logic wr_x0    = wr_reg && (addr[1:0]==2'b01);
   logic wr_y0    = wr_reg && (addr[1:0]==2'b10);

   // regs for sprite origin & bypass
   logic [10:0] x0_reg, y0_reg;
   logic        bypass_reg;
   always_ff @(posedge clk or posedge reset) begin
     if (reset) begin
       x0_reg    <= 0;
       y0_reg    <= 0;
       bypass_reg<= 0;
     end else begin
       if (wr_x0)     x0_reg     <= wr_data[10:0];
       if (wr_y0)     y0_reg     <= wr_data[10:0];
       if (wr_bypass) bypass_reg <= wr_data[0];
     end
   end

   // sprite source
   logic [CD-1:0] ball_rgb;
   ball_src #(
     .CD        (CD),
     .ADDR      (ADDR_WIDTH),
     .KEY_COLOR (KEY_COLOR)
   ) ball_src_unit (
     .clk       (clk),
     .x         (x),
     .y         (y),
     .x0        (x0_reg),
     .y0        (y0_reg),
     .we        (wr_ram),
     .addr_w    (addr[ADDR_WIDTH-1:0]),
     .pixel_in  (wr_data[1:0]),  // matches DATA_WIDTH=2 in your RAM
     .sprite_rgb(ball_rgb)
   );

   // chroma-key/blend with background
   logic [CD-1:0] chrom_rgb = (ball_rgb != KEY_COLOR) ? ball_rgb : si_rgb;
   assign so_rgb = bypass_reg ? si_rgb : chrom_rgb;

endmodule
