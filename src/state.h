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

    bool has_project_file_open;
    strbuf_t *curr_project_file_path;

    Action_Dyna actions;
    Vec_Spritesheet spritesheet_list; // A spritesheet vector with N items represents a spritesheet with N frames.

    int curr_spritesheet_id; // Spritesheet collection.
    int curr_spritesheet_frame_id;     // frame.
    Rect2i curr_spritesheet_rect;


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

    //Sprite_Dyna sprites;
    //int selected_sprite; // Instead use int_Dyna_get_safe(selected_sprites, 0);

    // [ Arenas ]
    struct {
        ArenaRoot root;
        Arena arena;
        strbuf_allocator_t strbuf_alloc;
    } frame_arena;
    // [ !Arenas ]
} Ctx;


typedef struct WidgetReq {
    bool   focus_area_request;
    bool   focus_area_success;
    Rect2i focus_area;

    bool scroll_max_px_request;
    bool scroll_max_px_success;
    int  scroll_max_px;
} WidgetReq;

typedef struct WidgetDraw {
    Rect2i area;
    bool focused;
    int scroll_px;
} WidgetDraw;

typedef enum WIDGET_TYPE {
    WIDGET_TYPE_NORMAL,
    WIDGET_TYPE_CONTAINER,
} WIDGET_TYPE;

typedef struct WidgetData {
    WIDGET_TYPE type;
    /*
    union {
        struct {
            int children_count;
            Widget *children;
        } split;
        float split;
        int mimo;
        float mimo2;
    };
    */
} WidgetData;

typedef struct Widget {
    Rect2i screen_area;
    Rect2i focus_area;
    bool focused;
    
    WidgetDraw draw_info; /* Passed to draw function. */

    void (*draw_function)(Ctx *ctx, const WidgetDraw widget, WidgetReq *req);

    bool scroll_enabled;
    int scroll_px;
    int scroll_max_px; /* Max height, reported by the widget draw_function through WidgetReq. */

    WidgetData widget_data;
} Widget;

//enum WIDEG

typedef struct WidgetMeta {
    WIDGET_TYPE type;
    union {
        struct {
            void (*draw_function)(Ctx *ctx, const WidgetDraw widget, WidgetReq *req);
            bool scroll_enabled;
        } normal;
        struct {
            void (*container_function)(Ctx *ctx, int child_count, Widget *children);
        } container;
    };
} WidgetMeta;

typedef enum CONTAINER_TYPE {
    CONTAINER_TYPE_SPLIT_H_PERCENTAGE,
    CONTAINER_TYPE_SPLIT_H_PIXEL,
} CONTAINER_TYPE;

typedef struct WidgetContainer {
    CONTAINER_TYPE type;
    union {
        struct {
            float percentage;
            bool adjustable; /* Allows you to adjust the amount of split */
        } split;
    };
} WidgetContainer;

#define MAKEVIEW__TYPE Rect2i
#include "make_view.h"

#define MAKEVIEW__TYPE Widget
#include "make_view.h"

#endif // !STATE_H
