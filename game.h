#ifndef _GAME_H_
#define _GAME_H_

#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
#define OS_WIN
#endif

#include <math.h>       /* atan2, cos, pow, sin, sqrt */ 
#include <stdio.h>      /* fprintf, sprintf */
#include <stdbool.h>    /* bool, true, false */
#ifdef OS_WIN
#include <windows.h>    /* sleep */
#else
#include <unistd.h>     /* sleep */
#endif

#include "raylib.h"
#include "vec.h"

#include "graphics.h"
#include "timer.h"

typedef enum {
    /* invalid */
    E_NONE = -1,

    /* enemy types */

    E_ENEMY_BASIC,
    E_ENEMY_LARGE,

    /* projectile types */

    E_PLAYER_BULLET,

    /* how many types there are, excluding player */
    E_COUNT,

    E_PLAYER,

} EntityType;

typedef struct {
    EntityType type;
    float x, y;
    float size;
    float speed, angle;
    int max_hp, hp, contact_damage;
    bool invincible;
} Entity;

typedef enum {
    GS_TITLE,
    GS_GAMEPLAY,
    GS_PAUSED,
    GS_GAMEOVER
} GameState;

typedef vec_t(Entity) EntityVec;

typedef struct {
    // in seconds
    float spawn_interval;
    // % border around the screen (1.20 = 120%), used if it spawns randomly
    float spawn_margin;
    // what it spawns
    EntityType subspawn_type;
    // how often, in seconds
    float subspawn_interval;
    // just for the player (sec)
    float invincibility_time;
    // speed in pixels per frame
    float speed;
    // draw size
    float size;
    int max_hp;
    int contact_damage;
} EntityAttrs;

typedef struct {
    Timer basic_enemy_spawn, player_invinc, player_fire_bullet;
} GameTimers;

typedef struct {
    
    struct {
        Vector2 window_init_dim;
        bool window_initialized;
        int target_fps;
        /* in % of the screen size, anything outside of here is considered offscreen. enemies spawn here. */
        float screen_margin;
        /* +2 for player */
        EntityAttrs entitydata[E_COUNT+2];
    } config;

    struct {
        Texture2D background;
    } textures;
    
    GameTimers timers;

    struct {
        Button start_btn, restart_btn;
        float gametime;
    } ui;

    GameState state;
    Camera2D camera;
    Entity player;

    /* indexed by type */
    EntityVec entities[E_COUNT];
} Game;

/* game methods */

void InitGame(void);
void RunGame(void);
void ReinitGame(void);
void DestroyGame(void);

void SetState(GameState s);

void HandleInput(void);
void UpdateCam(void);

void UpdateGameTime(void);

/* gamestate draw functions */

void DrawTitle(void);
void DrawGameplay(void);
void DrawPaused(void);
void DrawGameover(void);

/* draw functions */

void TileBackground(void);

void DrawPlayer(bool sprite_flickering);
void DamagePlayer(int amount);

void DrawBasicEnemy(Entity*);
void UpdateBasicEnemy(Entity*);

void DrawBullet(Entity*);
void UpdateBullet(Entity*);
void CollideBullets(void);

/* game ui elements */

void DrawGameUI(void);

void DisplayPlayerHP(void);
void DisplayGameTime(void);

/* entity methods */

Rectangle EntityHitbox(Entity);

void ManageEntities(bool draw, bool update);

Entity RandSpawnEnemy(void);
Entity PlayerFireBullet(void);

void MoveEntityToPlayer(Entity*);

/* general utils */

Vector2 screensize(void);
Vector2 screen_offset(void);

int randrange(int min, int max);
float randfloat(float min, float max);
float cointoss(float a, float b);

float distance(Vector2, Vector2);

bool is_collision(Entity, Entity);
bool entity_offscreen(Entity);

float entity_distance(Entity, Entity);
float entity_angle(Entity, Entity);
Entity *player_closest_entity(EntityType);

/* callbacks */

void StartBtnCallback(void);
void RestartBtnCallback(void);

void EnemySpawnTimerCallback(void);
void PlayerInvincTimerCallback(void);
void PlayerBulletTimerCallback(void);

void DoNothingCallback(void);

#endif /* _GAME_H_ */
