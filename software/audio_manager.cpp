// audio_manager.cpp
#include "audio_manager.h"
#include "chu_init.h"       // for sleep_ms
#include <cmath>            // for fabs

// ========== Static Core Pointers ==========
static DdfsCore* ddfs = nullptr;
static AdsrCore* adsr = nullptr;

// ========== Initialization ==========
void init_audio(DdfsCore* ddfs_core, AdsrCore* adsr_core) {
    ddfs = ddfs_core;
    adsr = adsr_core;
}

// ========== Basic Note Playback ==========
void play_note(double freq, int duration_ms, int attack, int decay, int sustain, int release, float level) {
    ddfs->set_carrier_freq((int)freq);
    int sustain_time = (sustain == -1) ? (duration_ms - (attack + decay + release)) : sustain;
    if (sustain_time < 0) sustain_time = 0;
    adsr->set_env(attack, decay, sustain_time, release, level);
    adsr->start();
    sleep_ms(duration_ms);
}

void play_smash_note(double freq, int duration_ms) {
    if (freq == REST) {
        sleep_ms(duration_ms);
        return;
    }
    int attack = 2, decay = 2, release = 10;
    int sustain = duration_ms - (attack + decay + release);
    if (sustain < 0) sustain = 0;
    ddfs->set_carrier_freq((int)freq);
    adsr->set_env(attack, decay, sustain, release, 0.9);
    adsr->start();
    while (!adsr->idle()) sleep_ms(1);
}

// ========== Song Playback State ==========
static Song* current_song = nullptr;
static int current_song_len = 0;
static int song_index = 0;
static unsigned long note_start_time = 0;
static bool note_playing = false;
static bool song_done = true;
static bool song_loop = false;

void start_song(Song* song_array, int song_length, bool loop) {
    current_song = song_array;
    current_song_len = song_length;
    song_index = 0;
    note_playing = false;
    song_done = false;
    song_loop = loop;
}

void play_song_tick() {
    if (song_done || current_song == nullptr || current_song_len <= 0)
        return;

    unsigned long now = now_ms();

    if (!note_playing) {
        int freq = current_song[song_index].freq;
        int dur  = current_song[song_index].duration;

        if (freq != REST) {
            int sustain_time = (dur - 30 > 0) ? dur - 30 : 5;
            ddfs->set_carrier_freq(freq);
            adsr->set_env(5, 10, sustain_time, 20, 0.8);
            adsr->start();
        }

        note_start_time = now;
        note_playing = true;
    } else {
        int dur = current_song[song_index].duration;
        if ((now - note_start_time) >= (unsigned long)dur) {
            song_index++;
            note_playing = false;

            if (song_index >= current_song_len) {
                if (song_loop)
                    song_index = 0;
                else
                    song_done = true;
            }
        }
    }
}

bool is_song_done() {
    return song_done;
}

// ========== Sound Effects ==========
void play_collision_sound() {
    ddfs->set_carrier_freq(NOTE_C4);
    adsr->set_env(5, 10, 0, 50, 1.0);
    adsr->start();
}

void play_kick_sound() {
    ddfs->set_carrier_freq(NOTE_C5);
    adsr->set_env(5, 15, 0, 60, 1.0);
    adsr->start();
}

void play_countdown_beep(int n) {
    if (n < 3)
        play_note(NOTE_C5, 150, 5, 10, 300, 100);
    else
        play_note(NOTE_G5, 500, 10, 10, 500, 200);
}

void play_goal_tune() {
    play_note(NOTE_C5, EIGHTH);
    play_note(NOTE_E5, EIGHTH);
    play_note(NOTE_G5, QUARTER);
}

// =========  Songs ========
Song mario_intro[] = {
    {NOTE_E5, EIGHTH},
	{NOTE_E5, EIGHTH},
	{REST, EIGHTH},
	{NOTE_E5, EIGHTH},
    {REST, EIGHTH},
	{NOTE_C5, EIGHTH},
	{NOTE_E5, QUARTER},
	{NOTE_G5, QUARTER},
    {REST, QUARTER},
	{NOTE_G4, QUARTER}
};
const int mario_intro_len = ARRAY_LEN(mario_intro);

Song luffy_theme[] = {
    {NOTE_FS4, EIGHTH},
	{NOTE_G4, EIGHTH},
	{NOTE_A4, QUARTER},
	{NOTE_B4, EIGHTH},
    {NOTE_CS5, EIGHTH},
	{NOTE_D5, EIGHTH},
	{NOTE_E5, EIGHTH},
	{NOTE_FS5, EIGHTH},
    {NOTE_G5, EIGHTH},
	{NOTE_A5, QUARTER},
	{NOTE_D6, QUARTER},
	{NOTE_A5, QUARTER},
    {NOTE_G5, EIGHTH},
	{NOTE_A5, SIXTEENTH},
	{NOTE_G5, SIXTEENTH},
	{NOTE_FS5, HALF}
};
const int luffy_theme_len = ARRAY_LEN(luffy_theme);

Song zoro_theme[] = {
    {NOTE_A4, EIGHTH},
	{NOTE_B4, EIGHTH},
	{NOTE_C5, EIGHTH},
	{NOTE_E4, EIGHTH},
    {NOTE_GS4, HALF},
	{NOTE_A4, QUARTER * 3},
	{NOTE_AS4, QUARTER},
	{NOTE_A4, HALF}
};
const int zoro_theme_len = ARRAY_LEN(zoro_theme);

Song smash_splash[] = {
    {NOTE_B4, EIGHTH},		// 1
	{NOTE_FS4, SIXTEENTH},
	{NOTE_G4, SIXTEENTH},
	{NOTE_FS5, EIGHTH * 3},
    {NOTE_B4, EIGHTH},
	{NOTE_FS5, EIGHTH},
	{NOTE_A5, EIGHTH},
	{NOTE_GS5, EIGHTH},		// 2
	{NOTE_E5, EIGHTH},
	{NOTE_B4, HALF},
	{REST, EIGHTH},
	{NOTE_B4, SIXTEENTH},
	{NOTE_CS5, SIXTEENTH},
	{NOTE_D5, HALF},		// 3
	{REST, EIGHTH},
	{NOTE_B4, EIGHTH},
	{NOTE_D5, EIGHTH},
	{NOTE_FS5, EIGHTH},
	{NOTE_E5, QUARTER * 3},	// 4
	{REST, QUARTER },

    {NOTE_B4, EIGHTH},		// 5
	{NOTE_FS4, SIXTEENTH},
	{NOTE_G4, SIXTEENTH},
	{NOTE_FS5, EIGHTH * 3},
    {NOTE_B4, EIGHTH},
	{NOTE_FS5, EIGHTH},
	{NOTE_A5, EIGHTH},
	{NOTE_GS5, EIGHTH},		// 6
	{NOTE_E5, EIGHTH},
	{NOTE_B4, HALF},
	{REST, EIGHTH},
	{NOTE_B4, SIXTEENTH},
	{NOTE_CS5, SIXTEENTH},
	{NOTE_D5, QUARTER},		// 7
	{NOTE_E4, QUARTER},
	{NOTE_G4, QUARTER * 2/3}, // triplet
	{NOTE_D5, QUARTER * 2/3}, // triplet
	{NOTE_B4, QUARTER * 2/3}, // triplet
	{NOTE_D5, HALF},
	{NOTE_E4, SIXTEENTH},
	{NOTE_D4, SIXTEENTH},

	//repeat down octave
    {NOTE_B3, EIGHTH},		// 1
	{NOTE_FS3, SIXTEENTH},
	{NOTE_G3, SIXTEENTH},
	{NOTE_FS4, EIGHTH * 3},
    {NOTE_B3, EIGHTH},
	{NOTE_FS4, EIGHTH},
	{NOTE_A4, EIGHTH},
	{NOTE_GS4, EIGHTH},		// 2
	{NOTE_E4, EIGHTH},
	{NOTE_B3, HALF},
	{REST, EIGHTH},
	{NOTE_B3, SIXTEENTH},
	{NOTE_CS4, SIXTEENTH},
	{NOTE_D4, HALF},		// 3
	{REST, EIGHTH},
	{NOTE_B3, EIGHTH},
	{NOTE_D4, EIGHTH},
	{NOTE_FS4, EIGHTH},
	{NOTE_E4, QUARTER * 3},	// 4
	{REST, QUARTER },

    {NOTE_B3, EIGHTH},		// 5
	{NOTE_FS3, SIXTEENTH},
	{NOTE_G3, SIXTEENTH},
	{NOTE_FS4, EIGHTH * 3},
    {NOTE_B3, EIGHTH},
	{NOTE_FS4, EIGHTH},
	{NOTE_A4, EIGHTH},
	{NOTE_GS4, EIGHTH},		// 6
	{NOTE_E4, EIGHTH},
	{NOTE_B3, HALF},
	{REST, EIGHTH},
	{NOTE_B3, SIXTEENTH},
	{NOTE_CS4, SIXTEENTH},
	{NOTE_D4, QUARTER},		// 7
	{NOTE_E3, QUARTER},
	{NOTE_G3, QUARTER * 2/3}, // triplet
	{NOTE_D4, QUARTER * 2/3}, // triplet
	{NOTE_B3, QUARTER * 2/3}, // triplet
	{NOTE_D4, HALF},
	{NOTE_E5, SIXTEENTH},
	{NOTE_D5, SIXTEENTH},

};
const int smash_splash_len = ARRAY_LEN(smash_splash);


