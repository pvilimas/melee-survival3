#include "game.h"

extern ScreenOffsetFunc GraphicsGetScreenOffset;
extern ScreenSizeFunc GraphicsGetScreenSize;

/* entity attribute access */
#define getattr(etype, attr) (game.config.entitydata[etype].attr)

/*
    TODO:
    - impl the restart button (HARD) (maybe)
    - fix the inputs so A+D won't freeze, but whichever was pressed first takes precedence

    - add 3 more types of enemies and spawn all of them randomly
    - then make it scale over time
    - then add a game timer (UI for how long you've been in the game)
    
    - make sure DestroyGame works properly w/ no mem leaks

    - timed sprite that changes over time as a function of the time it's been on screen, then gets removed at the end
        - like incineration flames from magic survival
*/

// used to tile the background
const char *BG_IMG_PATH = "./assets/background/bg_hextiles.png";

Game game = {
    .config = {
        .window_initialized = false,
        .window_init_dim = (Vector2) { 800, 500 },
        .target_fps = 60,
        .screen_margin = 1.20f,
        .entitydata = {
            [E_PLAYER] = {
                .subspawn_type = E_PLAYER_BULLET,
                .subspawn_interval = 1.0,
                .max_hp = 100,
                .invincibility_time = 1.0,
                .contact_damage = 0,
            },
            [E_ENEMY_BASIC] = {
                .spawn_interval = 0.5,
                .spawn_margin = 1.5,
                .speed = 1.0,
                .size = 6,
                .max_hp = 100,
                .contact_damage = 10,
            },
            [E_PLAYER_BULLET] = {
                .spawn_interval = 1.0,
                .speed = 4.0,
                .contact_damage = 100,
            },
        },
    },
    .ui = {
        .start_btn = (Button){
            38, 55, 24, 10, "Start", StartBtnCallback,
        },
    },
};


/* game methods */

void InitGame(void) {

    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    SetExitKey(KEY_ESCAPE);
    InitWindow(screensize().x, screensize().y, "Melee Survival");
    SetTargetFPS(game.config.target_fps);
    game.config.window_initialized = true;

    GraphicsGetScreenOffset = screen_offset;
    GraphicsGetScreenSize = screensize;

    for (int i = 0; i < E_COUNT; i++) {
        vec_init(&game.entities[i]);
    }

    Image bg_image = LoadImage(BG_IMG_PATH);
    game.textures.background = LoadTextureFromImage(bg_image);
    UnloadImage(bg_image);


    game.state = GS_TITLE;
    game.timers = (GameTimers){
        .enemy_spawn = NewTimer(getattr(E_ENEMY_BASIC, spawn_interval), EnemySpawnTimerCallback),
        .player_invinc = NewTimer(getattr(E_PLAYER, invincibility_time), PlayerInvincTimerCallback),
        .player_fire_bullet = NewTimer(getattr(E_PLAYER, subspawn_interval), PlayerBulletTimerCallback),
    };
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
        sleep(frametime - (GetTime() - time));
    }
}

void DestroyGame(void) {
    CloseWindow();
    game.config.window_initialized = false;
    for (int i = 0; i < E_COUNT; i++) {
        vec_deinit(&game.entities[i]);
    }
    UnloadTexture(game.textures.background);
}

void SetState(GameState new_state) {
    GameState old_state = game.state;
    game.state = new_state;
}

void HandleInput(void) {
    Entity *player = &game.player;
    float slow_speed = player->speed / (2 * sqrt(2.0));
    float fast_speed = player->speed;

    if (IsKeyPressed(KEY_P)) {
        if (game.state == GS_GAMEPLAY) {
            SetState(GS_PAUSED);
            return;
        } else if (game.state == GS_PAUSED) {
            SetState(GS_GAMEPLAY);
            return;
        }
    }

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
    CheckTimer(&game.timers.enemy_spawn);

    TileBackground();
    DrawGameUI();
    HandleInput();
    UpdateCam();
    DrawPlayer(true);
    ManageEntities(true, true);

    EndMode2D();
    
}

void DrawPaused(void) {

    BeginMode2D(game.camera);

    TileBackground();
    DrawGameUI();
    DrawPlayer(false);
    HandleInput();
    UpdateCam();
    ManageEntities(true, false);

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
    ClearBackground(RAYWHITE);
    DrawText("gameover screen", 100, 100, 20, RED);
}

/* draw functions */

/* please don't mess with this please :) */
void TileBackground(void) {
    for (int i = ((game.player.x - GetScreenWidth()/2) / game.textures.background.width) - 1; i < ((game.player.x + GetScreenWidth()/2) / game.textures.background.width) + 1; i++) {
        for (int j = ((game.player.y - GetScreenHeight()/2) / game.textures.background.height) - 1; j < ((game.player.y + GetScreenHeight()/2) / game.textures.background.height) + 1; j++) {
            DrawTexture(
                game.textures.background,
                i * game.textures.background.width,
                j * game.textures.background.height,
                (Color){255, 255, 255, 255}
            );
        }
    }
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

void DrawEnemy(Entity *enemy) {
    DrawCircle(enemy->x, enemy->y, enemy->size, MAROON);
    DrawCircle(enemy->x, enemy->y, enemy->size * 2/3, RED);
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

void UpdateEnemy(Entity *enemy) {
    MoveEntityToPlayer(enemy);
}

void UpdateBullet(Entity *bullet) {
    bullet->x += bullet->speed * cos(bullet->angle);
    bullet->y += bullet->speed * sin(bullet->angle);
}

/* checks all player bullets with all basic enemies to see if any collided, then remove if they did */
void CollideBullets(void) {

    EntityVec *bullets, *enemies;
    Entity bullet, enemy;

    enemies = &game.entities[E_ENEMY_BASIC];
    bullets = &game.entities[E_PLAYER_BULLET];

    if (vec_empty(enemies) || vec_empty(bullets)) {
        return;
    }

    for (int i = 0; i < enemies->length; i++) {
        enemy = enemies->data[i];
        for (int j = 0; j < bullets->length; i++) {
            bullet = bullets->data[j];
            if (is_collision(enemy, bullet)) {
                enemy.hp -= bullet.contact_damage;
                if (enemy.hp <= 0) {
                    // remove enemy
                    vec_remove(enemies, i);
                    i--;
                }
                

                // remove bullet
                vec_remove(bullets, j);
                j--;

                // bullet has been deleted, so return
                return;
            }
        } 
    }
}

/* game ui elements */

void DrawGameUI(void) {
    DisplayPlayerHP();
}

void DisplayPlayerHP(void) {
    char hp_str[15];
    sprintf(hp_str, "HP: %d/%d", game.player.hp, game.player.max_hp);
    DrawTextUI(hp_str, 87, 5, 30, BLACK);
}

/* entity methods */

Rectangle EntityHitbox(Entity e) {
    switch (e.type) {
        case E_ENEMY_BASIC:     return (Rectangle){ e.x-3, e.y-3, 6.0, 6.0 };
        case E_PLAYER_BULLET:   return (Rectangle){ e.x-2, e.y-2, 4.0, 4.0 };
        case E_PLAYER:          return (Rectangle){ e.x-4, e.y-4, 8.0, 8.0 };
        default:                return (Rectangle){ };
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
                
                /* draw bullets first, then the enemies they hit */
                case E_PLAYER_BULLET: {
                    e = &entlist->data[i];
                    if (entity_offscreen(*e)) {
                        if (update) {
                            vec_remove(entlist, i);
                            i--;
                        }
                    } else {

                        
                        if (update) {
                            // check for any collisions
                            targetlist = &game.entities[E_ENEMY_BASIC];
                            for (int j = 0; j < targetlist->length; j++) {
                                target = &targetlist->data[j];
                                if (is_collision(*e, *target)) {
                                    target->hp -= e->contact_damage;
                                    if (target->hp <= 0) {
                                        // remove target
                                        vec_remove(targetlist, j);
                                        j--;
                                    }
                                    
                                    // remove bullet
                                    vec_remove(entlist, i);
                                    i--;

                                    continue;
                                }
                            }
                            UpdateBullet(e);
                        }

                        if (draw) {
                           DrawBullet(e);
                        }
                    }

                    break;
                }
                case E_ENEMY_BASIC: {
                    
                    if (update) {
                        // if two enemies have the "same" xy pos, remove one
                        targetlist = &game.entities[E_ENEMY_BASIC];
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
                        UpdateEnemy(e);
                    }

                    if (draw) {
                        DrawEnemy(e);
                    }

                    break;
                }
                default: break;
            }

        }
    }
}

Entity RandSpawnEnemy(void) {
    float x, y;
    x = randrange(game.player.x - (game.config.screen_margin * GetScreenWidth()/2), game.player.x + (game.config.screen_margin * GetScreenWidth()/2));
    
    if (game.player.x - GetScreenWidth()/2 <= x && x <= game.player.x + GetScreenWidth()/2) {
        
        // if x position is on the screen, y pos should be offscreen
        y = cointoss(
            randrange(game.player.y - (game.config.screen_margin * GetScreenHeight()/2), game.player.y - (GetScreenHeight()/2)),
            randrange(game.player.y + (GetScreenHeight()/2), game.player.y + (game.config.screen_margin * GetScreenHeight()/2))
        );

    } else {

        // otherwise, y can be anywhere
        y = randrange(game.player.y - (game.config.screen_margin * GetScreenHeight()/2), game.player.y + (game.config.screen_margin * GetScreenHeight()/2));
    }

    // printf("spawning at (%f, %f)\n", x, y);

    return (Entity){
        .type = E_ENEMY_BASIC,
        .x = x,
        .y = y,
        .size = getattr(E_ENEMY_BASIC, size),
        .speed = getattr(E_ENEMY_BASIC, speed),
        .max_hp = getattr(E_ENEMY_BASIC, max_hp),
        .hp = getattr(E_ENEMY_BASIC, max_hp),
        .contact_damage = getattr(E_ENEMY_BASIC, contact_damage),
    };
}

/* Fires at direction of closest enemy and keeps going in that direction (not homing bullets) */
Entity PlayerFireBullet(void) {
    Entity *p = player_closest_entity(E_ENEMY_BASIC);
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
        .size = getattr(E_ENEMY_BASIC, size),
        .speed = getattr(E_PLAYER_BULLET, speed),
        .angle = entity_angle(game.player, *p),
        .contact_damage = getattr(E_PLAYER_BULLET, contact_damage)
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

// 50/50 chance
float cointoss(float a, float b) {
    return (float) ((rand() % 2) ? a : b);
}

float distance(Vector2 a, Vector2 b) {
    return sqrt(
        pow(b.x - a.x, 2) +
        pow(b.y - a.y, 2)
    );
}

bool is_collision(Entity a, Entity b) {
    return CheckCollisionRecs(EntityHitbox(a), EntityHitbox(b));
    return entity_distance(a, b) < 3.0f;
    return distance(
        (Vector2){EntityHitbox(a).x, EntityHitbox(a).y},
        (Vector2){EntityHitbox(b).x, EntityHitbox(b).y}
    ) < 3.0f;
}

bool entity_offscreen(Entity e) {
    float w = GetScreenWidth()/2;
    float h = GetScreenHeight()/2;
    return e.x < game.player.x - (w)
        || e.x > game.player.x + (w)
        || e.y < game.player.y - (h)
        || e.y > game.player.y + (h);
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

Entity *player_closest_entity(EntityType type) {
    EntityVec *entvec = &game.entities[type];

    if (entvec->data == NULL){
        return NULL;
    }

    float dist = 10000.0, temp;
    int closest;
    
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

void EnemySpawnTimerCallback(void) {
    vec_push(&game.entities[E_ENEMY_BASIC], RandSpawnEnemy());
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

void DoNothingCallback(void) {
    printf(":)\n");
}