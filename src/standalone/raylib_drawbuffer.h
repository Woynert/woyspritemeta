/*
   Simple draw buffer for 2D raylib commands:
   */

#ifndef RAYLIB_DRAWBUFFER_H
#define RAYLIB_DRAWBUFFER_H

#include <stdlib.h>
#include "arenady.h"
#include "raylib.h"
#include "raylib_extra.h"

typedef enum {
    DRAWCMD_RECTANGLE,
    DRAWCMD_RECTANGLE_LINES,
    DRAWCMD_TEXT_EX,
    DRAWCMD_TEXTURE,
    DRAWCMD_BEGIN_SCISSOR,
    DRAWCMD_END_SCISSOR,
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

uint8_t drawbuf_get_layer(void) { return DrawBuf__currlayer; }

typedef struct { Rect2i r; Color color; } drawbuf_t_DrawRectangle;
typedef struct { Rect2i r; Color color; int thickness; } drawbuf_t_DrawRectangleLines;
typedef struct { Font font; V2i position; int font_size; int spacing; int textLineSpacing; Color tint; int str_size; char str_data[]; } drawbuf_t_DrawTextEx;
typedef struct { Texture2D texture; Rectangle source; Rectangle dest; Vector2 origin; float rotation; Color tint; } drawbuf_t_DrawTexturePro;
typedef struct { Rect2i r; } drawbuf_t_BeginScissorMode;

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
                case DRAWCMD_BEGIN_SCISSOR:
                {
                    drawbuf_t_BeginScissorMode *args = arenady_new(&layer->arena, drawbuf_t_BeginScissorMode, 1);
                    BeginScissorMode(args->r.x, args->r.y, args->r.width, args->r.height);
                } break;
                case DRAWCMD_END_SCISSOR:
                {
                    EndScissorMode();
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

void b_BeginScissorMode(Rect2i r) {
    drawbuf_Layer *layer = &DrawBuf.layers[DrawBuf__currlayer];
    drawbuf_t_BeginScissorMode *args = arenady_new(&layer->arena, drawbuf_t_BeginScissorMode, 1);
    args->r = r;
    DrawCmd_da_append(&layer->commands, DRAWCMD_BEGIN_SCISSOR);
}

void b_EndScissorMode(void) {
    drawbuf_Layer *layer = &DrawBuf.layers[DrawBuf__currlayer];
    DrawCmd_da_append(&layer->commands, DRAWCMD_END_SCISSOR);
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

void b_DrawTextureRec(Texture2D texture, Rectangle source, V2i pos, Color tint) {
    Rectangle dest = { (float)pos.x, (float)pos.y, fabsf((float)source.width), fabsf((float)source.height) };
    //Rect2i dest = { pos.x, pos.y, abs(source.width), abs(source.height) };
    Vector2 origin = { 0.0f, 0.0f };
    b_DrawTexturePro(texture, source, dest, origin, 0, tint);
}

void b_DrawTextureScaled(Texture2D texture, Rect2i dest) {
    b_DrawTexturePro(
        texture,
        (Rectangle) {
            0, 0,
            (float)texture.width,
            (float)texture.height
        },
        (Rect2i_to_Rect2(dest)).rect,
        Vector2Zero(), 0, WHITE
    );
}

void b_DrawTextureScaled2(Texture2D texture, Rect2i dest, Rect2i source) {
    b_DrawTexturePro(
        texture,
        (Rect2i_to_Rect2(source)).rect,
        (Rect2i_to_Rect2(dest)).rect,
        Vector2Zero(), 0, WHITE
    );
}

void b_DrawTextureRec_flipped (Texture2D texture, Rect2i source, V2i position, Color tint) {
    b_DrawTextureRec(
        texture,
        (Rectangle){
            (float)source.x,
            (float)texture.height - (float)source.height - (float)source.y,
            (float)source.width,
            -(float)source.height
        },
        position, tint
    );
}

#endif
