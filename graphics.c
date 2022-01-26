#include "graphics.h"

/* gets top left corner position */
ScreenOffsetFunc GraphicsGetScreenOffset;
ScreenSizeFunc GraphicsGetScreenSize;

Button NewButton(float x, float y, float w, float h, const char *label, BtnCallback callback) {
    Button b = (Button) {
        x, y, w, h, {0}, callback
    };
    strcpy(b.label, label);
    return b;
}

void DrawButton(Button b) {

    Vector2 offset = GraphicsGetScreenOffset();
    Vector2 size = GraphicsGetScreenSize();

    float x = offset.x + (b.x / 100 * size.x);
    float y = offset.y + (b.y / 100 * size.y);
    float w = b.w / 100 * size.x;
    float h = b.h / 100 * size.y;
    
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
    DrawTextC(b.label, x + w/2, y + h/2, h/2, BLACK);
    
    if (MouseInside(b) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        b.on_click();
    }
}


/* centered */
void DrawTextC(const char *text, float x, float y, int fontSize, Color color) {
    float w = MeasureText(text, fontSize);
    float h = fontSize; // right?
    DrawText(text, x - w/2, y - h/2, fontSize, color);
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