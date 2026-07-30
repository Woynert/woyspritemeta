/*
   Simple draw buffer for 2D raylib commands:
   */

#ifndef RAYLIB_DRAWBUFFER_H
#define RAYLIB_DRAWBUFFER_H

#include "arenady.h"
#include "raylib.h"
#include "raylib_extra.h"

typedef enum {
    DRAWCMD_RECTANGLE,
    DRAWCMD_RECTANGLE_LINES,
    DRAWCMD_TEXT_EX,
    DRAWCMD_TEXTURE,
} DrawCmd;


#define DYNA__TYPE DrawCmd
#define DYNA__NAMESPACE DrawCmd_da
#include "da.h"


typedef struct drawbuf_Layer {
    ArenaDy arena;
    DrawCmd_da commands;
} drawbuf_Layer;


struct {
    drawbuf_Layer layers[256];
} DrawBuf;
uint8_t DrawBuf__currlayer = 0;


void drawbuf_init(void) {
    for (int i = 0; i < countofi(DrawBuf.layers); ++i) {
        drawbuf_Layer *layer = &DrawBuf.layers[i];
        layer->arena = arenady_create();
        layer->commands = DrawCmd_da_create();
    }
}

void drawbuf_deinit(void) {
    for (int i = 0; i < countofi(DrawBuf.layers); ++i) {
        drawbuf_Layer *layer = &DrawBuf.layers[i];
        arenady_free(&layer->arena);
        DrawCmd_da_free(&layer->commands);
    }
}

void drawbuf_set_layer(uint8_t layer) {
    DrawBuf__currlayer = layer;
}

typedef struct { Rect2i r; Color color; } drawbuf_t_DrawRectangle;
typedef struct { Rect2i r; Color color; int thickness; } drawbuf_t_DrawRectangleLines;
typedef struct { Font font; V2i position; int font_size; int spacing; int textLineSpacing; Color tint; int str_size; char str_data[]; } drawbuf_t_DrawTextEx;
typedef struct { Texture2D texture; Rectangle source; Rectangle dest; Vector2 origin; float rotation; Color tint; } drawbuf_t_DrawTexturePro;

void drawbuf_draw_all(void) {
    for (int i = 0; i < countofi(DrawBuf.layers); ++i) {
        drawbuf_Layer *layer = &DrawBuf.layers[i];
        arenady_reset_beginning(&layer->arena);
        for (dyna_foreach(DrawCmd, iter, layer->commands)) {
            DrawCmd cmd = *iter.ref;
            switch (cmd) {
                case DRAWCMD_RECTANGLE:
                {
                    drawbuf_t_DrawRectangle *args = arenady_new(&layer->arena, drawbuf_t_DrawRectangle, 1);
                    DrawRectangleReci(args->r, args->color);
                } break;
                case DRAWCMD_RECTANGLE_LINES:
                {
                    drawbuf_t_DrawRectangleLines *args = arenady_new(&layer->arena, drawbuf_t_DrawRectangleLines, 1);
                    DrawRectangleLinesi(args->r, args->color, (float)args->thickness);
                } break;
                case DRAWCMD_TEXT_EX:
                {
                    drawbuf_t_DrawTextEx *args = arenady_new(&layer->arena, drawbuf_t_DrawTextEx, 1);
                    arenady_new(&layer->arena, char, args->str_size);
                    strview_t str = { .data = args->str_data, .size = args->str_size };
                    DrawTextEx_strview_i(args->font, str, args->position, args->font_size, args->spacing, args->textLineSpacing, args->tint);
                } break;
                case DRAWCMD_TEXTURE:
                {
                    drawbuf_t_DrawTexturePro *args = arenady_new(&layer->arena, drawbuf_t_DrawTexturePro, 1);
                    DrawTexturePro(args->texture, args->source, args->dest, args->origin, args->rotation, args->tint);
                } break;
            }
        }

        // Reset for next frame.
        arenady_reset_beginning(&layer->arena);
        DrawCmd_da_clear_preserving(&layer->commands);
    }
}

void b_DrawTexturePro(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint) {
    drawbuf_Layer *layer = &DrawBuf.layers[DrawBuf__currlayer];
    drawbuf_t_DrawTexturePro *args = arenady_new(&layer->arena, drawbuf_t_DrawTexturePro, 1);
    args->texture = texture;
    args->source = source;
    args->dest = dest;
    args->origin = origin;
    args->rotation = rotation;
    args->tint = tint;
    DrawCmd_da_append(&layer->commands, DRAWCMD_TEXTURE);
}

void b_DrawRectangle(Rect2i r, Color color) {
    drawbuf_Layer *layer = &DrawBuf.layers[DrawBuf__currlayer];
    drawbuf_t_DrawRectangle *args = arenady_new(&layer->arena, drawbuf_t_DrawRectangle, 1);
    args->r = r;
    args->color = color;
    DrawCmd_da_append(&layer->commands, DRAWCMD_RECTANGLE);
}

void b_DrawRectangleLines(Rect2i r, Color color, int thickness) {
    drawbuf_Layer *layer = &DrawBuf.layers[DrawBuf__currlayer];
    drawbuf_t_DrawRectangleLines *args = arenady_new(&layer->arena, drawbuf_t_DrawRectangleLines, 1);
    args->r = r;
    args->color = color;
    args->thickness = thickness;
    DrawCmd_da_append(&layer->commands, DRAWCMD_RECTANGLE_LINES);
}

void b_DrawTextEx(Font font, const strview_t string, V2i pos, int font_size, int spacing, int textLineSpacing, Color tint) {
    drawbuf_Layer *layer = &DrawBuf.layers[DrawBuf__currlayer];
    // Allocate struct + size in one call.
    drawbuf_t_DrawTextEx *args = (drawbuf_t_DrawTextEx *)
        arenady_alloc(&layer->arena, (i64)(sizeof(drawbuf_t_DrawTextEx) + (size_t)string.size), _Alignof(drawbuf_t_DrawTextEx), 1);
    args->font = font;
    args->position = pos;
    args->font_size = font_size;
    args->spacing = spacing;
    args->textLineSpacing = textLineSpacing;
    args->tint = tint;
    args->str_size = string.size;
    memmove(args->str_data, string.data, (size_t)string.size);
    DrawCmd_da_append(&layer->commands, DRAWCMD_TEXT_EX);
}

// Extra functions for ease of use. ↓↓↓

void b_DrawText(const strview_t str, V2i pos, int font_size, Color tint) {
    b_DrawTextEx(GetFontDefault(), str, pos, font_size, 1, 1, tint);
}

void b_DrawTextureEx(Texture2D texture, V2i pos, float rotation, float scale, Color tint) {
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
    Rectangle dest = { (float)pos.x, (float)pos.y, (float)texture.width*scale, (float)texture.height*scale };
    Vector2 origin = { 0.0f, 0.0f };
    b_DrawTexturePro(texture, source, dest, origin, rotation, tint);
}

void b_DrawTexture(Texture2D texture, V2i pos, Color tint) {
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
    Rectangle dest = { (float)pos.x, (float)pos.y, (float)texture.width, (float)texture.height };
    Vector2 origin = { 0.0f, 0.0f };
    b_DrawTexturePro(texture, source, dest, origin, 0, tint);
}

#endif
