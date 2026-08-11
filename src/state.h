#ifndef STATE_H
#define STATE_H

#include "strbuf.h"
#include "raylib.h"
#include "zoompanel.h"
#include "arena.h"

#define EXTENSION ".spri.txt"

typedef struct Ctx Ctx;

typedef struct Action {
    strbuf_t *name;
    int (*op_ptr)(Ctx *ctx); /* @Returns Error. */
} Action;

typedef struct ActionLiteral {
    strview_t name;
    int (*op_ptr)(Ctx *ctx); /* @Returns Error. */
} ActionLiteral;

typedef struct SpritesheetFrame {
    strbuf_t *path;
    strbuf_t *relative_path; /* Relative from _Project File Dir_ to it. */
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
    int frames;
    strbuf_t *name;
} Sprite;

#define DYNA__TYPE Action
#define DYNA__NAMESPACE Vec_Action
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
    Texture splash_art;
} Draw;

typedef enum SHEETEDITOR_CURSOR {
    SHEETEDITOR_CURSOR_TWEAK,
    SHEETEDITOR_CURSOR_ADD,
    SHEETEDITOR_CURSOR_DRAG,
    SHEETEDITOR_CURSOR_RESIZE,
    SHEETEDITOR_CURSOR__COUNT,
} SHEETEDITOR_CURSOR;
#define SHEETEDITOR_CURSOR_DEFAULT SHEETEDITOR_CURSOR_TWEAK


typedef struct Project {
    strbuf_t *path_file; // Absolute.
    strview_t path_dir;  // Absolute.
    Vec_Spritesheet spritesheet_list;
} Project;


typedef struct Ctx {

    Draw draw;
    int ticks; // Frame counter since engine start.

    bool project_loaded;
    bool has_unsaved_changes;
    int curr_sheet_id;
    int curr_frame_id;
    Rect2i curr_sheet_size;
    int mouse_selected_spritesheet_id;

    // [ Arenas ]
    ArenaRoot frame_arena_root;
    Arena frame_arena;
    // [ !Arenas ]

    union {
        Project *project;
        Project *p;
    };

    // Floating menu.
    struct {
        bool open;
        Rect2i rect;
        Vec_Action actions;
    } menu;

    // Sheeteditor widget.
    struct {
        Zoompanel zoompanel;
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

} Ctx;

static bool WINDOW_SHOULD_CLOSE = false;

#endif // !STATE_H
