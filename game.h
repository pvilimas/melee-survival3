#ifndef _GAME_H_
#define _GAME_H_

#include <math.h>       /* atan2, cos, pow, sin, sqrt */ 
#include <stdarg.h>     /* va_list */
#include <stdbool.h>    /* bool, true, false */
#include <stdio.h>      /* fprintf, sprintf */
#include <time.h>       /* time */
#include <unistd.h>     /* sleep */

#include "raylib.h"
#include "vec.h"

#include "graphics.h"
#include "timer.h"

typedef enum {
    /* invalid */
    E_NONE = -1,

    /* enemy types */

    E_ENEMY_BASIC = 0,
    E_ENEMY_LARGE,

    /* projectile types */

    E_PLAYER_BULLET,
    E_PLAYER_SHELL,

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
    // for explosions
    bool spawned_particle;
} Entity;

typedef enum {
    P_NONE = -1,
    P_EXPLOSION = 0,
    P_ENEMY_FADEOUT_BASIC,
    P_ENEMY_FADEOUT_LARGE,
    /* how many there are */
    P_COUNT,
} ParticleType;

typedef struct {
    ParticleType type;
    float x, y;
    // this can change
    float size;
    // this cannot
    float starting_size;
    // how many frames it's drawn for
    int lifetime;
    // which frame it's on
    int currframe;
    int damage;
} Particle;

typedef enum {
    GS_TITLE,
    GS_GAMEPLAY,
    GS_PAUSED,
    GS_GAMEOVER
} GameState;

typedef vec_t(Entity) EntityVec;
typedef vec_t(Particle) ParticleVec;

typedef void (*EDrawFunc)(Entity*);
typedef void (*EUpdateFunc)(Entity*);

// TODO: spawn and despawn?
typedef void (*PDrawFunc)(Particle*, bool advance_frame);

typedef struct {
    // in seconds
    float spawn_interval;
    // how often it spawns the thing, in seconds (0 = does not spawn it)
    float child_spawns[E_COUNT];
    // just for the player (sec)
    float invincibility_time;
    // speed in pixels per frame
    float speed;
    // draw size
    float size;
    // unused for now
    float explosion_radius;
    int max_hp;
    int contact_damage;
    EDrawFunc draw;
    EUpdateFunc update;
} EntityAttrs;

typedef struct {
    float starting_size;
    int lifetime;
    int damage;
    PDrawFunc draw;
} ParticleAttrs;

typedef struct {
    Timer basic_enemy_spawn,
        large_enemy_spawn,
        player_invinc,
        player_fire_bullet,
        player_fire_shell;
} GameTimers;

typedef struct {
    bool loaded;
    const char *filepath;
    Texture2D data;
} GameTexture;

typedef struct {
    struct {
        Vector2 window_init_dim;
        bool window_initialized;
        const char *window_title;
        int target_fps;
        float animation_frametime;
        /* in % of the screen size, anything outside of here is considered offscreen. enemies spawn here. [0] is the inner bound, [1] is outer */
        float screen_margin[2];
        /* +2 for player */
        EntityAttrs entitydata[E_COUNT+2];
        ParticleAttrs particledata[P_COUNT];
        EntityType enemy_types[2];
        EntityType projectile_types[2];
    } config;
    struct {
        GameTexture background;
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
    /* now indexed as well */
    ParticleVec particles[P_COUNT];
} Game;

/* game methods */

void InitGame(void);
void RunGame(void);
void ReinitGame(void);
void DestroyGame(void);

void SetState(GameState);

void HandleInput(void);
void CheckTimers(void);
void UpdateCam(void);

void UpdateGameTime(void);

void InitTexture(GameTexture*);
void DeinitTexture(GameTexture*);

void InitGameTimers(void);

void GameSleep(float secs);

/* gamestate draw functions */

void DrawTitle(void);
void DrawGameplay(void);
void DrawPaused(void);
void DrawGameover(void);

/* draw functions */

void TileBackground(void);

void DrawEntityHitbox(Entity*);

void DrawPlayer(bool sprite_flickering);
void DamagePlayer(int amount);
void HealPlayer(int amount);

/* generics */

void DrawEntity(Entity*);
void UpdateEntity(Entity*);

void DrawBasicEnemy(Entity*);
void UpdateBasicEnemy(Entity*);

void DrawLargeEnemy(Entity*);
void UpdateLargeEnemy(Entity*);

void DrawProjectile(Entity*);
void UpdateProjectile(Entity*);

void DrawBullet(Entity*);
void DrawShell(Entity*);

/* game ui elements */

void DrawGameUI(void);

void DisplayPlayerHP(void);
void DisplayGameTime(void);

/* entity methods */

Rectangle EntityHitbox(Entity);

void DrawEntities(bool update);
void DrawParticles(bool update);

Entity RandSpawnEnemy(EntityType);
Entity PlayerFireBullet(void);
Entity PlayerFireShell(void);

void MoveEntityToPlayer(Entity*);

/* particle methods */

Rectangle ParticleHitbox(Particle);
void DrawParticleHitbox(Particle*);

void SpawnParticle(ParticleType, float x, float y);
Particle NewParticle(ParticleType, float x, float y);
bool ParticleDone(Particle);

/* generics */

void DrawParticle(Particle*, bool advance_frame);

void SpawnPExplosion(float x, float y);
void SpawnPEnemyFadeout(Entity*);

void DrawPExplosion(Particle*, bool advance_frame);
void DrawPEnemyFadeout(Particle*, bool advance_frame);


/* general utils */

Vector2 screensize(void);
Vector2 screen_offset(void);

int randrange(int min, int max);
float randfloat(float min, float max);
float randchoice(size_t count, float *probs, ...);

float distance(Vector2, Vector2);

bool is_collision(Entity, Entity);
bool is_p_collision(Particle, Entity);
bool entity_offscreen(Entity);

float entity_distance(Entity, Entity);
float entity_angle(Entity, Entity);
float vec_angle(Vector2, Vector2);
Entity *player_closest_entity(EntityType);
Entity *player_closest_enemy(void);

/* callbacks */

void StartBtnCallback(void);
void RestartBtnCallback(void);

void BasicEnemySpawnTimerCallback(void);
void LargeEnemySpawnTimerCallback(void);
void PlayerInvincTimerCallback(void);
void PlayerBulletTimerCallback(void);
void PlayerShellTimerCallback(void);

void DoNothingCallback(void);

#endif /* _GAME_H_ */
