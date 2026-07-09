#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "state.h"
#include "tinyfiledialogs.h"
#include "portable_utils.h"
#include "strbuf_extra.h"
#include "cwalk.h"
#include "raylib.h"

int create_new_project(Ctx *ctx);

void _setup_ctx(Ctx *ctx);

int init_ctx(Ctx *ctx) {
    ctx->actions = Action_Dyna_create();
    ctx->spritesheet_list = Spritesheet_Dyna_create();
    ctx->curr_project_file_path = strbuf_create_empty(0, NULL);

    _setup_ctx(ctx);
    return 0;
}

void free_ctx(Ctx *ctx) {
    Action_Dyna_free(&ctx->actions);
    Spritesheet_Dyna_free(&ctx->spritesheet_list);
    strbuf_destroy(&ctx->curr_project_file_path);
}

int create_new_project(Ctx *ctx) {
    const char *file_patterns[] = { "*.wsp" };
    const char *path_result = tinyfd_saveFileDialog(
        "Save new project", NULL, 1, file_patterns, ".wsp");
    if (path_result == NULL) { return 0; }

    // TODO: Make sure new project file ends in .wsp

    ctx->has_project_file_open = true;
    strbuf_assign(&ctx->curr_project_file_path, cstr(path_result));
    printfd("New project file is %s", path_result);

    return 0;
}

void _add_spritesheet(Ctx *ctx, strview_t path, Texture texture) {
    Spritesheet s = { .path = strbuf_create(path, NULL), .texture = texture };
    Spritesheet_Dyna_append(&ctx->spritesheet_list, s);
}

void _clear_spritesheet_list(Ctx *ctx) {
    for (int i = 0; i < ctx->spritesheet_list.size; ++i) {
        Spritesheet *s = &ctx->spritesheet_list.items[i];
        strbuf_destroy(&s->path);
        if (IsTextureValid(s->texture)) {
            UnloadTexture(s->texture);
        }
    }
    Spritesheet_Dyna_clear_freeing(&ctx->spritesheet_list);
}


int open_image_as_spritesheet(Ctx *ctx) {
    const char *file_patterns[] = { "*.png" };
    const char *path_result = tinyfd_openFileDialog("Open image file", NULL, 1, file_patterns, ".png", 0);
    if (path_result == NULL) { return 0; }

    //_clear_spritesheet_list(ctx);

    strbuf_t *spritesheet = strbuf_create(cstr(path_result), NULL);

    // Check if file is valid.

    Texture texture = LoadTexture(spritesheet->cstr);
    if (!IsTextureValid(texture)) {
        goto exit;
    };

    printfd("Sucessfully loaded texture %"PRIstr".", PRIstrargbuf(spritesheet));

    // Add to list.

    _add_spritesheet(ctx, strbuf_view2(spritesheet), texture);

    //if (cwk_path_has_extension(spritesheet->cstr)) {

    /* If basename ends in 1 or 0 will automatically scan for files with similar
       naming. Example: sheet1.png will scans for sheet2.png sheet3.png sheet4.png */

        // TODO: Try and scan for other 
    //}

    return 0;

    exit:
    {
        strbuf_destroy(&spritesheet);

        return -1;
    }
}

int no_op(Ctx *ctx) { printfd("TODO"); return 0; }

void call_action(Ctx *ctx, Action *action) {
    printfd("[%"PRIstr"]", PRIstrarg(strbuf_view2(action->name)));
    (*action->op_ptr)(ctx);
}

/// @Returns Font. Can fail, check with IsFontValid(...).
Font load_font_with_buncha_codepoints(const char* font_path, int font_size) {

    // Ranges are inclusive.
    int ranges[] = {
        0xFFFD,  0xFFFD,  // (�) codepoint
        32,      127,     // Basic latin
        0x00A1,  0x00FF,  // C1 Controls and Latin-1 Supplement
        0x0100,  0x017F,  // Latin Extended-A
        0x0180,  0x024F,  // Latin Extended-B
        0x1F300, 0x1F5FF, // Miscellaneous Symbols and Pictographs
        0x1F600, 0x1F64F, // Emoticons
    };

    int range_amount = (int)(sizeof(ranges)/sizeof(ranges[0]));
    int total_codepoints = 0;
    for (int i = 0; i < range_amount; i += 2) {
        total_codepoints += ranges[i+1] - ranges[i] +1; // Inclusive
    }

    int *codepoints = (int*)malloc((size_t)total_codepoints * sizeof(int));
    int codepoint_count = 0;

    for (int i = 0; i < range_amount; i += 2) {
        for (int j = ranges[i]; j <= ranges[i+1]; ++j) {
            codepoints[codepoint_count] = j;
            ++codepoint_count;
        }
    }

    ASSERT(codepoint_count == total_codepoints);
    Font font = LoadFontEx(font_path, (int)font_size, codepoints, (int)codepoint_count);
    free(codepoints);
    return font;
}

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

    action = (Action) { strbuf_create(0, NULL), open_image_as_spritesheet };
    strbuf_assign(&action.name, cstr_SL("Load image as spritesheet"));
    Action_Dyna_append(&ctx->actions, action);
}

void ctx_load_assets(Ctx *ctx) {

    // Draw
    ctx->draw.font_size = 18;
    ctx->draw.line_spacing = 2;
    ctx->draw.char_spacing = 0;
    ctx->draw.line_height = ctx->draw.font_size +ctx->draw.line_spacing;
    ctx->draw.font = load_font_with_buncha_codepoints(
            "assets/Roboto-Regular.ttf", ctx->draw.font_size);
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
