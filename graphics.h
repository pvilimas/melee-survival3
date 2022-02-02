#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"

typedef Vector2 (*ScreenOffsetFunc)(void);
typedef Vector2 (*ScreenSizeFunc)(void);

typedef void (*BtnCallback)(void);

typedef struct {

    /* a percentage of the screen width */
    float x, y, w, h;
    const char label[15];
    BtnCallback on_click;
} Button;

/* text methods */

float GetScaledFontSize(float scale);
void DrawTextUI(const char *text, float x_percent, float y_percent, float font_scale, Color color);

/* button methods */

Button NewButton(float x, float y, float w, float h, const char *label, BtnCallback callback);
void DrawButton(Button b);
bool MouseInside(Button b);

#endif
