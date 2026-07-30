#include "stdio.h"
#include "raylib.h"
#include "raylib_drawbuffer.h"
#include "raylib_extra.h"
#define LA_IMPLEMENTATION
#include "la.h"

Texture demosprite;

void draw_stuff(void) {
    Rect2i area;

    drawbuf_set_layer(255);
    area = (Rect2i) { {0, 0, 200, 200} };
    b_DrawRectangle(area, RED);
    b_DrawText(cstr_SL("RED"), area.pos, 15, BLACK);
    b_DrawText(cstr_SL("YES"), v2i(area.pos.x, area.pos.y +30), 15, BLACK);
    b_DrawTexture(demosprite, v2i_sub(v2i_add(area.pos, area.size), v2ii(70)), WHITE);
    b_DrawRectangleLines(area, BLACK, 2);

    drawbuf_set_layer(200);
    area = (Rect2i) { {50, 50, 200, 200} };
    b_DrawRectangle(area, BLUE);
    b_DrawText(cstr_SL("BLUE"), area.pos, 15, BLACK);
    b_DrawText(cstr_SL("YES"), v2i(area.pos.x, area.pos.y +30), 15, BLACK);
    b_DrawTexture(demosprite, v2i_sub(v2i_add(area.pos, area.size), v2ii(70)), WHITE);
    b_DrawRectangleLines(area, BLACK, 2);

    drawbuf_set_layer(0);
    area = (Rect2i) { {100, 100, 200, 200} };
    b_DrawRectangle(area, YELLOW);
    b_DrawText(cstr_SL("YELLOW"), area.pos, 15, BLACK);
    b_DrawText(cstr_SL("YES"), v2i(area.pos.x, area.pos.y +30), 15, BLACK);
    b_DrawTexture(demosprite, v2i_sub(v2i_add(area.pos, area.size), v2ii(70)), WHITE);
    b_DrawRectangleLines(area, BLACK, 2);

    drawbuf_draw_all();
}

int main(void) {

    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - basic window");

    SetTargetFPS(60);

    drawbuf_init();

    demosprite = LoadTexture("./assets/simplespritex64.png");

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        /*DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);*/

        draw_stuff();

        EndDrawing();
    }

    drawbuf_deinit();
    CloseWindow();

    return 0;
}
