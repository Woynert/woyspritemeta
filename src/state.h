#ifndef STATE_H
#define STATE_H

#include "strbuf.h"
#include "raylib.h"
#include "zoompanel.h"
#include "arena.h"

typedef struct Ctx Ctx;

typedef struct Action {
    strbuf_t *name;
    int (*op_ptr)(Ctx *ctx); /* @Returns Error. */
} Action;

typedef struct ActionLiteral {
    strview_t name;
    int (*op_ptr)(Ctx *ctx); /* @Returns Error. */
} ActionLiteral;

typedef struct strbuf_wrap {
    strbuf_t *str;
} strbuf_wrap;

typedef struct SpritesheetFrame {
    strbuf_t *path;
    Image image;     /* CPU side. */
    Texture texture; /* GPU side. */
} SpritesheetFrame;

#define DYNA__TYPE SpritesheetFrame
#define DYNA__NAMESPACE Vec_SpritesheetFrame
#include "da.h"


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

#define DYNA__TYPE Action
#define DYNA__NAMESPACE Vec_Action
#include "da.h"

#define DYNA__TYPE strbuf_wrap
#include "da.h"

#define DYNA__TYPE Sprite
#include "da.h"

#define DYNA__TYPE Sprite
#define DYNA__NAMESPACE Vec_Sprite
#include "da.h"

typedef struct Spritesheet {
    Vec_SpritesheetFrame frames;
    Vec_Sprite sprites;
    bool unfolded;
} Spritesheet;

#define DYNA__TYPE Spritesheet
#define DYNA__NAMESPACE Vec_Spritesheet
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
    int ticks; // Frame counter since engine start.

    // [SPRITESHEET]
    Vec_Spritesheet spritesheet_list; // A spritesheet vector with N items represents a spritesheet with N frames.
    int curr_sheet_id;    // Spritesheet collection.
    int curr_frame_id;    // frame.
    Rect2i curr_sheet_size;
    int mouse_selected_spritesheet_id;
    // ![SPRITESHEET]

    Texture splash_art;
    struct {
        bool loaded;
        strbuf_t *path;
        bool unsaved_changes;
    } project;

    // Floating menu.
    struct {
        bool open;
        Rect2i rect;
        Vec_Action actions;
    } menu;

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

    // [ Arenas ]
    ArenaRoot frame_arena_root;
    Arena frame_arena;
    // [ !Arenas ]
} Ctx;

#define MAKEVIEW__TYPE Rect2i
#include "make_view.h"

#endif // !STATE_H
