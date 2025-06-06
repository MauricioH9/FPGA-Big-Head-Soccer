# FPGA-Big-Head-Soccer

Big Head Soccer is a fully playable 2D multiplayer soccer game implemented on an FPGA. The project showcases real-time graphics rendering, hardware/software co-design, SoC architecture, and digital signal processing.
## Hardware
The hardware design is written in SystemVerilog using Xilinx Vivado. It features custom cores for sprite rendering, PS2 keyboard input, and visuals displayed through VGA output for smooth real-time animation. It also integrates ADSR and DDFS audio cores to generate dynamic in-game sound effects and music.
## Software
The software design is written in C++ using Xilinx Vitis. It handles the game logic, collision detection, scoring system, win/draw conditions, music, and game timer. This runs on the MicroBlaze soft processor within an SoC architecture, demonstrating a strong hardware/software co-design approach. To create a dynamic in-game experience, we composed unique melodies and sound effects for kicks, countdowns, goals, and victory tunes.
