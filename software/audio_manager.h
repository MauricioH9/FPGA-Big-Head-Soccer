#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "chu_init.h"
#include "ddfs_core.h"
#include "adsr_core.h"

// ========== 3 Octave Chromatic Range (C3 to D6) ==========
#define NOTE_C3   131
#define NOTE_CS3  139
#define NOTE_D3   147
#define NOTE_DS3  156
#define NOTE_E3   165
#define NOTE_F3   175
#define NOTE_FS3  185
#define NOTE_G3   196
#define NOTE_GS3  208
#define NOTE_A3   220
#define NOTE_AS3  233
#define NOTE_B3   247

#define NOTE_C4   262
#define NOTE_CS4  277
#define NOTE_D4   294
#define NOTE_DS4  311
#define NOTE_E4   330
#define NOTE_F4   349
#define NOTE_FS4  370
#define NOTE_G4   392
#define NOTE_GS4  415
#define NOTE_A4   440
#define NOTE_AS4  466
#define NOTE_B4   494

#define NOTE_C5   523
#define NOTE_CS5  554
#define NOTE_D5   587
#define NOTE_DS5  622
#define NOTE_E5   659
#define NOTE_F5   698
#define NOTE_FS5  740
#define NOTE_G5   784
#define NOTE_GS5  831
#define NOTE_A5   880
#define NOTE_AS5  932
#define NOTE_B5   988

#define NOTE_C6   1047
#define NOTE_CS6  1109
#define NOTE_D6   1175

#define REST      0

// ========== Note Durations ==========
#define WHOLE      1600
#define HALF       800
#define QUARTER    400
#define EIGHTH     200
#define SIXTEENTH  100

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))

// ========== Song Struct ==========
struct Song {
    int freq;
    int duration;
};

// ========== Audio Control ==========
void init_audio(DdfsCore* ddfs_core, AdsrCore* adsr_core);
void play_note(double freq, int duration_ms, int attack = 10, int decay = 10, int sustain = -1, int release = 100, float level = 0.8);
void play_smash_note(double freq, int duration_ms);

// ========== Sound Effects ==========
void play_kick_sound();
void play_collision_sound();
void play_goal_tune();
void play_countdown_beep(int n);

// ========== Async Song Playback ==========
void start_song(Song* song_array, int song_length, bool should_loop);
void play_song_tick();  // Call repeatedly inside main loop
bool is_song_done();

// ========== Song Data ==========
extern Song luffy_theme[];
extern Song zoro_theme[];
extern Song mario_intro[];
extern Song smash_splash[];

extern const int luffy_theme_len;
extern const int zoro_theme_len;
extern const int mario_intro_len;
extern const int smash_splash_len;

#endif // AUDIO_MANAGER_H
