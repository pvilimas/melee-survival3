#include "graphics.h"

/* gets top left corner position */
ScreenOffsetFunc GraphicsGetScreenOffset;
ScreenSizeFunc GraphicsGetScreenSize;

Button NewButton(float x, float y, float w, float h, const char *label, BtnCallback callback) {
    return (Button) {
        .x = x,
        .y = y,
        .w = w,
        .h = h,
        .label = label,
        .on_click = callback
    };
}

void DrawButton(Button b) {

    Vector2 offset = GraphicsGetScreenOffset();
    Vector2 size = GraphicsGetScreenSize();

    float x = offset.x + ((b.x / 100.0) * size.x);
    float y = offset.y + ((b.y / 100.0) * size.y);
    float w = b.w / 100.0 * size.x;
    float h = b.h / 100.0 * size.y;
    
    Color fill = (Color) {220, 220, 220, 255};
    if (MouseInside(b)) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            fill = (Color) {200, 200, 200, 255};
        } else {
            fill = (Color) {210, 210, 210, 255};
        }
    }
    
    DrawRectangleRoundedLines((Rectangle){ x, y, w, h }, 0.2f, 10, 2.0f, DARKGRAY);
    DrawRectangleRounded((Rectangle) { x, y, w, h }, 0.2f, 10, fill);
    DrawTextUI(b.label, b.x+b.w/2, b.y+b.h/2, h/2, BLACK);
    
    if (MouseInside(b) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        b.on_click();
    }
}


/* centered */
void DrawTextUI(const char *text, float x_percent, float y_percent, float fontSize, Color color) {
    
    Vector2 offset = GraphicsGetScreenOffset();
    Vector2 size = GraphicsGetScreenSize();

    float x = offset.x + ((x_percent / 100.0) * size.x);
    float y = offset.y + ((y_percent / 100.0) * size.y);
    float w = MeasureText(text, fontSize);
    float h = fontSize; // right?

    DrawTextEx(GetFontDefault(), text, (Vector2){x-w/2, y-h/2}, fontSize, 1.0f, color);
}

bool MouseInside(Button b) {

    Vector2 offset = GraphicsGetScreenOffset();
    Vector2 size = GraphicsGetScreenSize();

    float x = offset.x + (b.x / 100 * size.x);
    float y = offset.y + (b.y / 100 * size.y);
    float w = b.w / 100 * size.x;
    float h = b.h / 100 * size.y;

    int mx = GetMouseX();
    int my = GetMouseY();
    return (x < mx && mx < x + w && y < my && my < y + h);
}
