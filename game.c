#include "game.h"

extern ScreenOffsetFunc GraphicsGetScreenOffset;
extern ScreenSizeFunc GraphicsGetScreenSize;


/*
    TODO:
    - timer callbacks 
    - fix collision checking (CheckCollisionRecs not < 3.0f)
        - alternatively an entity size (or width and height)
    - impl player.hp, player invincibility timer, enemy HP and bullet damage
    - then add player HP meter
    - then make it go to the loss screen when it drops to 0
    - impl the restart button (HARD)

    - add 3 more types of enemies and spawn all of them randomly
    - then make it scale over time
    - then add a game timer
    
    - make sure DestroyGame works
*/

// used to tile the background
const char *BG_IMG_PATH = "./assets/background/bg_hextiles.png";

Game game = {
    .config = {
        .window_initialized = false,
        .window_init_dim = (Vector2) { 800, 500 },
        .target_fps = 60,
        .randspawn_margin = 1.20f,
        .playerdata = {
            .subspawn_type = E_PLAYER_BULLET,
            .subspawn_interval = 1.0,
            .speed = 3.0,
            .max_hp = 100,
            .invincibility_time = 0.5
        },
        .entitydata = {
            [E_ENEMY_BASIC] = {
                .spawn_interval = 0.5,
                .spawn_margin = 1.5,
                .speed = 1.0,
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
    .state = GS_TITLE,
    .ui = {
        .start_btn = (Button){
            38, 55, 24, 10, "Start", StartBtnCallback,
        },
    },
    .timers = {
        .enemy_spawn = NewTimer(0.5, EnemySpawnTimerCallback),
        .player_invinc = NewTimer(0.5, PlayerInvincTimerCallback),
        .player_fire_bullet = NewTimer(1.0, PlayerBulletTimerCallback),
    },
    .camera = (Camera2D){
        .target = (Vector2){ 0, 0 },
        .zoom = 1.0f
    },
    .player = (Entity){
        .x = 400,
        .y = 250,
        .speed = 3.0f,
        .invincible = false,
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

void SetState(GameState s) {
    game.state = s;
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
    // TODO: make this scale with screen size, make it % based like the buttons
    DrawTextC("Melee Survival", GetScreenWidth()/2, GetScreenHeight()/2 - 100, 50, BLACK);
    DrawButton(game.ui.start_btn);
}

void DrawGameplay(void) {

    BeginMode2D(game.camera);
    
    HandleInput();

    CheckTimer(&game.timers.enemy_spawn);
    CheckTimer(&game.timers.player_fire_bullet);

    TileBackground();
    DrawPlayer();

    ManageEntities(true, true);

    UpdateCam();

    EndMode2D();
}

void DrawPaused(void) {

    BeginMode2D(game.camera);

    HandleInput();

    TileBackground();
    DrawPlayer();

    ManageEntities(true, false);

    Vector2 offset = GraphicsGetScreenOffset();
    Vector2 size = GraphicsGetScreenSize();

    float x = offset.x;
    float y = offset.y;
    float w = size.x;
    float h = size.y;

    DrawRectangle(x, y, w, h, (Color){180, 180, 180, 180});
    DrawTextC("PAUSED", x+w/2, y+h/2, 50, BLACK);

    EndMode2D();
    
}

void DrawGameover(void) {
    ClearBackground(RAYWHITE);
    DrawText("gameover screen", 100, 100, 20, RED);
}

/* draw functions */

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

void DrawPlayer(void) {
    DrawCircle(game.player.x, game.player.y, 8, BLACK);
    DrawCircle(game.player.x, game.player.y, 6, (Color){60, 60, 60, 255});
}

void DrawEnemy(Entity *enemy) {
    DrawCircle(enemy->x, enemy->y, 6, MAROON);
    DrawCircle(enemy->x, enemy->y, 4, RED);
}

void DrawBullet(Entity *bullet) {
    DrawLineEx(
        (Vector2){bullet->x - 4 * cos(bullet->angle), bullet->y - 4 * sin(bullet->angle)},
        (Vector2){bullet->x + 4 * cos(bullet->angle), bullet->y + 4 * sin(bullet->angle)},
        4.0f, DARKGRAY
    );
    DrawLineEx(
        (Vector2){bullet->x - 3 * cos(bullet->angle), bullet->y - 3 * sin(bullet->angle)},
        (Vector2){bullet->x + 3 * cos(bullet->angle), bullet->y + 3 * sin(bullet->angle)},
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
                
                // remove enemy
                vec_remove(enemies, i);
                i--;

                // and remove bullet
                vec_remove(bullets, j);
                j--;
                return;
            }
        } 
    }
}

/* entity methods */

Rectangle EntityHitbox(Entity e) {
    switch (e.type) {
        case E_ENEMY_BASIC: return (Rectangle){e.x-3, e.y-3, 6.0, 6.0};
        case E_PLAYER: return (Rectangle){e.x-4, e.y-4, 8.0, 8.0};
        default: return (Rectangle){};
    }
}

void ManageEntities(bool draw, bool update) {

    if (!draw && !update) {
        return;
    }

    EntityVec *entlist, *targetlist;
    Entity *e, target;

    // for each type of entity
    for (int etype = 0; etype < E_COUNT; etype++) {
        entlist = &game.entities[etype];
        if (vec_empty(entlist))
            continue;

        // for each entity
        for (int i = 0; i < entlist->length; i++) {
            e = &entlist->data[i];

            switch (etype) {
                case E_ENEMY_BASIC: {
                    
                    if (update) {
                        // if two enemies have the "same" xy pos, remove one
                        targetlist = &game.entities[E_ENEMY_BASIC];
                        for (int j = 0; j < targetlist->length; j++) {
                            target = targetlist->data[j];
                            
                            if (i == j)
                                continue;

                            if (entity_distance(*e, target) < 0.5) {
                                // remove other
                                vec_remove(targetlist, j);
                                j--;
                                continue;
                            }
                        }
                        UpdateEnemy(e);
                    }

                    if (draw) {
                        DrawEnemy(e);
                    }

                    break;
                }
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
                                target = targetlist->data[j];
                                if (is_collision(*e, target)) {
                                    
                                    // remove projectile
                                    vec_remove(entlist, i);
                                    i--;

                                    // and remove target
                                    vec_remove(targetlist, j);
                                    j--;
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
                default: break;
            }

        }
    }
}

Entity RandSpawnEnemy(void) {
    float x, y;
    x = randrange(game.player.x - (game.config.randspawn_margin * GetScreenWidth()/2), game.player.x + (game.config.randspawn_margin * GetScreenWidth()/2));
    
    if (game.player.x - GetScreenWidth()/2 <= x && x <= game.player.x + GetScreenWidth()/2) {
        
        // if x position is on the screen, y pos should be offscreen
        y = cointoss(
            randrange(game.player.y - (game.config.randspawn_margin * GetScreenHeight()/2), game.player.y - (GetScreenHeight()/2)),
            randrange(game.player.y + (GetScreenHeight()/2), game.player.y + (game.config.randspawn_margin * GetScreenHeight()/2))
        );

    } else {

        // otherwise, y can be anywhere
        y = randrange(game.player.y - (game.config.randspawn_margin * GetScreenHeight()/2), game.player.y + (game.config.randspawn_margin * GetScreenHeight()/2));
    }

    // printf("spawning at (%f, %f)\n", x, y);

    return (Entity){
        E_ENEMY_BASIC,
        x, y,
        game.config.entitydata[E_ENEMY_BASIC].speed,
    };
}

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
        .speed = game.config.entitydata[E_PLAYER_BULLET].speed,
        .angle = entity_angle(game.player, *p)
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
    return entity_distance(a, b) < 3.0f;
    return distance(
        (Vector2){EntityHitbox(a).x, EntityHitbox(a).y},
        (Vector2){EntityHitbox(b).x, EntityHitbox(b).y}
    ) < 3.0f;
//    return CheckCollisionRecs(EntityHitbox(a), EntityHitbox(b));
}

bool entity_offscreen(Entity e) {
    return e.x < game.player.x - (GetScreenWidth()/2)
        || e.x > game.player.x + (GetScreenWidth()/2)
        || e.y < game.player.y - (GetScreenHeight()/2)
        || e.y > game.player.y + (GetScreenHeight()/2);
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

void DoNothingCallback(void) {
    printf(":)\n");
}

void EnemySpawnTimerCallback(void) {
    vec_push(&game.entities[E_ENEMY_BASIC], RandSpawnEnemy());
}

void PlayerInvincTimerCallback(void) {

}

void PlayerBulletTimerCallback(void) {
    Entity p = PlayerFireBullet();

    // if no enemies to fire at, don't fire
    if (p.type != E_NONE) {
        vec_push(&game.entities[E_PLAYER_BULLET], p);
    }
}