#ifndef _GAME_H_
#define _GAME_H_

#include <math.h>       /* pow, sqrt */ 
#include <stdio.h>      /* fprintf */
#include <stdbool.h>    /* bool */
#include <unistd.h>     /* sleep */ 

#include "raylib.h"
#include "vec.h"

#include "graphics.h"
#include "timer.h"

typedef enum {
    /* invalid */
    E_NONE = -1,

    /* enemy types */

    E_ENEMY_BASIC,

    /* projectile types */

    E_PLAYER_BULLET,

    /* how many types there are, excluding player */
    E_COUNT,

    E_PLAYER,

} EntityType;

typedef struct {
    EntityType type;
    float x, y;
    float speed, angle;
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
    double spawn_interval;
    // % border around the screen (1.20 = 120%), used if it spawns randomly
    double spawn_margin;
    // what it spawns
    EntityType subspawn_type;
    // how often, in seconds
    double subspawn_interval;
    // just for the player (sec)
    double invincibility_time;
    // speed in pixels per frame
    double speed;
    int max_hp;
    int contact_damage;
} EntityAttrs;

typedef struct {
    
    struct {
        Vector2 window_init_dim;
        bool window_initialized;
        int target_fps;
        /* in % of the screen size */
        float randspawn_margin;
        EntityAttrs playerdata;
        EntityAttrs entitydata[E_COUNT];
    } config;

    struct {
        Texture2D background;
    } textures;
    
    struct {
        Timer enemy_spawn, player_invinc, player_fire_bullet;
    } timers;

    struct {
        Button start_btn, test_btn;
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
void DestroyGame(void);

/* contains initialization logic for each state */
void SetState(GameState s);

void HandleInput(void);
void UpdateCam(void);

/* gamestate draw functions */

void DrawTitle(void);
void DrawGameplay(void);
void DrawPaused(void);
void DrawGameover(void);

/* draw functions */

void TileBackground(void);

void DrawPlayer(void);

void DrawEnemy(Entity*);
void UpdateEnemy(Entity*);

void DrawBullet(Entity*);
void UpdateBullet(Entity*);
void CollideBullets(void);

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
void DoNothingCallback(void);

void EnemySpawnTimerCallback(void);
void PlayerInvincTimerCallback(void);
void PlayerBulletTimerCallback(void);

#endif
