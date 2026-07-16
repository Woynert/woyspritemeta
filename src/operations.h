#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "state.h"
#include "tinyfiledialogs.h"
#include "portable_utils.h"
#include "strbuf_extra.h"
#include "strnum.h"
#include "cwalk.h"
#include "raylib.h"


int init_ctx(Ctx *ctx);
void free_ctx(Ctx *ctx);
int create_new_project(Ctx *ctx);
int open_image_as_spritesheet(Ctx *ctx);
void call_action(Ctx *ctx, Action *action);
Font load_font_with_buncha_codepoints(const char* font_path, int font_size);
void ctx_load_assets(Ctx *ctx);
int no_op(Ctx *ctx) { printfd("TODO"); return 0; }

void _setup_ctx(Ctx *ctx);
void _add_spritesheet(Ctx *ctx, strview_t path, Image image, Texture texture);
void _clear_spritesheet_list(Ctx *ctx);
int _try_load_image_as_spritesheet(Ctx *ctx, strview_t file_path);

int init_ctx(Ctx *ctx) {
    _init_ctx(ctx);
    _setup_ctx(ctx);
    return 0;
}

void free_ctx(Ctx *ctx) {
    _free_ctx(ctx);
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

    zoompanel_init(&ctx->zoompanel, ZOOMPANEL_CONF_PIXEL_PERFECT, MOUSE_BUTTON_RIGHT);
}

void ctx_load_assets(Ctx *ctx) {

    // Draw
    ctx->draw.font_size = 18;
    ctx->draw.line_spacing = 2;
    ctx->draw.char_spacing = 0;
    ctx->draw.line_height = ctx->draw.font_size +ctx->draw.line_spacing;
    ctx->draw.font = load_font_with_buncha_codepoints(
            "assets/Roboto-Regular.ttf", ctx->draw.font_size);

    ctx->draw.aux_viewport = LoadRenderTexture(GetMonitorWidth(0), GetMonitorHeight(0));
    ctx->draw.aux_viewport2 = LoadRenderTexture(GetMonitorWidth(0), GetMonitorHeight(0));

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

void _add_spritesheet(Ctx *ctx, strview_t path, Image image, Texture texture) {
    Spritesheet s = {
        .path = strbuf_create(path, NULL),
        .image = image,
        .texture = texture,
    };
    Spritesheet_Dyna_append(&ctx->spritesheet_list, s);
}

void _clear_spritesheet_list(Ctx *ctx) {
    for (int i = 0; i < ctx->spritesheet_list.size; ++i) {
        Spritesheet *s = &ctx->spritesheet_list.items[i];
        strbuf_destroy(&s->path);
        if (IsImageValid(s->image)) { UnloadImage(s->image); }
        if (IsTextureValid(s->texture)) { UnloadTexture(s->texture); }
    }
    Spritesheet_Dyna_clear_freeing(&ctx->spritesheet_list);
}


/// @returns Error.
int _try_load_image_as_spritesheet(Ctx *ctx, strview_t file_path) {
    int return_err = 0;

    strbuf_t *aux_str = strbuf_create(file_path, NULL); // Raylib only accepts cstrings :'(

    Image image;
    Texture texture;

    image = LoadImage(aux_str->cstr);
    if (!IsImageValid(image)) {
        printfd("WAR: Couldn't load image [%"PRIstr"].", PRIstrarg(file_path));
        return_err = -1;
        goto exit_cleanup;
    }

    if (image.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        printfd("WAR: Image isn't R8G8B8A8, converting...");
        ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    }

    texture = LoadTextureFromImage(image);
    if (!IsTextureValid(texture)) {
        printfd("WAR: Couldn't load texture [%"PRIstr"].", PRIstrarg(file_path));
        return_err = -1;
        goto exit_cleanup;
    };

    printfd("Sucessfully loaded texture %"PRIstr".", PRIstrarg(file_path));

    _add_spritesheet(ctx, file_path, image, texture);

    exit_cleanup:
    if ((0)) {
        UnloadImage(image);
        UnloadTexture(texture);
    }

    strbuf_destroy(&aux_str);
    return return_err;
}


int open_image_as_spritesheet(Ctx *ctx) {

    const char *file_patterns[] = { "*.png" };
    const char *path_result_cstr = tinyfd_openFileDialog("Open image file", NULL, 1, file_patterns, ".png", 0);
    if (path_result_cstr == NULL) {
        return -1;
    }
    strview_t path = cstr(path_result_cstr);

    _clear_spritesheet_list(ctx);

    int err = _try_load_image_as_spritesheet(ctx, path);
    if (err != 0) {
        return -1;
    }

    // Save dimensions

    ctx->spritesheet_image_rect.pos = v2ii(0);
    ctx->spritesheet_image_rect.width = ctx->spritesheet_list.items[0].texture.width;
    ctx->spritesheet_image_rect.height = ctx->spritesheet_list.items[0].texture.height;

    strview_t base = path;
    strview_t extension = { 0 };

    // Extract extension and remove it from base

    if (cwk_path_has_extension(base.data)) {
        size_t size;
        cwk_path_get_extension(base.data, &extension.data, &size);
        extension.size = (int)size;

        base.size -= extension.size;
    }

    // Check if basename ends in a single digit number.

    strview_t digit_str = strnum_get_all_trailing_digits(base);
    int digit = strnum_int(digit_str, -1, STRNUM_DEFAULT);
    if (digit == -1) {
        return 0;
    }

    base.size -= digit_str.size;

    if ((0)) {
        printfd("Basename [%"PRIstr"] Extension [%"PRIstr"]", PRIstrarg(base), PRIstrarg(extension));
        printfd("basename_digit_str [%"PRIstr"] basename_digit [%d]", PRIstrarg(digit_str), digit);
    }

    // Traverse frame files.
    // Check for next frame --> "file_name[digit +1].png"
    // @note: Currently we don't support zero padded digits like 001 002 003.

    {
        strbuf_t *possible_file_path = strbuf_create(0, NULL);

        for (;;) {
            ++digit;

            strbuf_printf(&possible_file_path, "%"PRIstr"%d%"PRIstr"",
                    PRIstrarg(base), digit, PRIstrarg(extension));

            err = _try_load_image_as_spritesheet(ctx, strbuf_view2(possible_file_path));
            if (err != 0) { break; }
        }

        strbuf_destroy(&possible_file_path);
    }

    return 0;
}

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

    wassert(codepoint_count == total_codepoints);
    Font font = LoadFontEx(font_path, (int)font_size, codepoints, (int)codepoint_count);
    free(codepoints);
    return font;
}


void register_sprite(Ctx *ctx, Rect2i rect) {
    if (Rect2i_is_out_of_bounds(rect, ctx->spritesheet_image_rect)) {
        return;
    }

    printfd("SAVING SPRITE "V2i_Fmt, V2i_Arg(rect.size));
    Sprite sprite = {
        .rect = rect,
        .name = strbuf_create_empty(0, NULL),
        .frames = 1,
    };
    Sprite_Dyna_append(&ctx->sprites, sprite);
}


void remove_sprite(Ctx *ctx, int index) {
    Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, index);
    if (sprite == NULL) { return; }
    strbuf_destroy(&sprite->name);
    Sprite_Dyna_pop_at_preserve_order(&ctx->sprites, index, NULL);
}


void spritesheet_select_append(Ctx *ctx, Rect2i selection) {
    int_Dyna_clear_preserving(&ctx->editor.selected_sprites_cursor);

    for (dyna_foreach(Sprite, i, ctx->sprites)) {
        if (Rect2i_collides_Rect2i(selection, i.ref->rect)) {
            int_Dyna_append(&ctx->editor.selected_sprites_cursor, i.index);
        }
    }
}

void spritesheet_clear_selection(Ctx *ctx) {
    int_Dyna_clear_preserving(&ctx->editor.selected_sprites);
}

void spritesheet_commit_selection(Ctx *ctx) {
    for (dyna_foreach(int, i, ctx->editor.selected_sprites_cursor)) {
        if (!int_Dyna_has(&ctx->editor.selected_sprites, *i.ref)) {
            int_Dyna_append(&ctx->editor.selected_sprites, *i.ref);
        }
    }

    int_Dyna_clear_preserving(&ctx->editor.selected_sprites_cursor);
}

void spritesheet_select_toggle(Ctx *ctx, V2i point) {
    int_Dyna_clear_preserving(&ctx->editor.selected_sprites_cursor);

    // Simple click -> Toggle.
    for (dyna_foreach(Sprite, i, ctx->sprites)) {
        if (Rect2i_collides_Rect2i((Rect2i){.pos=point, .size=v2ii(1)}, i.ref->rect)) {
            int out_i = -1;
            if (int_Dyna_find(&ctx->editor.selected_sprites, i.index, &out_i)) {
                int_Dyna_remove_at(&ctx->editor.selected_sprites, out_i);
            } else {
                int_Dyna_append(&ctx->editor.selected_sprites, i.index);
            }
        }
    }
}


void editor_cancel_drag(Ctx *ctx);

void spritesheet_try_set_cursor_mode(Ctx *ctx, SHEETEDITOR_CURSOR new_mode);

void spritesheet_reset_cursor_mode(Ctx *ctx) {
    spritesheet_try_set_cursor_mode(ctx, SHEETEDITOR_CURSOR_TWEAK);
}

/// @retval NULL. Not found.
Sprite *try_get_first_selected_sprite(Ctx *ctx) {
    int* sprite_id = int_Dyna_get_safe(&ctx->editor.selected_sprites, 0);
    if (sprite_id == NULL) { return NULL; }
    return Sprite_Dyna_get_safe(&ctx->sprites, *sprite_id);
}


void spritesheet_try_set_cursor_mode(Ctx *ctx, SHEETEDITOR_CURSOR new_mode) {
    // Requisites.
    switch (new_mode) {
        case SHEETEDITOR_CURSOR_MOVE:
        {
            if (ctx->editor.selected_sprites.size <= 0) { return; }
            break;
        }
        case SHEETEDITOR_CURSOR_RESIZE:
        {
            if (ctx->editor.selected_sprites.size != 1) { return; }
            break;
        }
        case SHEETEDITOR_CURSOR_TWEAK:
        case SHEETEDITOR_CURSOR_ADD:
        case SHEETEDITOR_CURSOR__COUNT: { break; }
    }
    // Cleanup.
    switch (ctx->editor.cursor) {
        case SHEETEDITOR_CURSOR_TWEAK: { break; }
        case SHEETEDITOR_CURSOR_ADD: { break; }
        case SHEETEDITOR_CURSOR_MOVE: { break; }
        case SHEETEDITOR_CURSOR_RESIZE: { break; }
        case SHEETEDITOR_CURSOR__COUNT: { break; }
    }
    // Setup.
    switch (new_mode) {
        case SHEETEDITOR_CURSOR_TWEAK: { break; }
        case SHEETEDITOR_CURSOR_ADD:
        {
            spritesheet_clear_selection(ctx);
            break;
        }
        case SHEETEDITOR_CURSOR_MOVE:
        {
            ctx->editor.drag_prev_mouse_pos = ctx->editor.mouse_pos;
            ctx->editor.drag_origin = ctx->editor.mouse_pos;
            break;
        }
        case SHEETEDITOR_CURSOR_RESIZE:
        {
            Sprite* sprite = try_get_first_selected_sprite(ctx);
            if (sprite == NULL) { return; }
            ctx->editor.is_selecting = true;
            ctx->editor.selection_origin = sprite->rect.pos;
            break;
        }
        case SHEETEDITOR_CURSOR__COUNT: { break; }
    }
    ctx->editor.cursor = new_mode;
}

/*
   Called every frame.
   Sometimes you get events like mouse pressed or released. To check hold try ctx->editor.is_selecting.
   */
void editor_process_cursor_logic(Ctx *ctx) {
    bool pressed_inside = ctx->editor.mouse_inside && BetterMouse_is_pressed(MOUSE_BUTTON_LEFT);
    bool released_inside = ctx->editor.mouse_inside && BetterMouse_is_released(MOUSE_BUTTON_LEFT);
    Rect2i selection = ctx->editor.selection;

    switch(ctx->editor.cursor) {
    case SHEETEDITOR_CURSOR_TWEAK:
    {
        if (IsKeyPressed(KEY_G)) {
            spritesheet_try_set_cursor_mode(ctx, SHEETEDITOR_CURSOR_MOVE);
            return;
        }
        if (IsKeyPressed(KEY_S)) {
            spritesheet_try_set_cursor_mode(ctx, SHEETEDITOR_CURSOR_RESIZE);
            return;
        }

        if (pressed_inside) {
            bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            if (!shift) { spritesheet_clear_selection(ctx); }
        }
        if (ctx->editor.is_selecting) {
            spritesheet_select_append(ctx, selection);
        }
        if (released_inside) {
            if (v2i_eq(ctx->editor.selection_origin, ctx->editor.mouse_pos)) {
                spritesheet_select_toggle(ctx, ctx->editor.mouse_pos);
            } else {
                spritesheet_commit_selection(ctx);
            }
        }
        break;
    }
    case SHEETEDITOR_CURSOR_ADD:
    {
        if (!ctx->editor.is_selecting) { return; }
        if (released_inside) {
            if (selection.width > 1 && selection.height > 1) {
                register_sprite(ctx, selection);
            }
        }
        break;
    }
    case SHEETEDITOR_CURSOR_MOVE:
    {

        // Cancel.
        if (BetterMouse_is_pressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) {
            editor_cancel_drag(ctx);
            spritesheet_reset_cursor_mode(ctx);
            return;
        }
        // Commit.
        if (BetterMouse_is_pressed(MOUSE_BUTTON_LEFT)) {
            spritesheet_reset_cursor_mode(ctx);
            return;
        }

        // Update transform on mouse move.
        if (v2i_eq(ctx->editor.drag_prev_mouse_pos, ctx->editor.mouse_pos)) { return; }

        V2i delta = v2i_sub(ctx->editor.mouse_pos, ctx->editor.drag_prev_mouse_pos);
        ctx->editor.drag_prev_mouse_pos = ctx->editor.mouse_pos;

        for (dyna_foreach(int, sprite_id, ctx->editor.selected_sprites)) {
            Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, *sprite_id.ref);
            if (sprite == NULL) {
                printfd("ERR: Sprite not found."); continue;
            }
            sprite->offset = v2i_add(sprite->offset, delta);
        }
        break;
    }
    case SHEETEDITOR_CURSOR_RESIZE:
    {
        // Cancel.
        if (IsKeyPressed(KEY_ESCAPE) || BetterMouse_is_pressed(MOUSE_BUTTON_RIGHT)) {
            spritesheet_reset_cursor_mode(ctx);
            return;
        }
        // Commit.
        if (pressed_inside) {
            Sprite* sprite = try_get_first_selected_sprite(ctx);
            if (sprite == NULL) { return; }
            sprite->rect = selection;

            spritesheet_reset_cursor_mode(ctx);
        }
        break;
    }
    case SHEETEDITOR_CURSOR__COUNT: { break; } }
}


void editor_cancel_drag(Ctx *ctx) {
    if (ctx->editor.cursor != SHEETEDITOR_CURSOR_MOVE) { return; }
    spritesheet_reset_cursor_mode(ctx);

    // Reset original positions.

    V2i delta = v2i_sub(ctx->editor.drag_origin, ctx->editor.drag_prev_mouse_pos);

    for (dyna_foreach(int, sprite_id, ctx->editor.selected_sprites)) {
        Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, *sprite_id.ref);
        if (sprite == NULL) { printfd("ERR: Sprite not found."); continue; }
        sprite->offset = v2i_add(sprite->offset, delta);
    }
}

void editor_process_delete(Ctx *ctx) {
    if (!IsKeyPressed(KEY_X)) { return; }
    if (ctx->editor.cursor != SHEETEDITOR_CURSOR_TWEAK) { return; }
    if (ctx->editor.selected_sprites.size <= 0) { return; }

    // TODO: This dance is ugly... Modify strbuf_t struct?
    Arena arena = ctx->frame_arena.arena;
    strbuf_allocator_t arena_allocator = make_arena_strbuf_allocator(&arena);
    strbuf_t *delete_msg = strbuf_create_empty(0, &arena_allocator);
    strbuf_printf(&delete_msg, "Delete %d items? No uno.", ctx->editor.selected_sprites.size);

    int confirm_delete = tinyfd_messageBox("DELETE?", delete_msg->cstr, "yesno", "warning", 0);
    if (confirm_delete == 0) { return; }

    int_Dyna_sort(&ctx->editor.selected_sprites);

    // Ensure ids are aligned.
    int prev_id = INT_MIN;
    for (dyna_foreach(int, sprite_id, ctx->editor.selected_sprites)) {
        if (prev_id > *sprite_id.ref) {
            printfd("ERR: Ids are not aligned."); return;
        }
        prev_id = *sprite_id.ref;
    }

    // Delete sprites
    for (int i = ctx->editor.selected_sprites.size-1; i > -1; i -= 1) {
        int sprite_id = ctx->editor.selected_sprites.items[i];
        remove_sprite(ctx, sprite_id);
    }

    spritesheet_clear_selection(ctx);
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
