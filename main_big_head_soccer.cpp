#include "chu_init.h"
#include "gpio_cores.h"
#include "vga_core.h"
#include "sseg_core.h"
#include "ps2_core.h"
#include "ddfs_core.h"
#include "adsr_core.h"
#include "audio_manager.h"
#include "game_physics.h"
#include <cstdio>
#include <cstring>
#include <cmath>

// ===== Constants =====
#define MATCH_DURATION_SEC 90
static const int GP_W = 8, GP_H = 80;
const int INVISIBLE_LINE_Y = SCREEN_H - 40;
static const float JUMP_VELOCITY = -10.0f * 0.75f;
static const float DEFAULT_ENV_LEVEL = 0.8;

// Goalpost positions
static const int LEFT_POST_X      = 10;
static const int RIGHT_POST_X     = SCREEN_W - 10 - GP_W;
static const int POST_INNER_LEFT  = LEFT_POST_X + GP_W;
static const int POST_INNER_RIGHT = RIGHT_POST_X;
static const int POST_TOP_Y       = SCREEN_H - GP_H;

// Scoreboard layout
static const int P1_COL = 7, P1_ROW = 2;
static const int P2_COL = 73, P2_ROW = 2;
static const int TIME_COL = 38, TIME_ROW = 2;

// ===== Core Instantiations =====
GpoCore    led      (get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore    sw       (get_slot_addr(BRIDGE_BASE, S3_SW));
SsegCore   sseg     (get_slot_addr(BRIDGE_BASE, S8_SSEG));
Ps2Core    ps2      (get_slot_addr(BRIDGE_BASE, S11_PS2));
DdfsCore   ddfs     (get_slot_addr(BRIDGE_BASE, S12_DDFS));
AdsrCore   adsr     (get_slot_addr(BRIDGE_BASE, S13_ADSR), &ddfs);

// Sprite Cores
SpriteCore player1   (get_sprite_addr(BRIDGE_BASE, V3_PLAYER1), 1024);
SpriteCore player2   (get_sprite_addr(BRIDGE_BASE, V4_PLAYER2), 1024);
SpriteCore ball      (get_sprite_addr(BRIDGE_BASE, V2_BALL),     256);
SpriteCore goalpost1 (get_sprite_addr(BRIDGE_BASE, V6_GOALPOST1), 512);
SpriteCore goalpost2 (get_sprite_addr(BRIDGE_BASE, V5_GOALPOST2), 512);
GpvCore    bar       (get_sprite_addr(BRIDGE_BASE, V7_BAR));
OsdCore    osd       (get_sprite_addr(BRIDGE_BASE, V1_OSD));

// ===== Game State =====
int p1_score = 0, p2_score = 0;
bool game_over = false;
unsigned long start_time;
bool p1_kick = false, p2_kick = false;

// ===== Function Prototypes =====
void draw_splash_credits();
void starting_splash_screen(OsdCore *osd_p, Ps2Core *ps2_p);
void load_goalposts();
void show_countdown(OsdCore *osd_p);
void draw_score_and_timer();
void update_sprite_positions();
void reset_positions(bool ball_on_ground);
void detect_goal();
void handle_ps2_input();
void process_controls();
void game_loop();

void starting_splash_screen(OsdCore *osd_p, Ps2Core *ps2_p) {
    osd_p->clr_screen();
    draw_splash_credits();
    osd_p->set_color(0xFF0, 0x000); // yellow on black

    const char *title = "BIG HEAD SOCCER";
    const char *prompt = "Press ENTER to start game";

    int title_len = strlen(title);
    int prompt_len = strlen(prompt);
    int title_x = (80 - title_len) / 2;
    int prompt_x = (80 - prompt_len) / 2;

    int title_row = 11;
    int prompt_row = 15;

    for (int i = 0; i < title_len; i++) {
        osd_p->wr_char(title_x + i, title_row, title[i]);
    }

    bool show_prompt = true;
    unsigned long last_flash_time = now_ms();

    // Start looping splash music
    start_song(smash_splash, smash_splash_len, true);

    bool enter_pressed = false;

    while (!enter_pressed) {
        unsigned long now = now_ms();

        // Flash the prompt every 700ms
        if (now - last_flash_time >= 700) {
            for (int i = 0; i < prompt_len; i++) {
                osd_p->wr_char(prompt_x + i, prompt_row, show_prompt ? prompt[i] : ' ');
            }
            show_prompt = !show_prompt;
            last_flash_time = now;
        }

        play_song_tick();  // Advance the looping music

        // Exit only when Enter (scan code 0x5A) is pressed
        while (!ps2_p->rx_fifo_empty()) {
            int code = ps2_p->rx_byte();
            if (code == 0x5A) {
                enter_pressed = true;
                break;
            }
            if (code == 0xF0) {
                (void)ps2_p->rx_byte(); // skip break codes
            }
        }

        sleep_ms(20);  // Smooth updates
    }

    // Stop looping, play Mario intro once
    start_song(mario_intro, mario_intro_len, false);
    while (!is_song_done()) {
        play_song_tick();
        sleep_ms(20);
    }

    osd_p->clr_screen();  // Clear before countdown
}

// Fill both goalpost RAMs with a 1 pixel border (code=1) and filled interior (code=2)
void load_goalposts() {
    for (int row = 0; row < GP_H; row++) {
        for (int col = 0; col < GP_W; col++) {
            uint8_t code = (row == 0 || row == GP_H-1 || col == 0 || col == GP_W-1) ? 1 : 2;
            int addr = row * GP_W + col;
            goalpost1.wr_mem(addr, code);
            goalpost2.wr_mem(addr, code);
        }
    }
}

void show_countdown(OsdCore *osd_p) {
  const char *msgs[] = { "3", "2", "1", "START!!!" };
  osd_p->set_color(0xF00, 0x000); // red on black

  for (int i = 0; i < 4; i++) {
    osd_p->clr_screen();

    const char *msg = msgs[i];
    int msg_len = strlen(msg);
    int x = (80 - msg_len) / 2;  // Center horizontally (80 columns)
    int y = 12;                  // Center vertically

    for (int j = 0; j < msg_len; j++) {
      osd_p->wr_char(x + j, y, msg[j]);
    }

    // Play sound: short for 3,2,1 — long for START!!!
    play_countdown_beep(i);
    if (i < 3) {
      sleep_ms(200);  // short beep
    } else {
      sleep_ms(600);  // longer "GO" tone
    }

    osd_p->clr_screen(); // hide message
    sleep_ms(400);       // pause between flashes
  }

}

void draw_score_and_timer() {
    osd.clr_screen();
    osd.set_color(0xFFF, 0x000); // white on black for best visibility

    // ==== PLAYER 1 label (shift left to center) ====
    const char *p1_label = "PLAYER 1";
    int p1_label_len = strlen(p1_label);
    int p1_label_col = P1_COL - (p1_label_len - 1) / 2;  // Shift left to center above score
    for (int i = 0; i < p1_label_len; i++) {
        osd.wr_char(p1_label_col + i, P1_ROW - 1, p1_label[i]);
    }

    // ==== Draw P1 score ====
    {
        char buf[4];
        sprintf(buf, "%d", p1_score);
        for (int i = 0; buf[i]; i++) {
            osd.wr_char(P1_COL + i, P1_ROW, buf[i]);
        }
    }

    // ==== PLAYER 2 label (shift left to center) ====
    const char *p2_label = "PLAYER 2";
    int p2_label_len = strlen(p2_label);
    int p2_label_col = P2_COL - (p2_label_len - 1) / 2;  // Same logic for player 2
    for (int i = 0; i < p2_label_len; i++) {
        osd.wr_char(p2_label_col + i, P2_ROW - 1, p2_label[i]);
    }

    // ==== Draw P2 score ====
    {
        char buf[4];
        sprintf(buf, "%d", p2_score);
        for (int i = 0; buf[i]; i++) {
            osd.wr_char(P2_COL + i, P2_ROW, buf[i]);
        }
    }

    // ==== Draw TIME label ====
    {
        const char *label = "TIME";
        int label_len = strlen(label);
        int time_len = 5;  // MM:SS
        int label_row = TIME_ROW - 1;
        int centered_col = TIME_COL + (time_len - label_len) / 2;

        for (int i = 0; i < label_len; i++) {
            osd.wr_char(centered_col + i, label_row, label[i]);
        }
    }

    // ==== Draw actual timer ====
    {
        char buf[8];
        unsigned long elapsed = (now_ms() - start_time) / 1000;
        int remaining = MATCH_DURATION_SEC - (int)elapsed;
        sprintf(buf, "%02d:%02d", remaining / 60, remaining % 60);
        for (int i = 0; buf[i]; i++) {
            osd.wr_char(TIME_COL + i, TIME_ROW, buf[i]);
        }
    }

    // ==== Player 1 Instructions ====
    const char *p1_kick = "Move:  A W D";
    const char *p1_move = "Kick: SPACEBAR";
    int p1_instr_col = 14;  // Adjust until visually centered in left gray box
    int instr_row1 = 1;     // row below PLAYER 1 label
    int instr_row2 = 2;

    for (int i = 0; p1_kick[i]; i++)
        osd.wr_char(p1_instr_col + i, instr_row1, p1_kick[i]);

    for (int i = 0; p1_move[i]; i++)
        osd.wr_char(p1_instr_col + i, instr_row2, p1_move[i]);

    // ==== Player 2 Instructions ====
    const char *p2_kick = "Move: \x1B \x18 \x1A";
    const char *p2_move = "Kick:   P";  // ← ↑ → (ASCII arrow symbols if supported)
    int p2_instr_col = 53;  // Adjust until centered in right gray box
    int instr_row3 = 1;
    int instr_row4 = 2;

    for (int i = 0; p2_kick[i]; i++)
        osd.wr_char(p2_instr_col + i, instr_row3, p2_kick[i]);

    for (int i = 0; p2_move[i]; i++)
        osd.wr_char(p2_instr_col + i, instr_row4, p2_move[i]);

}

void draw_splash_credits() {
	osd.set_color(0xFFF, 0x000);  // white on black
    const char* name_left  = "Mauricio Herrera";
    const char* name_right = "Michael Rosales";
    const char* middle     = "ECE 4305";

    // ==== Mauricio Herrera ====
    osd.set_color(0xFFF, 0x000); // white
    for (int i = 0; name_left[i]; i++) {
        osd.wr_char(13 + i, 2, name_left[i]);
    }

    // ==== Michael Rosales ====
    for (int i = 0; name_right[i]; i++) {
        osd.wr_char(51 + i, 2, name_right[i]);
    }

    // ==== ECE 4305 ====
    int mid_col = TIME_COL + (5 - strlen(middle)) / 2;
    for (int i = 0; middle[i]; i++) {
    	osd.wr_char(mid_col + i, TIME_ROW, middle[i]);
    }
}


void update_sprite_positions() {
    player1.move_xy(p1_x, p1_y);
    player2.move_xy(p2_x, p2_y);
    ball   .move_xy(ball_x, ball_y);

    int post_y = INVISIBLE_LINE_Y - GP_H;
    goalpost1.move_xy(10,                   post_y);
    goalpost2.move_xy(SCREEN_W - 10 - GP_W, post_y);

}

 void reset_positions(bool ball_on_ground) {
    // ball back to center or floor
    ball_x = SCREEN_W / 2 - BALL_W / 2;
    ball_y = ball_on_ground ? (INVISIBLE_LINE_Y - BALL_H) : (SCREEN_H / 2 - BALL_H / 2);
    ball_vx = 0;
    ball_vy = ball_on_ground ? 0 : 5;

    // players back on ground line
    p1_x = 50;
    p1_y = INVISIBLE_LINE_Y - PLAYER_H;
    p2_x = SCREEN_W - 50 - PLAYER_W;
    p2_y = INVISIBLE_LINE_Y - PLAYER_H;

    p1_vy = p2_vy = 0;
    p1_on_ground = p2_on_ground = ball_on_ground;
}

void detect_goal() {
    const char* goal_msg = "GOLAZO!!!";
    int msg_len = strlen(goal_msg);
    int msg_x = (80 - msg_len) / 2;     // 80 columns on screen
    int msg_y = 15;                     // center vertically (30 rows / 2)

    auto show_golazo = [&]() {
        osd.set_color(0xFF0, 0x000);  // yellow on black
        for (int i = 0; i < msg_len; i++) {
            osd.wr_char(msg_x + i, msg_y, goal_msg[i]);
        }
        play_goal_tune();
        sleep_ms(1000);  // let message stay briefly
        osd.clr_screen();  // clear it before next play
    };

    // Left goal: ball’s right edge fully past the left post’s inner X
    if (ball_x + BALL_W < POST_INNER_LEFT) {
        p2_score++;
        show_golazo();
        reset_positions(false); // ball starts in air
        update_sprite_positions();
        return;
    }

    // Right goal: ball’s left edge fully past the right post’s inner X
    if (ball_x > POST_INNER_RIGHT) {
        p1_score++;
        show_golazo();
        reset_positions(false); // ball starts in air
        update_sprite_positions();
        return;
    }
}

void handle_ps2_input() {
    static bool break_code = false;

    while (!ps2.rx_fifo_empty()) {
        int code = ps2.rx_byte();

        if (code == 0xF0) {
            break_code = true;
        } else {
            if (break_code) {
                key_state[code] = false;  // Key released
                break_code = false;
            } else {
                key_state[code] = true;   // Key pressed
            }
        }
    }
}

void process_controls() {
    // - Player 1 Movement -
    if (key_state[0x1C]) p1_x -= 5;            // A
    if (key_state[0x23]) p1_x += 5;            // D
    if (key_state[0x1D] && p1_on_ground) {     // W
        p1_vy = JUMP_VELOCITY;
        p1_on_ground = false;
    }

    // - Player 2 Movement -
    if (key_state[0x6B]) p2_x -= 5;            // <- Arrow
    if (key_state[0x74]) p2_x += 5;            // -> Arrow
    if (key_state[0x75] && p2_on_ground) {     // ^ Arrow
        p2_vy = JUMP_VELOCITY;
        p2_on_ground = false;
    }

    // - Kick‐sprite toggle for P1 (Space = 0x29) -
    if (key_state[0x29]) {
        if (!p1_kick) {
            p1_kick = true;
            player1.bypass(0);
            player1.wr_ctrl(0x02);
        }
    } else {
        if (p1_kick) player1.wr_ctrl(0x00);
        p1_kick = false;
    }

    // - Kick‐sprite toggle for P2 (P = 0x4D) -
    if (key_state[0x4D]) {
        if (!p2_kick) {
            p2_kick = true;
            player2.bypass(0);
            player2.wr_ctrl(0x02);
        }
    } else {
        if (p2_kick) player2.wr_ctrl(0x00);
        p2_kick = false;
    }

    // - Manual clamping on screen -
    if (p1_x < 0) p1_x = 0;
    else if (p1_x > SCREEN_W - PLAYER_W) p1_x = SCREEN_W - PLAYER_W;

    if (p2_x < 0) p2_x = 0;
    else if (p2_x > SCREEN_W - PLAYER_W) p2_x = SCREEN_W - PLAYER_W;
}

void game_loop() {
  start_time = now_ms();
  unsigned long last_collision_time = 0;

  while (!game_over) {
    unsigned long now = now_ms(); //  Get this once per frame

    handle_ps2_input();
    process_controls();

    apply_gravity_and_ground();
    update_ball_motion();

    // === Player-Ball Collision ===
    bool is_p1_kicking = key_state[0x29];  // spacebar
    bool is_p2_kicking = key_state[0x4D];  // P key

    handle_player_ball_collision(p1_x, p1_y, is_p1_kicking, now, last_collision_time, 1);
    handle_player_ball_collision(p2_x, p2_y, is_p2_kicking, now, last_collision_time, 2);


    detect_goal();
    update_sprite_positions();
    draw_score_and_timer();

    if ((now - start_time) / 1000 >= MATCH_DURATION_SEC) {
        game_over = true;
    }

    sleep_ms(30);
  }

    // Game over message
    char win_msg[30];
    bool is_draw = (p1_score == p2_score);
    if (is_draw) {
        sprintf(win_msg, "Draw!!!");
    } else {
        sprintf(win_msg, "Player %d has won!!!", (p1_score > p2_score) ? 1 : 2);
    }

    int win_len = strlen(win_msg);
    int win_x = (80 - win_len) / 2;
    int y_pos = 13;

    Song* theme = is_draw ? zoro_theme : luffy_theme;
    int theme_len = is_draw ? zoro_theme_len : luffy_theme_len;

    // Interleave theme and flashing
    for (int i = 0; i < theme_len; i++) {
        osd.set_color(0x080, 0x000);  // green on black
        for (int j = 0; j < win_len; j++) {
            osd.wr_char(win_x + j, y_pos, (i % 2 == 0) ? win_msg[j] : ' ');
            //play_note(theme[i].freq, theme[i].duration);
        }
        play_note(theme[i].freq, theme[i].duration);
    }

    // ==== Phase 2: Flash both winner/draw and restart messages ====
    osd.set_color(0x080, 0x000); // green on black
    const char *restart_msg = "Press ENTER to play again";
    int restart_len = strlen(restart_msg);
    int restart_x = (80 - restart_len) / 2;

    bool show_text = true;
    bool enter_pressed = false;

    while (!enter_pressed) {
        // Flash winner and prompt
        if (show_text) {
            for (int i = 0; i < win_len; i++) osd.wr_char(win_x + i, y_pos, win_msg[i]);
            for (int i = 0; i < restart_len; i++) osd.wr_char(restart_x + i, y_pos + 2, restart_msg[i]);
        } else {
            for (int i = 0; i < win_len; i++) osd.wr_char(win_x + i, y_pos, ' ');
            for (int i = 0; i < restart_len; i++) osd.wr_char(restart_x + i, y_pos + 2, ' ');
        }

        show_text = !show_text;
        sleep_ms(700);

        // Check for Enter (scan code 0x5A) press only
        while (!ps2.rx_fifo_empty()) {
            int code = ps2.rx_byte();
            if (code == 0x5A) {
                enter_pressed = true;
                break;
            }
            if (code == 0xF0) {
                // skip break codes
                (void)ps2.rx_byte();
            }
        }
    }
}

int main() {
    init_audio(&ddfs, &adsr);
    load_goalposts();

    while (true) {
        reset_positions(true);
        update_sprite_positions();
        p1_score = p2_score = 0;
        game_over = false;

        player1.bypass(0);
        player2.bypass(0);
        bar.bypass(0);
        ball.bypass(0);
        goalpost1.bypass(0);
        goalpost2.bypass(0);
        osd.bypass(0);

        while (!ps2.rx_fifo_empty()) (void)ps2.rx_byte();
        starting_splash_screen(&osd, &ps2);
        show_countdown(&osd);

        reset_positions(false);
        update_sprite_positions();
        game_loop();
    }

    return 0;
}

