#ifndef STATE_H
#define STATE_H

#include "strbuf.h"
#include "raylib.h"
#include "zoompanel.h"
#include "arena.h"
#include "arena_extra.h"

typedef struct Ctx Ctx;

typedef struct Action {
    strbuf_t *name;
    int (*op_ptr)(Ctx *ctx); /* @Returns Error. */
} Action;

typedef struct strbuf_wrap {
    strbuf_t *str;
} strbuf_wrap;

typedef struct Spritesheet {
    strbuf_t *path;
    Image image;     /* CPU side. */
    Texture texture; /* GPU side. */
} Spritesheet;

typedef struct Sprite {
    union {
        struct {
            V2i offset;
            V2i size;
        };
        Rect2i rect;
    };

    int frames; /* Calculated by detecting empty sprite. */
    strbuf_t *name;
} Sprite;

#define DYNA__TYPE Action
#include "da.h"

#define DYNA__TYPE strbuf_wrap
#include "da.h"

#define DYNA__TYPE Spritesheet
#include "da.h"

#define DYNA__TYPE Sprite
#include "da.h"

#define DYNA__TYPE int
#define DYNA__ENABLE_COMPARISONS
#include "da.h"

typedef struct Draw {
    int line_height;
    int font_size;
    int char_spacing;
    int line_spacing;
    Font font;

    /* For when needing to draw to a "canvas" that's gonna be shown partially.*/
    RenderTexture2D aux_viewport;
    RenderTexture2D aux_viewport2;

} Draw;

typedef enum SHEETEDITOR_CURSOR {
    SHEETEDITOR_CURSOR_TWEAK,
    SHEETEDITOR_CURSOR_ADD,
    SHEETEDITOR_CURSOR_DRAG,
    SHEETEDITOR_CURSOR_RESIZE,
    SHEETEDITOR_CURSOR__COUNT,
} SHEETEDITOR_CURSOR;
#define SHEETEDITOR_CURSOR_DEFAULT SHEETEDITOR_CURSOR_TWEAK

typedef struct Ctx {
    Draw draw;

    bool has_project_file_open;
    strbuf_t *curr_project_file_path;

    Action_Dyna actions;
    Spritesheet_Dyna spritesheet_list;
    Rect2i spritesheet_image_rect;


    // Sheeteditor widget.
    Zoompanel zoompanel;
    struct {
        int_Dyna selected_sprites;
        int_Dyna selected_sprites_cursor;

        SHEETEDITOR_CURSOR cursor;
        V2i mouse_pos; // @Note: Pixel position in spritesheet image.
        bool mouse_inside;

        // [ @Group: Will be reset on cursor change. ]
        V2i selection_origin;
        bool mouse_is_selecting;
        Rect2i selection;
        V2i drag_prev_mouse_pos; // @Note: This also represents the latest commited drag endpoint.
        V2i drag_origin;
        bool add_can_undo;
        // [ @!Group ]
    } editor;

    Sprite_Dyna sprites;

    // [ Arenas ]
    struct {
        ArenaRoot root;
        Arena arena;
        strbuf_allocator_t strbuf_alloc;
    } frame_arena;
    // [ Arenas ]
} Ctx;

int _init_ctx(Ctx *ctx) {
    ctx->actions = Action_Dyna_create();
    ctx->spritesheet_list = Spritesheet_Dyna_create();
    ctx->curr_project_file_path = strbuf_create_empty(0, NULL);
    ctx->editor.selected_sprites_cursor = int_Dyna_create();
    ctx->editor.selected_sprites = int_Dyna_create();
    ctx->frame_arena.root = ArenaRoot_create(1024 * 1024 * 10); // 10 MB.
    ctx->frame_arena.arena = ArenaRoot_get_arena(ctx->frame_arena.root);
    ctx->frame_arena.strbuf_alloc = make_arena_strbuf_allocator(&ctx->frame_arena.arena);
    return 0;
}

void _free_ctx(Ctx *ctx) {

    // Free draw stuf.
    {
        UnloadFont(ctx->draw.font);
        UnloadTexture(ctx->draw.aux_viewport.texture);
        UnloadTexture(ctx->draw.aux_viewport2.texture);
    }

    strbuf_destroy(&ctx->curr_project_file_path);
    for (int i = 0; i < ctx->actions.size; ++i) {
        strbuf_destroy(&ctx->actions.items[i].name);
    }
    Action_Dyna_free(&ctx->actions);

    {
        // Free spritesheet list.
        for (dyna_foreach(Spritesheet, i, ctx->spritesheet_list)) {
            Spritesheet *s = i.ref;
            strbuf_destroy(&s->path);
            if (IsImageValid(s->image)) { UnloadImage(s->image); }
            if (IsTextureValid(s->texture)) { UnloadTexture(s->texture); }
        }
        Spritesheet_Dyna_free(&ctx->spritesheet_list);
    }

    {
        // Free sprite list.
        for (int i = 0; i < ctx->sprites.size; ++i) {
            Sprite *sprite = &ctx->sprites.items[i];
            strbuf_destroy(&sprite->name);
        }
        Sprite_Dyna_free(&ctx->sprites);
    }

    int_Dyna_free(&ctx->editor.selected_sprites_cursor);
    int_Dyna_free(&ctx->editor.selected_sprites);
    ArenaRoot_free(&ctx->frame_arena.root);
}

typedef struct WidgetDraw {
    Rect2i area;
    bool focused;
} WidgetDraw;

typedef struct WidgetReq {
    bool success;
    bool request_focus_area;
    Rect2i focus_area;
} WidgetReq;

typedef struct Widget {
    WidgetDraw draw;
    Rect2i focus_area;
    void (*draw_function)(Ctx *ctx, const WidgetDraw widget, WidgetReq *req);
} Widget;

#define MAKEVIEW__TYPE Rect2i
#include "make_view.h"

#define MAKEVIEW__TYPE Widget
#include "make_view.h"

#endif // !STATE_H
