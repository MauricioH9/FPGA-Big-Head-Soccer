module player2_ram_lut 
   #(
    parameter ADDR_WIDTH = 11,  // number of address bits
              DATA_WIDTH = 3    // up to 7 diff colors
   )
   (
    input  logic clk,
    input  logic we,
    input  logic [ADDR_WIDTH-1:0] addr_r,
    input  logic [ADDR_WIDTH-1:0] addr_w,
    input  logic [DATA_WIDTH-1:0] din,
    output logic [DATA_WIDTH-1:0] dout
   );

   // signal declaration
   logic [DATA_WIDTH-1:0] ram [0:2**ADDR_WIDTH-1];
   logic [DATA_WIDTH-1:0] data_reg;
   
   // player2_map.mem specifies the initial values of ram 
   initial 
      $readmemh("player2_map.mem", ram);
      
   // body
   always_ff @(posedge clk)
   begin
      if (we)
         ram[addr_w] <= din;
      data_reg <= ram[addr_r];
   end
   assign dout = data_reg;
endmodule   
