module bar_src (
   input  logic        clk,
   input  logic [10:0] x, y,      // pixel coords 0-639, 0-479
   output logic [11:0] bar_rgb
);

   // screen geometry
   localparam int SCREEN_W = 640;
   localparam int SCREEN_H = 480;

   // vertical splits
  localparam int STAT_HUD =  60;            // top HUD height
  localparam int HORIZON  = SCREEN_H - 80;  // sky ends at y=400, 80px of grass below

   // colors (12-bit R:G:B)
   localparam [11:0] HUD_RGB       = 12'h888;  // mid-grey
   localparam [11:0] SCORE_BOX_RGB = 12'h000;  // black
   localparam [11:0] TIMER_BOX_RGB = 12'h000;  // black
   localparam [11:0] SKY_RGB       = 12'h8CF;  // light sky blue
   localparam [11:0] CLOUD_RGB1    = 12'hDEF;  // pale cloud
   localparam [11:0] CLOUD_RGB2    = 12'hFFF;  // white cloud
   localparam [11:0] SUN_RGB       = 12'hFF0;  // yellow sun
   localparam [11:0] GRASS_RGB2    = 12'h074;  // darker grass
   localparam [11:0] GRASS_RGB1    = 12'h0A8;  // lighter center stripe
   localparam [11:0] LINE_RGB      = 12'hFFF;  // white line

   // sun circle test
   logic signed [11:0] dx, dy;
   logic [23:0]        dist2;

   // pipeline regs
   logic [11:0] rgb_next, rgb_d1, rgb_d2;

   always_comb begin
      // --- 1) HUD area (y < STAT_HUD) ---
      if (y < STAT_HUD) begin
         rgb_next = HUD_RGB;
         // left score box
         if (x >=  10 && x < 100)            rgb_next = SCORE_BOX_RGB;
         // timer box (center)
         else if (x >= SCREEN_W/2-80 
               && x < SCREEN_W/2+80)       rgb_next = TIMER_BOX_RGB;
         // right score box
         else if (x >= SCREEN_W-100 
               && x < SCREEN_W-10)          rgb_next = SCORE_BOX_RGB;
      end

      // --- 2) sky + horizon (STAT_HUD <= y < HORIZON) ---
      else if (y < HORIZON) begin
         rgb_next = SKY_RGB;
         // sun
         dx    = x - 550; dy = y -  80;
         dist2 = dx*dx + dy*dy;
         if (dist2 < 35*35) rgb_next = SUN_RGB;
         // clouds #1
         if ((x>50 && x<150 && y>100 && y<140) ||
             (x>220&& x<320 && y> 60 && y<100))
            rgb_next = CLOUD_RGB1;
         // clouds #2
         if ((x>300&& x<380 && y>130 && y<160) ||
             (x>420&& x<500 && y> 90 && y<120))
            rgb_next = CLOUD_RGB2;
      end

      // --- 3) grass (y >= HORIZON, exactly 80px tall) ---
      else begin
         // carve 3 equal horizontal bands within that 80px:
         localparam int GRASS_H  = SCREEN_H - HORIZON;        // =80
         localparam int STRIPE_H = GRASS_H / 3;               // ?26
         localparam int T1       = HORIZON + STRIPE_H;        // 426
         localparam int T2       = HORIZON + 2*STRIPE_H;      // 452

         // center vertical line
         if (x == SCREEN_W/2)
            rgb_next = LINE_RGB;
         // top stripe (y < T1)
         else if (y < T1)
            rgb_next = GRASS_RGB2;
         // middle stripe (T1 ? y < T2)
         else if (y < T2)
            rgb_next = GRASS_RGB1;
         // bottom stripe (y ? T2)
         else
            rgb_next = GRASS_RGB2;
      end
   end

   // two-stage pipeline to align timing
   always_ff @(posedge clk) begin
      rgb_d1 <= rgb_next;
      rgb_d2 <= rgb_d1;
   end

   assign bar_rgb = rgb_d2;
endmodule