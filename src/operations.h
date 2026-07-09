#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "state.h"
#include "tinyfiledialogs.h"
#include "portable_utils.h"
#include "strbuf_extra.h"

int create_new_project(Ctx *ctx);

void _setup_ctx(Ctx *ctx);

int init_ctx(Ctx *ctx) {
    ctx->actions = Action_Dyna_create();
    ctx->spritesheet_paths = strbuf_wrap_Dyna_create();

    _setup_ctx(ctx);
    return 0;
}

void free_ctx(Ctx *ctx) {
    Action_Dyna_free(&ctx->actions);
    strbuf_wrap_Dyna_free(&ctx->spritesheet_paths);
}

int create_new_project(Ctx *ctx) {
    const char *file_patterns[] = { "*.wsp" };
    const char *path_result = tinyfd_saveFileDialog(
        "Save new project", NULL, 1, file_patterns, "woyspritemeta Project");
    printfd("Result is %s", path_result);
    if (path_result == NULL) { return 0; }
    return 0;
}

int no_op(Ctx *ctx) { printfd("TODO"); return 0; }

void _setup_ctx(Ctx *ctx) {
    // Actions
    Action action;

    action = (Action) { strbuf_create(0, NULL), create_new_project };
    strbuf_assign(&action.name, cstr_SL("New Project"));
    Action_Dyna_append(&ctx->actions, action);

    action = (Action) { strbuf_create(0, NULL), no_op };
    strbuf_assign(&action.name, cstr_SL("Open Project"));
    Action_Dyna_append(&ctx->actions, action);
    
    action = (Action) { strbuf_create(0, NULL), no_op };
    strbuf_assign(&action.name, cstr_SL("Save Project"));
    Action_Dyna_append(&ctx->actions, action);

    action = (Action) { strbuf_create(0, NULL), no_op };
    strbuf_assign(&action.name, cstr_SL("Load image as spritesheet"));
    Action_Dyna_append(&ctx->actions, action);

    // Draw
    ctx->draw.font_size = 15;
    ctx->draw.line_height = 17;
}

void call_action(Ctx *ctx, Action *action) {
    printfd("[%"PRIstr"]", PRIstrarg(strbuf_view2(action->name)));
    (*action->op_ptr)(ctx);
}


//void save_new

//void open_new_file(Ctx *ctx) {
    //tinyfd_openFileDialog(
        //NULL,
        //NULL, // TODO: Set to current project path.
    
//}

/*
...


typedef enum EVENT {
    EVENT_SYMBOL_QUERY,
    EVENT_MAX,
} EVENT;

typedef struct EventSymbolQuery {
    uint symbol_node_id;
} EventSymbolQuery;

typedef struct Event {
    EVENT type;
    union {
        EventSymbolQuery event_symbol_query;
    };
} Event;

...

void WuiState_queue_event_symbol_query(WuiState *w, EventSymbolQuery event) {
    Event_da_append(&w->events, (Event){ .type = EVENT_SYMBOL_QUERY, .event_symbol_query = event });
}

...

            if (BetterMouse_is_pressed(MOUSE_LEFT_BUTTON)) {
                WuiState_queue_event_symbol_query(&ctx->wui_state,
                    (EventSymbolQuery) {
                        .symbol_node_id = it.node_id,
                    }
                );
   */

#endif // !OPERATIONS_H
