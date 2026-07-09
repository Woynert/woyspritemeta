#ifndef STATE_H
#define STATE_H

#include "strbuf.h"

typedef struct Ctx Ctx;

typedef struct Action {
    strbuf_t *name;
    int (*op_ptr)(Ctx *ctx); /* @Returns Error. */
} Action;

typedef struct strbuf_wrap {
    strbuf_t *str;
} strbuf_wrap;

#define DYNA__TYPE Action
#include "da.h"

#define DYNA__TYPE strbuf_wrap
#include "da.h"

typedef struct Draw {
    int line_height;
    int font_size;
} Draw;

typedef struct Ctx {
    Draw draw; // Draw context.
    Action_Dyna actions;
    strbuf_wrap_Dyna spritesheet_paths;
} Ctx;

#endif // !STATE_H
