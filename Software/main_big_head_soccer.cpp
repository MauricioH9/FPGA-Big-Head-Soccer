#include "chu_init.h"
#include "gpio_cores.h"
#include "vga_core.h"
#include "sseg_core.h"
#include "ps2_core.h"
#include "ddfs_core.h"
#include "adsr_core.h"
#include <cstdio>
#include <cstring>

// External core instantiations
GpoCore led(get_slot_addr(BRIDGE_BASE, S2_LED));
GpiCore sw(get_slot_addr(BRIDGE_BASE, S3_SW));
SsegCore sseg(get_slot_addr(BRIDGE_BASE, S8_SSEG));
Ps2Core ps2(get_slot_addr(BRIDGE_BASE, S11_PS2));
DdfsCore ddfs(get_slot_addr(BRIDGE_BASE, S12_DDFS));
AdsrCore adsr(get_slot_addr(BRIDGE_BASE, S13_ADSR), &ddfs);

// Sprites
SpriteCore player1(get_sprite_addr(BRIDGE_BASE, V3_PLAYER1), 1024);
SpriteCore player2(get_sprite_addr(BRIDGE_BASE, V4_PLAYER2), 1024);
SpriteCore ball(get_sprite_addr(BRIDGE_BASE, V2_BALL), 1024);
OsdCore osd(get_sprite_addr(BRIDGE_BASE, V1_OSD));

// Constants
const int SCREEN_W = 640;
const int SCREEN_H = 480;
const int PLAYER_W = 32;
const int PLAYER_H = 32;
const int BALL_W = 16;
const int BALL_H = 16;
const float DEFAULT_ENV_LEVEL = 0.8;


// 2 Octave Chromatic range C4 to C6
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define REST     0


// Note Durations
#define WHOLE     1600
#define HALF      800
#define QUARTER   400
#define EIGHTH    200
#define SIXTEENTH 100

// forward music declarations
void play_kick_sound();
void play_countdown_beep(int n);
void play_goal_tune();
void play_mario_intro();


// Game state
int p1_x = 50, p1_y = SCREEN_H - PLAYER_H;
int p2_x = SCREEN_W - 50 - PLAYER_W, p2_y = SCREEN_H - PLAYER_H;
int ball_x = SCREEN_W / 2 - BALL_W / 2, ball_y = 0;
int p1_score = 0, p2_score = 0;
bool game_over = false;
unsigned long start_time;

// Gravity and velocities
int ball_vx = 0, ball_vy = 5;
bool p1_kick = false, p2_kick = false;

// =====Music/Sound effects====
void play_note(double freq, int duration_ms, int attack = 10, int decay = 10, int sustain = 400, int release = 100, float level = 0.8) {
  ddfs.set_carrier_freq((int)freq);
  adsr.set_env(attack, decay, sustain, release, level);
  adsr.start();
  sleep_ms(duration_ms);
}


void play_kick_sound() {
  play_note(NOTE_C4, 80, 2, 10, 200, 50);
}

void play_countdown_beep(int n) {
  if (n < 3) {
    play_note(NOTE_C5, 150, 5, 10, 300, 100);  // for "3", "2", "1"
  } else if (n == 3) {
    play_note(NOTE_G5, 500, 10, 10, 500, 200); // for "START!!!"
  }
}


void play_goal_tune() {
  play_note(NOTE_C5, EIGHTH);
  play_note(NOTE_E5, EIGHTH);
  play_note(NOTE_G5, QUARTER);
}

void play_mario_intro() {
  play_note(NOTE_E5, EIGHTH);
  play_note(NOTE_E5, EIGHTH);
  play_note(REST, EIGHTH);
  play_note(NOTE_E5, EIGHTH);
  play_note(REST, EIGHTH);
  play_note(NOTE_C5, EIGHTH);
  play_note(NOTE_E5, EIGHTH);
  play_note(REST, EIGHTH);
  play_note(NOTE_G5, QUARTER);
  play_note(REST, QUARTER);
  play_note(NOTE_G4, QUARTER);
}


void starting_splash_screen(OsdCore *osd_p, Ps2Core *ps2_p) {
    osd_p->set_color(0x0f0, 0x000); // green on black
    osd_p->clr_screen();

    const char *title = "BIG HEAD SOCCER";
    const char *prompt = "Press any button to start game";

    int title_len = strlen(title);
    int prompt_len = strlen(prompt);
    int title_x = (80 - title_len) / 2;   // Center horizontally
    int prompt_x = (80 - prompt_len) / 2;

    // Print title on top row (e.g., row 5)
    for (int i = 0; i < title_len; i++) {
        osd_p->wr_char(title_x + i, 5, title[i]);
    }

    bool show_prompt = true;
    while (true) {
        // Flash prompt on row 15 (middle of screen)
        for (int i = 0; i < prompt_len; i++) {
            osd_p->wr_char(prompt_x + i, 15, show_prompt ? prompt[i] : ' ');
        }

        sleep_ms(500);
        show_prompt = !show_prompt;

        if (!ps2_p->rx_fifo_empty()) {
            (void)ps2_p->rx_byte();
            break;
        }
    }

    osd_p->clr_screen();
}


void show_countdown(OsdCore *osd_p) {
  const char *msgs[] = { "3", "2", "1", "START!!!" };
  osd_p->set_color(0x0f0, 0x000); // green on black

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
  //play_mario_intro();
}

void draw_score_and_timer() {
  osd.clr_screen();
  char buf[40];
  unsigned long elapsed = (now_ms() - start_time) / 1000;
  int remaining = 20 - (int)elapsed; // Change time here
  sprintf(buf, "P1: %d  P2: %d", p1_score, p2_score);
  for (int i = 0; i < (int)strlen(buf); i++) osd.wr_char(i, 0, buf[i]);
  sprintf(buf, "Time: %02d", remaining);
  for (int i = 0; i < (int)strlen(buf); i++) osd.wr_char(35 + i, 0, buf[i]);
}

void update_sprite_positions() {
  player1.move_xy(p1_x, p1_y);
  player2.move_xy(p2_x, p2_y);
  ball.move_xy(ball_x, ball_y);
}


void detect_goal() {
  if (ball_x <= 0) {
    p2_score++;
    play_goal_tune();
    ball_x = SCREEN_W / 2 - BALL_W / 2;
    ball_y = 0;
    ball_vx = 0;
    ball_vy = 5;
  } else if (ball_x + BALL_W >= SCREEN_W) {
    p1_score++;
    play_goal_tune();
    ball_x = SCREEN_W / 2 - BALL_W / 2;
    ball_y = 0;
    ball_vx = 0;
    ball_vy = 5;
  }
}

void handle_ps2_input() {
  int key = ps2.rx_byte();
  if (key == 0x1c) p1_x -= 5;         // A
  if (key == 0x23) p1_x += 5;         // D
  if (key == 0x1d) p1_y -= 10;        // W
  if (key == 0x29) {                 // Spacebar
    if (abs(p1_x - ball_x) < PLAYER_W && abs(p1_y - ball_y) < PLAYER_H) {
      p1_kick = true;
      ball_vx = 3;
      ball_vy = -2;
      play_kick_sound();
    }
  }
  if (key == 0x75) p2_y -= 10;        // Up
  if (key == 0x6b) p2_x -= 5;         // Left
  if (key == 0x74) p2_x += 5;         // Right
  if (key == 0x4d) {                 // P
    if (abs(p2_x - ball_x) < PLAYER_W && abs(p2_y - ball_y) < PLAYER_H) {
      p2_kick = true;
      ball_vx = -3;
      ball_vy = -2;
      play_kick_sound();
    }
  }
}

void game_loop() {
  start_time = now_ms();
  while (!game_over) {
    handle_ps2_input();
    if (p1_y < SCREEN_H - PLAYER_H) p1_y++;
    if (p2_y < SCREEN_H - PLAYER_H) p2_y++;
    if (ball_y < SCREEN_H - BALL_H) ball_y += ball_vy;
    else ball_vy = 0;
    ball_x += ball_vx;
    detect_goal();
    update_sprite_positions();
    draw_score_and_timer();
    if ((now_ms() - start_time) >= 20000) { // change time here
      game_over = true;
      play_mario_intro(); // plays when game is done
    }
    sleep_ms(30);
  }
  // Game over message
  char win_msg[30];
  if (p1_score == p2_score) {
      sprintf(win_msg, "Draw!!!");
  } else {
      sprintf(win_msg, "Player %d has won!!!", (p1_score > p2_score) ? 1 : 2);
  }
  const char *restart_msg = "Press any button to play again";

  int win_len = strlen(win_msg);
  int restart_len = strlen(restart_msg);
  int win_x = (80 - win_len) / 2;
  int restart_x = (80 - restart_len) / 2;
  int y_pos = 13;

  // Flash the winner message for 2 seconds first
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < win_len; j++) osd.wr_char(win_x + j, y_pos, (i % 2 == 0) ? win_msg[j] : ' ');
    sleep_ms(500);
  }

  // Now flash both the win message and the restart prompt
  while (true) {
    for (int i = 0; i < win_len; i++) osd.wr_char(win_x + i, y_pos, win_msg[i]);
    for (int i = 0; i < restart_len; i++) osd.wr_char(restart_x + i, y_pos + 2, restart_msg[i]);
    sleep_ms(500);

    for (int i = 0; i < win_len; i++) osd.wr_char(win_x + i, y_pos, ' ');
    for (int i = 0; i < restart_len; i++) osd.wr_char(restart_x + i, y_pos + 2, ' ');
    sleep_ms(500);

    if (!ps2.rx_fifo_empty()) {
    	(void)ps2.rx_byte();
      break;
    }
  }
}

void hide_all_sprites() {
  player1.move_xy(-100, -100);
  player2.move_xy(-100, -100);
  ball.move_xy(-100, -100);
}



int main() {
    while (true) {
        hide_all_sprites();
        starting_splash_screen(&osd, &ps2);

        player1.bypass(0);
        player2.bypass(0);
        ball.bypass(0);
        osd.bypass(0);

        // Reset positions and scores
        p1_x = 50;
        p1_y = SCREEN_H - PLAYER_H;
        p2_x = SCREEN_W - 50 - PLAYER_W;
        p2_y = SCREEN_H - PLAYER_H;
        ball_x = SCREEN_W / 2;
        ball_y = 0;
        ball_vx = 0;
        ball_vy = 2;
        p1_score = 0;
        p2_score = 0;
        game_over = false;

        update_sprite_positions();
        show_countdown(&osd);
        game_loop();
    }

    return 0;
}



