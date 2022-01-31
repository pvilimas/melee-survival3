#include "game.h"

extern ScreenSizeFunc GraphicsGetScreenSize;
extern ScreenOffsetFunc GraphicsGetScreenOffset;

/* entity attribute access */
#define getattr(etype, attr) (game.config.entitydata[etype].attr)
// 50/50 chance
#define cointoss(a, b) ((rand() % 2) ? a : b)

/*
    TODO:

    - ParticleAttrs (for cleanup)
    - enemy fadeout particle

    - add 2 more types of enemies (4 in total) and spawn all of them randomly
    - then make it scale over time
    
    - make sure DestroyGame works properly w/ no mem leaks
    - impl GameSleep with GetTime for cross platform (nvm just alias sleep again - #ifdef WIN32)

    - "you lasted <time>" on the end screen - improve it, not just in the corner
    - add a couple more projectile types

    - fix the inputs so A+D won't freeze, but whichever was pressed first takes precedence
    - upgrade tree?
    - keymaps?
*/

bool show_hitboxes = false;

Game game = {
    .config = {
        .window_initialized = false,
        .window_init_dim = (Vector2) { 800, 500 },
        .window_title = "Melee Survival",
        .target_fps = 60,
        .animation_frametime = 1.0 / 60,
        .screen_margin = { 1.2, 1.5 },
        .entitydata = {
            [E_PLAYER] = {
                .child_spawns = {
                    [E_PLAYER_BULLET] = 1.0f,
                    [E_PLAYER_SHELL] = 4.0f,
                },
                .max_hp = 100,
                .invincibility_time = 1.0,
                .contact_damage = 0,
            },
            [E_ENEMY_BASIC] = {
                .spawn_interval = 0.5,
                .speed = 1.0,
                .size = 6,
                .max_hp = 100,
                .contact_damage = 10,
            },
            [E_ENEMY_LARGE] = {
                .spawn_interval = 5,
                .speed = 0.3,
                .size = 30,
                .max_hp = 500,
                .contact_damage = 40,
            },
            [E_PLAYER_BULLET] = {
                .spawn_interval = 1.0,
                .speed = 4.0,
                .size = 4.0,
                .contact_damage = 100,
            },
            [E_PLAYER_SHELL] = {
                .spawn_interval = 4.0,
                .speed = 6.0,
                .size = 8.0,
                .contact_damage = 200,
                .explosion_radius = 10.0,
            }
        },
    },
    .ui = {
        .gametime = 0,
        .start_btn = (Button){
            38, 55, 24, 10, "Start", StartBtnCallback,
        },
        .restart_btn = (Button){
            38, 55, 24, 10, "Try again?", RestartBtnCallback
        }
    },
    .textures = {
        /* TODO: macro to initialize this from just a path? */
        .background = {
            .filepath = "./assets/background/bg_hextiles.png",
            .data = NULL,
        },
    },
};


/* game methods */

void InitGame(void) {

    srand(time(NULL));
    
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(screensize().x, screensize().y, game.config.window_title);
    SetTargetFPS(game.config.target_fps);
    game.config.window_initialized = true;
    
    GraphicsGetScreenOffset = screen_offset;
    GraphicsGetScreenSize = screensize;

    for (int i = 0; i < E_COUNT; i++) {
        vec_init(&game.entities[i]);
    }
    vec_init(&game.particles);

    InitTexture(&game.textures.background);

    game.state = GS_TITLE;
    InitGameTimers();
    game.camera = (Camera2D){
        .target = (Vector2){ 0, 0 },
        .zoom = 1.0f
    };
    game.player = (Entity){
        .x = screensize().x / 2,
        .y = screensize().y / 2,
        .speed = 3,
        .size = 6,
        .max_hp = getattr(E_PLAYER, max_hp),
        .hp = getattr(E_PLAYER, max_hp),
        .invincible = false,
    };
}

void RunGame(void) {
    
    while (!WindowShouldClose()) {
        /* FPS control */
        static const double frametime = 1.0 / 60.0;
        double time = GetTime();
        BeginDrawing();
        switch(game.state) {
            case GS_TITLE: {
                DrawTitle();
                break;
            }
            case GS_GAMEPLAY: {
                DrawGameplay();
                if (game.player.hp <= 0) {
                    SetState(GS_GAMEOVER);
                }
                break;
            }
            case GS_PAUSED: {
                DrawPaused();
                break;
            }
            case GS_GAMEOVER: {
                DrawGameover();
                break;
            }
            default: break;
        }

        EndDrawing();

        /* FPS control */
        GameSleep(frametime - (GetTime() - time));
    }
}

/* for restarting */
void ReinitGame(void) {

    for (int i = 0; i < E_COUNT; i++) {
        vec_clear(&game.entities[i]);
    }
    vec_clear(&game.particles);

    game.state = GS_TITLE;
    InitGameTimers();
    game.camera = (Camera2D){
        .target = (Vector2){ 0, 0 },
        .zoom = 1.0f
    };
    game.player = (Entity){
        .x = screensize().x / 2,
        .y = screensize().y / 2,
        .speed = 3,
        .size = 6,
        .max_hp = getattr(E_PLAYER, max_hp),
        .hp = getattr(E_PLAYER, max_hp),
        .invincible = false,
    };
    game.ui.gametime = 0;
}

void DestroyGame(void) {
    CloseWindow();
    game.config.window_initialized = false;
    for (int i = 0; i < E_COUNT; i++) {
        vec_deinit(&game.entities[i]);
    }
    vec_deinit(&game.particles);
    DeinitTexture(&game.textures.background);
}

/* contains initialization logic for each state */
void SetState(GameState new_state) {
    GameState old_state = game.state;
    
    /* if starting over */
    if (old_state == GS_GAMEOVER && new_state == GS_GAMEPLAY) {
        ReinitGame();
    }
    /* if player is dead */
    else if (old_state == GS_GAMEPLAY && new_state == GS_GAMEOVER) {
        game.player.x = screensize().x / 2;
        game.player.y = screensize().y / 2;
        UpdateCam();
    }

    game.state = new_state;
}

void HandleInput(void) {
    Entity *player = &game.player;
    float slow_speed = player->speed / (2 * sqrt(2.0));
    float fast_speed = player->speed;

    // for debugging
    if (IsKeyPressed(KEY_K)) {
        //game.ui.gametime += 260000;
        game.player.hp = 0;
        return;
    }

    if (IsKeyPressed(KEY_P)) {
        if (game.state == GS_GAMEPLAY) {
            SetState(GS_PAUSED);
            return;
        } else if (game.state == GS_PAUSED) {
            SetState(GS_GAMEPLAY);
            return;
        }
    }

    /* keep this here i forget why it's needed */
    if (game.state != GS_GAMEPLAY) {
        return;
    }

    /* movement */

    // invalid input?
    if (!((IsKeyDown(KEY_W) && IsKeyDown(KEY_S)) || (IsKeyDown(KEY_A) && IsKeyDown(KEY_D)))) {

        if (IsKeyDown(KEY_W)) {
            if (IsKeyDown(KEY_A)) {
                player->x -= slow_speed;
                player->y -= slow_speed;
            } else if (IsKeyDown(KEY_D)) {
                player->x += slow_speed;
                player->y -= slow_speed;
            } else {
                player->y -= fast_speed;
            }
        }
        if (IsKeyDown(KEY_A)) {
            if (IsKeyDown(KEY_W)) {
                player->x -= slow_speed;
                player->y -= slow_speed;
            } else if (IsKeyDown(KEY_S)) {
                player->x -= slow_speed;
                player->y += slow_speed;
            } else {
                player->x -= fast_speed;
            }
        }
        if (IsKeyDown(KEY_S)) {
            if (IsKeyDown(KEY_A)) {
                player->x -= slow_speed;
                player->y += slow_speed;
            } else if (IsKeyDown(KEY_D)) {
                player->x += slow_speed;
                player->y += slow_speed;
            } else {
                player->y += fast_speed;
            }
        }
        if (IsKeyDown(KEY_D)) {
            if (IsKeyDown(KEY_W)) {
                player->x += slow_speed;
                player->y -= slow_speed;
            } else if (IsKeyDown(KEY_S)) {
                player->x += slow_speed;
                player->y += slow_speed;
            } else {
                player->x += fast_speed;
            }
        }
    }
}

void UpdateCam(void) {
    game.camera.offset = (Vector2) { GetScreenWidth()/2, GetScreenHeight()/2 };
    game.camera.target = (Vector2){ game.player.x, game.player.y };
}

void UpdateGameTime(void) {
    game.ui.gametime += 1.0 / game.config.target_fps;
}

void InitTexture(GameTexture* t) {
    Image temp = LoadImage(t->filepath);
    t->data = LoadTextureFromImage(temp);
    UnloadImage(temp);
    t->loaded = true;
}

void DeinitTexture(GameTexture* t) {
    UnloadTexture(t->data);
    t->data = (Texture2D){0};
    t->loaded = false;
}

void InitGameTimers(void) {
    game.timers = (GameTimers){
        .basic_enemy_spawn = NewTimer(getattr(E_ENEMY_BASIC, spawn_interval), BasicEnemySpawnTimerCallback),
        .large_enemy_spawn = NewTimer(getattr(E_ENEMY_LARGE, spawn_interval), LargeEnemySpawnTimerCallback),
        .player_invinc = NewTimer(getattr(E_PLAYER, invincibility_time), PlayerInvincTimerCallback),
        .player_fire_bullet = NewTimer(getattr(E_PLAYER, child_spawns)[E_PLAYER_BULLET], PlayerBulletTimerCallback),
        .player_fire_shell = NewTimer(getattr(E_PLAYER, child_spawns)[E_PLAYER_SHELL], PlayerShellTimerCallback),
    };
}

void GameSleep(float secs) {
    sleep(secs);
}

/* gamestate draw functions */

void DrawTitle(void) {
    ClearBackground((Color){170, 170, 170, 255});
    DrawTextUI("Melee Survival", 50, 45, 50, BLACK);
    DrawButton(game.ui.start_btn);
}

void DrawGameplay(void) {
    BeginMode2D(game.camera);

    CheckTimer(&game.timers.player_invinc);
    CheckTimer(&game.timers.player_fire_bullet);
    CheckTimer(&game.timers.player_fire_shell);
    CheckTimer(&game.timers.basic_enemy_spawn);
    CheckTimer(&game.timers.large_enemy_spawn);

    /* don't mess with this order */
    TileBackground();
    DrawGameUI();
    DrawPlayer(true);
    HandleInput();
    UpdateCam();
    ManageEntities(true, true);
    ManageParticles(true, true);
    
    EndMode2D();
    UpdateGameTime();
}

void DrawPaused(void) {

    BeginMode2D(game.camera);

    TileBackground();
    DrawGameUI();
    DrawPlayer(false);
    HandleInput();
    UpdateCam();
    ManageEntities(true, false);
    ManageParticles(true, false);

    Vector2 offset = GraphicsGetScreenOffset();
    Vector2 size = GraphicsGetScreenSize();

    float x = offset.x;
    float y = offset.y;
    float w = size.x;
    float h = size.y;

    DrawRectangle(x, y, w, h, (Color){180, 180, 180, 180});
    DrawTextUI("PAUSED", 50, 50, 50, BLACK);

    EndMode2D();
    
}

void DrawGameover(void) {

    BeginMode2D(game.camera);

    ClearBackground((Color){170, 170, 170, 255});
    DrawButton(game.ui.restart_btn);
    DrawTextUI("You Died", 50, 45, 50, BLACK);
    DisplayGameTime();

    EndMode2D();

}

/* draw functions */

/* please don't mess with this please :) */
void TileBackground(void) {
    for (int i = ((game.player.x - GetScreenWidth()/2) / game.textures.background.data.width) - 1; i < ((game.player.x + GetScreenWidth()/2) / game.textures.background.data.width) + 1; i++) {
        for (int j = ((game.player.y - GetScreenHeight()/2) / game.textures.background.data.height) - 1; j < ((game.player.y + GetScreenHeight()/2) / game.textures.background.data.height) + 1; j++) {
            DrawTexture(
                game.textures.background.data,
                i * game.textures.background.data.width,
                j * game.textures.background.data.height,
                (Color){255, 255, 255, 255}
            );
        }
    }
}

void DrawEntityHitbox(Entity *e) {
    DrawRectangleLinesEx(EntityHitbox(*e), 1.0, BLACK);
}

void DrawPlayer(bool sprite_flickering) {
    // sprite flickering
    if (((game.player.invincible && (int)(GetTime() * 10000) % 200 >= 100) || !game.player.invincible) || !sprite_flickering) {
        DrawCircle(game.player.x, game.player.y, game.player.size, BLACK);
        DrawCircle(game.player.x, game.player.y, game.player.size * 3/4, (Color){60, 60, 60, 255});
    }
}

void DamagePlayer(int amount) {
    if (!game.player.invincible) {
        game.player.hp -= amount;
        game.player.invincible = true;
    }
}

void DrawBasicEnemy(Entity *enemy) {
    DrawCircle(enemy->x, enemy->y, enemy->size, MAROON);
    DrawCircle(enemy->x, enemy->y, enemy->size * 2/3, RED);
}

void UpdateBasicEnemy(Entity *enemy) {
    MoveEntityToPlayer(enemy);
}

void DrawLargeEnemy(Entity *enemy) {
    DrawCircle(enemy->x, enemy->y, enemy->size, BLACK);
    DrawCircle(enemy->x, enemy->y, enemy->size * 2/3, MAROON);
}

void UpdateLargeEnemy(Entity *enemy) {
    MoveEntityToPlayer(enemy);
}

void DrawProjectile(Entity *proj) {
    switch (proj->type) {
        case E_PLAYER_BULLET:   return DrawBullet(proj);
        case E_PLAYER_SHELL:    return DrawShell(proj);
        default:                return;
    }
}

/* for projectiles that travel in straight lines */
void UpdateProjectile(Entity *proj) {
    proj->x += proj->speed * cos(proj->angle);
    proj->y += proj->speed * sin(proj->angle);
}

void DrawBullet(Entity *bullet) {
    DrawLineEx(
        (Vector2){bullet->x - bullet->size * cos(bullet->angle), bullet->y - bullet->size * sin(bullet->angle)},
        (Vector2){bullet->x + bullet->size * cos(bullet->angle), bullet->y + bullet->size * sin(bullet->angle)},
        4.0f, DARKGRAY
    );
    DrawLineEx(
        (Vector2){bullet->x - bullet->size * 3/4 * cos(bullet->angle), bullet->y - bullet->size * 3/4 * sin(bullet->angle)},
        (Vector2){bullet->x + bullet->size * 3/4 * cos(bullet->angle), bullet->y + bullet->size * 3/4 * sin(bullet->angle)},
        2.0f, BLACK
    );
}

void DrawShell(Entity *shell) {
    DrawCircle(shell->x, shell->y, shell->size, ORANGE);
    DrawCircle(shell->x, shell->y, shell->size * 5/6, RED);
}


/* game ui elements */

void DrawGameUI(void) {
    DisplayPlayerHP();
    DisplayGameTime();
}

void DisplayPlayerHP(void) {
    static Color gradient[41] = {
        (Color){ 219, 11, 11, 255 },
        (Color){ 218, 20, 12, 255 },
        (Color){ 216, 28, 13, 255 },
        (Color){ 215, 37, 14, 255 },
        (Color){ 213, 46, 15, 255 },
        (Color){ 212, 55, 16, 255 },
        (Color){ 211, 64, 17, 255 },
        (Color){ 209, 72, 18, 255 },
        (Color){ 208, 81, 19, 255 },
        (Color){ 206, 90, 20, 255 },
        (Color){ 205, 98, 21, 255 },
        (Color){ 204, 107, 22, 255 },
        (Color){ 202, 116, 23, 255 },
        (Color){ 201, 125, 24, 255 },
        (Color){ 199, 134, 25, 255 },
        (Color){ 198, 142, 26, 255 },
        (Color){ 197, 151, 27, 255 },
        (Color){ 195, 160, 28, 255 },
        (Color){ 194, 168, 29, 255 },
        (Color){ 192, 177, 30, 255 },
        (Color){ 191, 186, 31, 255 },
        (Color){ 191, 186, 31, 255 },
        (Color){ 182, 184, 29, 255 },
        (Color){ 173, 182, 28, 255 },
        (Color){ 165, 180, 26, 255 },
        (Color){ 156, 178, 25, 255 },
        (Color){ 147, 177, 23, 255 },
        (Color){ 138, 175, 22, 255 },
        (Color){ 129, 173, 20, 255 },
        (Color){ 121, 171, 19, 255 },
        (Color){ 112, 169, 17, 255 },
        (Color){ 103, 167, 16, 255 },
        (Color){ 94, 165, 14, 255 },
        (Color){ 85, 163, 12, 255 },
        (Color){ 77, 161, 11, 255 },
        (Color){ 68, 159, 9, 255 },
        (Color){ 59, 158, 8, 255 },
        (Color){ 50, 156, 6, 255 },
        (Color){ 41, 154, 5, 255 },
        (Color){ 33, 152, 3, 255 },
        (Color){ 24, 150, 2, 255 },
    };
    int proportion = (int) (40 * game.player.hp / game.player.max_hp);
    int width = 1;
    DrawRectangle(game.player.x - 21, game.player.y - 26, 42, 7, BLACK);
    DrawRectangle(game.player.x - 20, game.player.y - 25, width * proportion, 5, gradient[proportion]);
}

void DisplayGameTime(void) {
    int secs = (int) game.ui.gametime;
    int h = secs / 3600;
    int m = (secs / 60) % 60;
    int s = secs % 60;
    char time_str[30];
    if (h > 0) {
        sprintf(time_str, "%02d:%02d:%02d", h, m, s);
        DrawTextUI(time_str, 94, 5, 20, BLACK);
    } else {
        sprintf(time_str, "%02d:%02d", m, s);
        DrawTextUI(time_str, 95, 5, 20, BLACK);
    }
}

/* entity methods */

Rectangle EntityHitbox(Entity e) {
    switch (e.type) {
        case E_ENEMY_BASIC:     return (Rectangle){ e.x-3, e.y-3, 6.0, 6.0 };
        case E_ENEMY_LARGE:     return (Rectangle){ e.x-15, e.y-15, 30.0, 30.0};
        case E_PLAYER_BULLET:   return (Rectangle){ e.x-2, e.y-2, 4.0, 4.0 };
        case E_PLAYER:          return (Rectangle){ e.x-4, e.y-4, 8.0, 8.0 };
        default:                return (Rectangle){ e.x-e.size/2, e.y-e.size/2, e.size, e.size };
    }
}

void ManageEntities(bool draw, bool update) {

    if (!draw && !update) {
        return;
    }

    EntityVec *entlist, *targetlist;
    Entity *e, *target;

    // for each type of entity
    for (int etype = 0; etype < E_COUNT; etype++) {
        entlist = &game.entities[etype];
        if (vec_empty(entlist))
            continue;

        // for each entity
        for (int i = 0; i < entlist->length; i++) {
            e = &entlist->data[i];

            switch (etype) {
                
                /* draw projectiles first, then the enemies they hit */
                case E_PLAYER_BULLET: 
                case E_PLAYER_SHELL: {
                    e = &entlist->data[i];
                    if (entity_offscreen(*e)) {
                        if (update) {
                            vec_remove(entlist, i);
                            i--;
                        }
                    } else {

                        
                        if (update) {

                            // check for any collisions with enemies
                            targetlist = &game.entities[E_ENEMY_BASIC];
                            for (int j = 0; j < targetlist->length; j++) {
                                target = &targetlist->data[j];
                                if (is_collision(*e, *target)) {
                                    target->hp -= e->contact_damage;
                                    
                                    /* only shells spawn explosions */
                                    if (etype == E_PLAYER_SHELL) {
                                        SpawnParticle(P_EXPLOSION, e->x, e->y);
                                    }
                                    
                                    if (target->hp <= 0) {
                                        // remove target
                                        vec_remove(targetlist, j);
                                        j--;
                                    }
                                    
                                    // remove projectile
                                    vec_remove(entlist, i);
                                    i--;


                                    continue;
                                }
                            }
                            UpdateProjectile(e);
                        
                            targetlist = &game.entities[E_ENEMY_LARGE];
                            for (int j = 0; j < targetlist->length; j++) {
                                target = &targetlist->data[j];
                                if (is_collision(*e, *target)) {
                                    target->hp -= e->contact_damage;

                                    /* only shells spawn explosions */
                                    if (etype == E_PLAYER_SHELL) {
                                        SpawnParticle(P_EXPLOSION, e->x, e->y);
                                    }

                                    if (target->hp <= 0) {
                                        // remove target
                                        vec_remove(targetlist, j);
                                        j--;
                                    }
                                    
                                    // remove projectile
                                    vec_remove(entlist, i);
                                    i--;

                                    continue;
                                }
                            }
                            UpdateProjectile(e);
                            
                        }

                    }

                    if (draw) {
                        DrawProjectile(e);
                    }
                }

                break;
                
                case E_ENEMY_BASIC: {
                    
                    if (update) {
                        // if two enemies have the "same" xy pos, remove one
                        targetlist = &game.entities[E_ENEMY_BASIC];
                        if (vec_empty(targetlist)) {
                            break;
                        }
                        for (int j = 0; j < targetlist->length; j++) {
                            target = &targetlist->data[j];
                            
                            if (i == j)
                                continue;

                            /* optimization - enemy merging - keep it like this */
                            if (entity_distance(*e, *target) < 0.5) {
                                // remove other
                                vec_remove(targetlist, j);
                                j--;
                            }

                            if (is_collision(game.player, *e)) {
                                DamagePlayer(e->contact_damage);
                            }
                        }
                        UpdateBasicEnemy(e);
                    }

                    if (draw) {
                        DrawBasicEnemy(e);
                        if (show_hitboxes) {
                            DrawEntityHitbox(e);
                        }
                    }

                    break;
                }
                case E_ENEMY_LARGE: {
                    
                    if (update) {
                        // if two enemies have the "same" xy pos, remove one
                        targetlist = &game.entities[E_ENEMY_LARGE];
                        if (vec_empty(targetlist)) {
                            break;
                        }
                        for (int j = 0; j < targetlist->length; j++) {
                            target = &targetlist->data[j];
                            
                            if (i == j)
                                continue;

                            /* optimization - enemy merging - keep it like this */
                            if (entity_distance(*e, *target) < 0.5) {
                                // remove other
                                vec_remove(targetlist, j);
                                j--;
                            }

                            if (is_collision(game.player, *e)) {
                                DamagePlayer(e->contact_damage);
                            }
                        }
                        UpdateLargeEnemy(e);
                    }

                    if (draw) {
                        DrawLargeEnemy(e);
                    }

                    break;
                }
                default: break;
            }
        }
    }
}

void ManageParticles(bool draw, bool update) {
    if (!draw && !update) {
        return;
    }

    EntityVec ev;
    Entity *target;
    Particle *p;

    for (int i = 0; i < game.particles.length; i++) {
        p = &game.particles.data[i];
        if (draw) {
            DrawPExplosion(p, update);
            if (show_hitboxes) {
                DrawParticleHitbox(p);
            }
        }

        if (update) {
            ev = game.entities[E_ENEMY_BASIC];
            for (int j = 0; j < ev.length; j++) {
                target = &ev.data[j];
                if (is_p_collision(*p, *target)) {
                    target->hp -= p->damage;

                    if (target->hp <= 0) {
                        // remove target
                        vec_remove(&ev, j);
                        j--;
                        continue;
                    }
                }
            }

            ev = game.entities[E_ENEMY_LARGE];
            for (int j = 0; j < ev.length; j++) {
                target = &ev.data[j];
                if (is_p_collision(*p, *target)) {
                    target->hp -= p->damage;

                    if (target->hp <= 0) {
                        // remove target
                        vec_remove(&ev, j);
                        j--;
                        continue;
                    }
                }
            }

            if (ParticleDone(*p)) {
                vec_remove(&game.particles, i);
                i--;
                continue;
            }
        }
    }
}

Entity RandSpawnEnemy(EntityType type) {
    float x, y;

    x = randrange(game.player.x - (game.config.screen_margin[1] * GetScreenWidth()/2), game.player.x + (game.config.screen_margin[1] + GetScreenWidth()/2));

    if (game.player.x - (game.config.screen_margin[0] * GetScreenWidth()/2) <= x && x <= game.player.x + (game.config.screen_margin[0] * GetScreenWidth()/2)) {
        // if x position is on the screen, y pos should be offscreen
        y = cointoss(
            randrange(game.player.y - (game.config.screen_margin[1] * GetScreenHeight()/2), game.player.y - (game.config.screen_margin[0] * GetScreenHeight()/2)),
            randrange(game.player.y + (game.config.screen_margin[0] * GetScreenHeight()/2), game.player.y + (game.config.screen_margin[1] * GetScreenHeight()/2))
        );
    } else {
        // otherwise, y can be anywhere
        y = randrange(game.player.y - (game.config.screen_margin[1] * GetScreenHeight()/2), game.player.y + (game.config.screen_margin[1] * GetScreenHeight()/2));
    }

    // printf("spawning at (%f, %f)\n", x, y);

    return (Entity){
        .type = type,
        .x = x,
        .y = y,
        .size = getattr(type, size),
        .speed = getattr(type, speed),
        .max_hp = getattr(type, max_hp),
        .hp = getattr(type, max_hp),
        .contact_damage = getattr(type, contact_damage),
    };
}

/* Fires at direction of closest enemy and keeps going in that direction (not homing bullets) */
Entity PlayerFireBullet(void) {
    Entity *p = player_closest_enemy();
    if (p == NULL) {
        // no enemies to shoot at, so don't fire any bullets
        // NONE means it's invalid
        return (Entity) {
            .type = E_NONE
        };
    }
    return (Entity) {
        .type = E_PLAYER_BULLET,
        .x = game.player.x,
        .y = game.player.y,
        .size = getattr(E_PLAYER_BULLET, size),
        .speed = getattr(E_PLAYER_BULLET, speed),
        .angle = entity_angle(game.player, *p),
        .contact_damage = getattr(E_PLAYER_BULLET, contact_damage)
    };
}

/* Fires at the mouse cursor and keeps going that way */
Entity PlayerFireShell(void) {
    return (Entity) {
        .type = E_PLAYER_SHELL,
        .x = game.player.x,
        .y = game.player.y,
        .size = getattr(E_PLAYER_SHELL, size),
        .speed = getattr(E_PLAYER_SHELL, speed),
        .angle = vec_angle(
            (Vector2){ game.player.x, game.player.y }, 
            (Vector2){ GraphicsGetScreenOffset().x + GetMouseX(), GraphicsGetScreenOffset().y + GetMouseY() }
        ),
        .contact_damage = getattr(E_PLAYER_SHELL, contact_damage)
    };
}

void MoveEntityToPlayer(Entity *e) {
    double dist_to_player;
    Vector2 v;

    dist_to_player = sqrt(
        pow(game.player.x - e->x, 2) + 
        pow(game.player.y - e->y, 2)
    );

    v = (Vector2){game.player.x - e->x, game.player.y - e->y};
    v.x /= dist_to_player;
    v.y /= dist_to_player;
    e->x += (v.x * e->speed);
    e->y += (v.y * e->speed);
}

/* particle methods */

Rectangle ParticleHitbox(Particle p) {
    return (Rectangle){ p.x-p.size/2, p.y-p.size/2, p.size, p.size };
}

void DrawParticleHitbox(Particle *p) {
    DrawRectangleLinesEx(ParticleHitbox(*p), 1.0, BLACK);
}

void SpawnParticle(ParticleType type, float x, float y) {
    vec_push(&game.particles, NewParticle(type, x, y));
}

Particle NewParticle(ParticleType type, float x, float y) {
    return (Particle) {
        .type = type,
        .x = x,
        .y = y,
        // TODO: make ParticleAttrs indexed by type
        .starting_size = 3.0,
        .size = 3.0,
        .lifetime = 10,
        .currframe = 1,
        .damage = 200,
    };
}

/* does it need to be removed? is the animation done? */
bool ParticleDone(Particle p) {
    return p.currframe >= p.lifetime;
}

void DrawPExplosion(Particle *exp, bool advance_frame) {
    exp->size = exp->currframe * exp->starting_size;
    DrawCircle(exp->x, exp->y, exp->size, YELLOW);
    if (advance_frame) {
        exp->currframe++;
    }
}

/* general utils */

Vector2 screensize(void) {
    if (game.config.window_initialized) {
        return (Vector2){ GetScreenWidth(), GetScreenHeight() };
    } else {
        return game.config.window_init_dim;
    }
}

Vector2 screen_offset(void) {
    return (Vector2) { game.player.x - screensize().x/2, game.player.y - screensize().y/2 };
}

int randrange(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

// [min, max]
float randfloat(float min, float max) {
    float scale = rand() / (float) RAND_MAX; /* [0, 1.0] */
    return min + scale * ( max - min );      /* [min, max] */
}

// bad idea (is this even used?)
float randchoice(size_t count, float *probs, ...) {
    va_list opts;
    float r = randrange(0, count-1);
    float ret;
    va_start(opts, probs);
    for (int i = 0; i <= r; i++) {
        ret = va_arg(opts, float);
    }
    va_end(opts);
    return ret;
}

float distance(Vector2 a, Vector2 b) {
    return sqrt(
        pow(b.x - a.x, 2) +
        pow(b.y - a.y, 2)
    );
}

bool is_collision(Entity a, Entity b) {
    return CheckCollisionRecs(EntityHitbox(a), EntityHitbox(b));
}

bool is_p_collision(Particle p, Entity e) {
    return CheckCollisionRecs(ParticleHitbox(p), EntityHitbox(e));
}

bool entity_offscreen(Entity e) {
    float w = GetScreenWidth()/2;
    float h = GetScreenHeight()/2;
    return e.x < game.player.x - (game.config.screen_margin[0] * w)
        || e.x > game.player.x + (game.config.screen_margin[0] * w)
        || e.y < game.player.y - (game.config.screen_margin[0] * h)
        || e.y > game.player.y + (game.config.screen_margin[0] * h);
}

float entity_distance(Entity a, Entity b) {
    return distance(
        (Vector2){ a.x, a.y },
        (Vector2){ b.x, b.y }
    );
} 

float entity_angle(Entity a, Entity b) {
    return atan2(
        b.y - a.y, b.x - a.x
    );
}

float vec_angle(Vector2 a, Vector2 b) {
    return atan2(
        b.y - a.y, b.x - a.x
    );
}

Entity *player_closest_enemy(void) {

    float smallest_dist = 10000.0, temp;
    int closest_index = -1;
    EntityType closest_type = -1;

    EntityVec *entvec;

    static EntityType enemytypes[] = { E_ENEMY_BASIC, E_ENEMY_LARGE };

    for (int etype = 0; etype < 2; etype++) {
        entvec = &game.entities[enemytypes[etype]];
        if (entvec->data == NULL){
            continue;
        }
        
        for (int i = 0; i < entvec->length; i++) {

            if ((temp = entity_distance(game.player, entvec->data[i])) < smallest_dist) {

                smallest_dist = temp;
                closest_index = i;
                closest_type = etype;
                continue;
            }
        }
    }

    return (closest_type != E_NONE && closest_index != -1) ? &game.entities[closest_type].data[closest_index] : NULL;
    
}

Entity *player_closest_entity(EntityType type) {
    float dist = 10000.0, temp;
    int closest;
    
    EntityVec *entvec = &game.entities[type];

    if (entvec->data == NULL){
        return NULL;
    }
    
    for (int i = 0; i < entvec->length; i++) {
        if ((temp = entity_distance(game.player, entvec->data[i])) < dist) {
            dist = temp;
            closest = i;
            continue;
        }
    }

    return &entvec->data[closest];
}

/* callbacks */

void StartBtnCallback(void) {
    SetState(GS_GAMEPLAY);
}

void RestartBtnCallback(void) {
    SetState(GS_GAMEPLAY);
}

void BasicEnemySpawnTimerCallback(void) {
    vec_push(&game.entities[E_ENEMY_BASIC], RandSpawnEnemy(E_ENEMY_BASIC));
}

void LargeEnemySpawnTimerCallback(void) {
    vec_push(&game.entities[E_ENEMY_LARGE], RandSpawnEnemy(E_ENEMY_LARGE));
}

void PlayerInvincTimerCallback(void) {
    game.player.invincible = false;
}

void PlayerBulletTimerCallback(void) {
    Entity p = PlayerFireBullet();

    // if no enemies to fire at, don't fire
    if (p.type != E_NONE) {
        vec_push(&game.entities[E_PLAYER_BULLET], p);
    }
}

void PlayerShellTimerCallback(void) {
    vec_push(&game.entities[E_PLAYER_SHELL], PlayerFireShell());
}

void DoNothingCallback(void) {
    printf(":)\n");
}
