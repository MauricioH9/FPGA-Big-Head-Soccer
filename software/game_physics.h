// game_physics.h
#ifndef PHYSICS_H
#define PHYSICS_H

#include <cmath>
#include "chu_init.h"

extern const int SCREEN_H;
extern const int SCREEN_W;
extern const int PLAYER_H;
extern const int PLAYER_W;
extern const int BALL_W;
extern const int BALL_H;
extern const float GRAVITY;
extern const float FRICTION;
extern const float BOUNCE_DAMPING;
extern const float GROUND_Y;

extern const int GROUND_OFFSET;
extern const int PLAYER_GROUND_Y;
extern const int BALL_GROUND_Y;

extern int p1_y, p2_y;
extern int p1_x, p2_x;
extern float p1_vy, p2_vy;
extern bool p1_on_ground, p2_on_ground;

extern int ball_x, ball_y;
extern float ball_vx, ball_vy;

extern bool key_state[128];

void apply_gravity_and_ground();
void update_ball_motion();
void handle_player_ball_collision(int p_x, int p_y, bool is_kicking, unsigned long now, unsigned long &last_collision_time, int player_id);

#endif
