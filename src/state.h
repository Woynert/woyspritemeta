#ifndef STATE_H
#define STATE_H

#include "strbuf.h"
#include "raylib.h"
#include "zoompanel.h"

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
    Texture texture;
} Spritesheet;

#define DYNA__TYPE Action
#include "da.h"

#define DYNA__TYPE strbuf_wrap
#include "da.h"

#define DYNA__TYPE Spritesheet
#include "da.h"

typedef struct Draw {
    int line_height;
    int font_size;
    int char_spacing;
    int line_spacing;
    Font font;

    /* For when needing to draw to a "canvas" that's gonna be shown partially.*/
    RenderTexture2D aux_viewport;

} Draw;

typedef enum SHEETEDITOR_CURSOR {
    SHEETEDITOR_CURSOR_TWEAK,
    SHEETEDITOR_CURSOR_ADD,
    SHEETEDITOR_CURSOR_DELETE,
    SHEETEDITOR_CURSOR__COUNT,
} SHEETEDITOR_CURSOR;

typedef struct Ctx {
    Draw draw;

    bool has_project_file_open;
    strbuf_t *curr_project_file_path;

    Action_Dyna actions;
    Spritesheet_Dyna spritesheet_list;

    // Sheeteditor widget.

    Zoompanel zoompanel;
    SHEETEDITOR_CURSOR editor_curr_cursor;

} Ctx;

#endif // !STATE_H
