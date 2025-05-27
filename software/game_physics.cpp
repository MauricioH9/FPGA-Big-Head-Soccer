#include "game_physics.h"
#include <cmath>
#include <cstdlib>

const int SCREEN_W = 640;
const int SCREEN_H = 480;
const int PLAYER_W = 32, PLAYER_H = 32;
const int BALL_W   = 16, BALL_H   = 16;



const float GRAVITY = 0.5;       // Pull down per frame
const float BOUNCE_DAMPING = 0.7; // 70% of velocity kept on bounce
const float FRICTION = 0.98;     // Slow down horizontal velocity

// 40px above bottom is your “invisible line”
const int GROUND_OFFSET    = 40;
const int PLAYER_GROUND_Y  = SCREEN_H - GROUND_OFFSET - PLAYER_H;
const int BALL_GROUND_Y    = SCREEN_H - GROUND_OFFSET - BALL_H;

float p1_vy = 0, p2_vy = 0;
bool  p1_on_ground = true, p2_on_ground = true;
int   p1_x = 50, p1_y = PLAYER_GROUND_Y;
int   p2_x = SCREEN_W - 50 - PLAYER_W, p2_y = PLAYER_GROUND_Y;
int   ball_x = SCREEN_W/2 - BALL_W/2, ball_y = 0;
float ball_vx = 0, ball_vy = 0;
bool  key_state[128] = {false};


extern void play_kick_sound();
extern void play_collision_sound();

void apply_gravity_and_ground() {
    // gravity
    p1_vy += GRAVITY;
    p2_vy += GRAVITY;
    p1_y += int(p1_vy);
    p2_y += int(p2_vy);

    // 40px above‐bottom line
    if (p1_y >= PLAYER_GROUND_Y) {
        p1_y = PLAYER_GROUND_Y;
        p1_vy = 0;
        p1_on_ground = true;
    }
    if (p2_y >= PLAYER_GROUND_Y) {
        p2_y = PLAYER_GROUND_Y;
        p2_vy = 0;
        p2_on_ground = true;
    }
}


void update_ball_motion() {
    ball_vy += GRAVITY;
    ball_x += ball_vx;
    ball_y += ball_vy;

    // bounce off your new ground line
    if (ball_y >= BALL_GROUND_Y) {
        ball_y = BALL_GROUND_Y;
        ball_vy *= -BOUNCE_DAMPING;
        if (std::fabs(ball_vy) < 1.0f) ball_vy = 0;
    }
    // Wall bounce
    if (ball_x <= 0) {
        ball_x = 0;
        ball_vx *= -BOUNCE_DAMPING;
    } else if (ball_x + BALL_W >= 640) {
        ball_x = 640 - BALL_W;
        ball_vx *= -BOUNCE_DAMPING;
    }

    // Friction
    ball_vx *= FRICTION;
}

// in game_physics.cpp or wherever your collision lives:
void handle_player_ball_collision(int p_x, int p_y, bool is_kicking,
                                  unsigned long now, unsigned long& last_collision_time,
                                  int player_id)
{
    // --- 1) compute centers & radii (same as before) ---
    float cx_p = p_x + PLAYER_W/2.0f;
    float cy_p = p_y + PLAYER_H/2.0f;
    float cx_b = ball_x + BALL_W/2.0f;
    float cy_b = ball_y + BALL_H/2.0f;

    float r_p = (PLAYER_W/2.0f) * 0.8f;
    float r_b = BALL_W/2.0f;

    float dx = cx_b - cx_p;
    float dy = cy_b - cy_p;
    float dist2 = dx*dx + dy*dy;
    float radSum = r_p + r_b;

    // --- 2) circle‐circle overlap test ---
    if (dist2 < radSum*radSum) {
        float dist = std::sqrt(dist2);
        float nx = (dist > 0.0f ? dx/dist : 1.0f);
        float ny = (dist > 0.0f ? dy/dist : 0.0f);

        // --- 3) resolve penetration so ball can't tunnel through ---
        float overlap = radSum - dist;
        cx_b += nx * overlap;
        cy_b += ny * overlap;
        ball_x = int(cx_b - BALL_W/2.0f);
        ball_y = int(cy_b - BALL_H/2.0f);

        // --- 4) decide kick vs. bounce ---
        float dir = (nx >= 0) ? 1.0f : -1.0f;

        // only allow a kick if:
        //  - key is down
        //  - AND for P1: dx >= 0  (ball on their right)
        //    for P2: dx <= 0  (ball on their left)
        bool inner_hit = (player_id == 1 && dx >= 0) ||
                         (player_id == 2 && dx <= 0);

        if (is_kicking && inner_hit) {
            // strong, inner‐side kick
            ball_vx = 15.0f * dir + ((rand() % 3) - 1);
            ball_vy = -6.0f;
            play_kick_sound();
        }
        else if (now - last_collision_time > 200) {
            // normal bounce when not a valid kick
            if (ball_y < p_y + 5) {
                ball_vx = 3.0f * dir;
                ball_vy = -6.0f;
            } else {
                ball_vx = 2.0f * dir;
                ball_vy = -2.5f;
            }
            play_collision_sound();
            last_collision_time = now;
        }
    }
}
