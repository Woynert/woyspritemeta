#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "state.h"
#include "state_init.h"
#include "tinyfiledialogs.h"
#include "portable_utils.h"
#include "strbuf_extra.h"
#include "strnum.h"
#include "wstrview.h"
#include "cwalk.h"
#include "raylib.h"
#include "ui_mouse_input.h"


int init_ctx(Ctx *ctx);
void free_ctx(Ctx *ctx);
int create_new_project(Ctx *ctx);
void call_action(Ctx *ctx, Action *action);
Font load_font_with_buncha_codepoints(const char* font_path, int font_size);
void ctx_load_assets(Ctx *ctx);
int no_op(Ctx *ctx) { printfd("TODO"); return 0; }

void _setup_ctx(Ctx *ctx);
void _add_spritesheet(Ctx *ctx, strview_t path, Image image, Texture texture);


int init_ctx(Ctx *ctx) {
    _ctx_init(ctx);
    _setup_ctx(ctx);
    return 0;
}

void free_ctx(Ctx *ctx) {
    _ctx_free(ctx);
}

void _setup_ctx(Ctx *ctx) {
    zoompanel_init(&ctx->zoompanel, ZOOMPANEL_CONF_PIXEL_PERFECT, MouseRight);
}

void ctx_load_assets(Ctx *ctx) {

    // Draw
    ctx->draw.font_size = 18;
    ctx->draw.line_spacing = 0;
    ctx->draw.char_spacing = 0;
    ctx->draw.line_height = ctx->draw.font_size +ctx->draw.line_spacing +2;
    ctx->draw.font = load_font_with_buncha_codepoints(
            "assets/Roboto-Regular.ttf", ctx->draw.font_size);
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

void spritesheet_clear_selection(Ctx *ctx) {
    int_Dyna_clear_preserving(&ctx->editor.selected_sprites);
    int_Dyna_clear_preserving(&ctx->editor.selected_sprites_cursor);
}

void editor_reset_selection(Ctx *ctx) {
    spritesheet_clear_selection(ctx);
    ctx->editor.mouse_is_selecting = false;
    ctx->editor.add_can_undo = false;
}


/// @Returns NULL of not found.
Spritesheet *get_current_spritesheet(Ctx *ctx) {
    return Vec_Spritesheet_get_safe(&ctx->spritesheet_list, ctx->curr_sheet_id);
}

//void select_spritesheet(Ctx *ctx, int sheet_id) {
    //Spritesheet *sheet = Vec_Spritesheet_get_safe(&ctx->spritesheet_list, sheet_id);
    //if (!sheet) { return; }
    //ctx->curr_spritesheet_id = sheet_id;
    //ctx->curr_spritesheet_id = sheet_id;
//}

//void get_spritesheet_frame(Ctx *ctx, int sheet_id, int frame_id) {
    //Spritesheet *sheet = Vec_Spritesheet_get_safe(&ctx->spritesheet_list, sheet_id);
    //if (sheet == NULL) { return; }
    //SpritesheetFrame *frame = Vec_SpritesheetFrame_get_safe(&sheet->frames, frame_id);
    //if (frame == NULL) { return; }

void select_spritesheet_frame(Ctx *ctx, int sheet_id, int frame_id) {
    Spritesheet *sheet = Vec_Spritesheet_get_safe(&ctx->spritesheet_list, sheet_id);
    if (sheet == NULL) { return; }
    SpritesheetFrame *frame = Vec_SpritesheetFrame_get_safe(&sheet->frames, frame_id);
    if (frame == NULL) { return; }
    if (ctx->curr_sheet_id != sheet_id) {
        editor_reset_selection(ctx);
    }
    ctx->curr_sheet_id = sheet_id;
    ctx->curr_frame_id = frame_id;
    ctx->curr_sheet_size = (Rect2i) {
        .pos = v2ii(0),
        .size = v2i(frame->texture.width, frame->texture.height),
    };
}

SpritesheetFrame *get_selected_spritesheet_frame(Ctx *ctx) {
    Spritesheet *sheet = Vec_Spritesheet_get_safe(&ctx->spritesheet_list, ctx->curr_sheet_id);
    if (sheet == NULL) { return NULL; }
    return Vec_SpritesheetFrame_get_safe(&sheet->frames, ctx->curr_frame_id);
}


/// @Returns NULL if Spritesheet doesn't exist.
Spritesheet *spritesheet_get_if_exists(Ctx *ctx, strview_t path, int *out_id) {
    for (dyna_foreach(Spritesheet, kter, ctx->spritesheet_list)) {
        Spritesheet *sheet = kter.ref;
        for (dyna_foreach(SpritesheetFrame, iter, sheet->frames)) {
            SpritesheetFrame *frame = iter.ref;
            if (wstrview_equals(path, strbuf_view2(frame->path))) {
                if (out_id != NULL) { *out_id = kter.index; }
                return sheet;
            }
        }
    }
    return NULL;
}


int open_image_as_spritesheet(Ctx *ctx, strview_t path) {

    // If spritesheet already exists then free it and rebuild.

    int sheet_id;
    Spritesheet *new_sheet = spritesheet_get_if_exists(ctx, path, &sheet_id);
    if (new_sheet != NULL) { Spritesheet_clear_frames(new_sheet); }

    // Load from path.

    SpritesheetFrame frame = {0};
    int err = SpritesheetFrame_make(path, &frame);
    if (err != 0) { return -1; }
    if (new_sheet == NULL) {
        Spritesheet _new_sheet = Spritesheet_make();
        sheet_id = Vec_Spritesheet_append(&ctx->spritesheet_list, _new_sheet);
        new_sheet = Vec_Spritesheet_get_safe(&ctx->spritesheet_list, sheet_id);
        if (new_sheet == NULL) { return -1; }
    }
    Vec_SpritesheetFrame_append(&new_sheet->frames, frame);

    // Get base and extension.

    strview_t base = path;
    strview_t extension = { 0 };

    if (cwk_path_has_extension(base.data)) {
        size_t size;
        cwk_path_get_extension(base.data, &extension.data, &size);
        extension.size = (int)size;
        base.size -= extension.size;
    }

    // Check if basename ends in a single digit number.

    strview_t digit_str = strnum_get_all_trailing_digits(base);
    int digit = strnum_int(digit_str, -1, STRNUM_DEFAULT);
    if (digit == -1) { goto commit; }
    base.size -= digit_str.size;

    // Traverse frame files.
    // Check for next frame --> "file_name[digit +1].png"
    // @note: Currently we don't support zero padded digits like 001 002 003.

    if ((0)) {
        printfd("Basename [%"PRIstr"] Extension [%"PRIstr"]", PRIstrarg(base), PRIstrarg(extension));
        printfd("basename_digit_str [%"PRIstr"] basename_digit [%d]", PRIstrarg(digit_str), digit);
    }

    {
        Arena arena = ctx->frame_arena.arena;
        strbuf_t *possible_file_path = strbuf_create_with_arena(0, &arena);

        for (;;) {
            ++digit;
            strbuf_printf(&possible_file_path, PRIstrw"%d"PRIstrw, PRIstrarg(base), digit, PRIstrarg(extension));
            err = SpritesheetFrame_make(strbuf_view2(possible_file_path), &frame);
            if (err != 0) { break; }
            Vec_SpritesheetFrame_append(&new_sheet->frames, frame);
        }
    }

    // Finally commit.

    commit:
    {
        select_spritesheet_frame(ctx, sheet_id, 0); // TODO: Auto select this new one.
    }
    return 0;
}


int open_image_as_spritesheet_file_dialog(Ctx *ctx) {
    const char *file_patterns[] = { "*.png" };
    const char *path_result_cstr = tinyfd_openFileDialog("Open image file", NULL, 1, file_patterns, ".png", 0);
    if (path_result_cstr == NULL) { return -1; }
    strview_t path = cstr(path_result_cstr);
    return open_image_as_spritesheet(ctx, path);
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
    if (Rect2i_is_out_of_bounds(rect, ctx->curr_sheet_size)) {
        return;
    }

    Spritesheet *sheet = get_current_spritesheet(ctx);
    if (sheet == NULL) { return; }

    printfd("SAVING SPRITE "V2i_Fmt, V2i_Arg(rect.size));
    Sprite sprite = {
        .rect = rect,
        .name = strbuf_create_empty(0, NULL),
        .frames = 1,
    };
    Vec_Sprite_append(&sheet->sprites, sprite);
}


void remove_sprite(Ctx *ctx, int index) {
    Spritesheet *sheet = get_current_spritesheet(ctx);
    if (sheet == NULL) { return; }
    Sprite *sprite = Vec_Sprite_get_safe(&sheet->sprites, index);
    if (sprite == NULL) { return; }
    strbuf_destroy(&sprite->name);
    Vec_Sprite_pop_at_preserve_order(&sheet->sprites, index, NULL);

    /*
       @Note: Every time the ctx->sprites list is modified
       ctx->editor.selected_sprites should be cleared.
    */
}


bool sprite_name_already_exists(Ctx *ctx, strview_t name) {
    Spritesheet *sheet = get_current_spritesheet(ctx);
    if (sheet == NULL) { return false; }
    for (dyna_foreach(Sprite, sprite, sheet->sprites)) {
        if (strview_equal(name, strbuf_view2(sprite.ref->name))) {
            return true;
        }
    }
    return false;
}


void spritesheet_select_append(Ctx *ctx, Rect2i selection) {
    int_Dyna_clear_preserving(&ctx->editor.selected_sprites_cursor);
    Spritesheet *sheet = get_current_spritesheet(ctx);
    if (sheet == NULL) { return; }
    for (dyna_foreach(Sprite, i, sheet->sprites)) {
        if (Rect2i_collides_Rect2i(selection, i.ref->rect)) {
            int_Dyna_append(&ctx->editor.selected_sprites_cursor, i.index);
        }
    }
}

int *get_selected_sprite(Ctx *ctx) {
    if (ctx->editor.selected_sprites.size != 1) { return NULL; }
    return int_Dyna_get_safe(&ctx->editor.selected_sprites, 0);
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
    Spritesheet *sheet = get_current_spritesheet(ctx);
    if (sheet == NULL) { return; }
    // Simple click -> Toggle.
    for (dyna_foreach(Sprite, i, sheet->sprites)) {
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
    Spritesheet *sheet = get_current_spritesheet(ctx);
    if (sheet == NULL) { return NULL; }
    return Vec_Sprite_get_safe(&sheet->sprites, *sprite_id);
}


void spritesheet_try_set_cursor_mode(Ctx *ctx, SHEETEDITOR_CURSOR new_mode) {
    // Requisites to switch.
    switch (new_mode) {
        case SHEETEDITOR_CURSOR_DRAG:
        {
            if (ctx->editor.selected_sprites.size <= 0) {
                new_mode = SHEETEDITOR_CURSOR_DEFAULT;
            }
            break;
        }
        case SHEETEDITOR_CURSOR_RESIZE:
        {
            if (ctx->editor.selected_sprites.size != 1) {
                new_mode = SHEETEDITOR_CURSOR_DEFAULT;
            }
            break;
        }
        case SHEETEDITOR_CURSOR_TWEAK:
        case SHEETEDITOR_CURSOR_ADD:
        case SHEETEDITOR_CURSOR__COUNT: { break; }
    }

    // Cleanup old mode.
    switch (ctx->editor.cursor) {
        case SHEETEDITOR_CURSOR_TWEAK: { break; }
        case SHEETEDITOR_CURSOR_ADD: { break; }
        case SHEETEDITOR_CURSOR_DRAG: { break; }
        case SHEETEDITOR_CURSOR_RESIZE: { break; }
        case SHEETEDITOR_CURSOR__COUNT: { break; }
    }

    // Reset state
    ctx->editor.cursor              = new_mode;
    ctx->editor.selection_origin    = (V2i) { 0 };
    ctx->editor.mouse_is_selecting  = false;
    ctx->editor.selection           = (Rect2i) { 0 };
    ctx->editor.drag_origin         = (V2i) { 0 };
    ctx->editor.drag_prev_mouse_pos = (V2i) { 0 };
    ctx->editor.add_can_undo        = false;

    // Setup new mode.
    switch (ctx->editor.cursor) {
        case SHEETEDITOR_CURSOR_TWEAK: { break; }
        case SHEETEDITOR_CURSOR_ADD:
        {
            spritesheet_clear_selection(ctx);
            break;
        }
        case SHEETEDITOR_CURSOR_DRAG:
        {
            ctx->editor.drag_prev_mouse_pos = ctx->editor.mouse_pos;
            ctx->editor.drag_origin = ctx->editor.mouse_pos;
            break;
        }
        case SHEETEDITOR_CURSOR_RESIZE:
        {
            Sprite* sprite = try_get_first_selected_sprite(ctx);
            if (sprite == NULL) { return; }
            ctx->editor.mouse_is_selecting = true;
            ctx->editor.selection_origin = sprite->rect.pos;
            break;
        }
        case SHEETEDITOR_CURSOR__COUNT: { break; }
    }
}


void editor_process_cursor_mode_logic(Ctx *ctx) {
    bool mouse_pressed = mice_pressed(MouseLeft);
    bool mouse_released = mice_released(MouseLeft);
    bool mouse_inside = ctx->editor.mouse_inside;
    bool pressed_inside = ctx->editor.mouse_inside && mouse_pressed;
    bool released_inside = ctx->editor.mouse_inside && mouse_released;
    Rect2i selection = ctx->editor.selection;

    switch(ctx->editor.cursor) {
    case SHEETEDITOR_CURSOR_TWEAK:
    {
        if (mouse_inside) {
            if (IsKeyPressed(KEY_G)) {
                spritesheet_try_set_cursor_mode(ctx, SHEETEDITOR_CURSOR_DRAG);
                return;
            }
            if (IsKeyPressed(KEY_S)) {
                spritesheet_try_set_cursor_mode(ctx, SHEETEDITOR_CURSOR_RESIZE);
                return;
            }
        }

        if (pressed_inside) {
            bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
            if (!shift) { spritesheet_clear_selection(ctx); }
        }
        if (ctx->editor.mouse_is_selecting) {
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
        if (mouse_inside) {
            // Delete last added sprite.
            if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyDown(KEY_Z) && ctx->editor.add_can_undo) {
                ctx->editor.add_can_undo = false;
                Spritesheet *sheet = get_current_spritesheet(ctx);
                if (sheet) {
                    remove_sprite(ctx, sheet->sprites.size -1);
                    spritesheet_clear_selection(ctx);
                }
            }
        }
        if (!ctx->editor.mouse_is_selecting) { return; }
        if (released_inside) {
            if (selection.width > 1 && selection.height > 1) {
                register_sprite(ctx, selection);
                ctx->editor.add_can_undo = true;
            }
        }
        break;
    }
    case SHEETEDITOR_CURSOR_DRAG:
    {

        // Cancel.
        if (mice_pressed(MouseRight) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) {
            editor_cancel_drag(ctx);
            spritesheet_reset_cursor_mode(ctx);
            return;
        }

        // Commit.
        if (pressed_inside) {
            spritesheet_reset_cursor_mode(ctx);
            return;
        }

        // Update transform on mouse move.
        if (v2i_eq(ctx->editor.drag_prev_mouse_pos, ctx->editor.mouse_pos)) { return; }

        V2i delta = v2i_sub(ctx->editor.mouse_pos, ctx->editor.drag_prev_mouse_pos);
        ctx->editor.drag_prev_mouse_pos = ctx->editor.mouse_pos;

        Spritesheet *sheet = get_current_spritesheet(ctx);
        if (!sheet) { break; }
        for (dyna_foreach(int, sprite_id, ctx->editor.selected_sprites)) {
            Sprite *sprite = Vec_Sprite_get_safe(&sheet->sprites, *sprite_id.ref);
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
        if (mice_pressed(MouseRight) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_Q)) {
            spritesheet_reset_cursor_mode(ctx);
            return;
        }
        // Commit.
        if (pressed_inside) {
            Sprite* sprite = try_get_first_selected_sprite(ctx);
            if (sprite == NULL) { return; }
            sprite->rect = selection;
            ctx->editor.mouse_is_selecting = false;
            spritesheet_reset_cursor_mode(ctx);
        }
        break;
    }
    case SHEETEDITOR_CURSOR__COUNT: { break; } }
}


void editor_process_cursor_logic(Ctx *ctx) {

    // Start selection.
    if (!ctx->editor.mouse_is_selecting && ctx->editor.mouse_inside
        && mice_pressed(MouseLeft)
        && (
            ctx->editor.cursor == SHEETEDITOR_CURSOR_TWEAK
            || ctx->editor.cursor == SHEETEDITOR_CURSOR_ADD
        )
    ) {
        ctx->editor.mouse_is_selecting = true;
        ctx->editor.selection_origin = ctx->editor.mouse_pos;
    }

    // Calculate selection.
    {
        ctx->editor.selection = Rect2i_from_two_positions(
                ctx->editor.selection_origin, ctx->editor.mouse_pos);

        // Shift to select a square.
        if ((IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
            && (
                ctx->editor.cursor == SHEETEDITOR_CURSOR_ADD
                || ctx->editor.cursor == SHEETEDITOR_CURSOR_RESIZE
            )
        ) {
            ctx->editor.selection = Rect2i_make_square_from_a_corner_and_a_point(
                    ctx->editor.selection_origin, ctx->editor.mouse_pos);
        }

        ctx->editor.selection.size = v2i_add(ctx->editor.selection.size, v2ii(1));
    }

    editor_process_cursor_mode_logic(ctx);

    // End selection.
    if (ctx->editor.mouse_is_selecting
        && mice_released(MouseLeft)
    ) {
        ctx->editor.mouse_is_selecting = false;
    }
}


void editor_cancel_drag(Ctx *ctx) {
    if (ctx->editor.cursor != SHEETEDITOR_CURSOR_DRAG) { return; }
    Spritesheet *sheet = get_current_spritesheet(ctx);
    if (!sheet) { return; }

    // Reset original positions.

    V2i delta = v2i_sub(ctx->editor.drag_origin, ctx->editor.drag_prev_mouse_pos);

    for (dyna_foreach(int, sprite_id, ctx->editor.selected_sprites)) {
        Sprite *sprite = Vec_Sprite_get_safe(&sheet->sprites, *sprite_id.ref);
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
    strbuf_printf(&delete_msg, "Delete %d items? No undo.", ctx->editor.selected_sprites.size);

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


int action_spritesheet_delete(Ctx *ctx) {
    Spritesheet *sheet = Vec_Spritesheet_get_safe(&ctx->spritesheet_list, ctx->mouse_selected_spritesheet_id);
    if (sheet == NULL) { return -1; }
    int confirm_delete = tinyfd_messageBox("DELETE?", "Delete spritesheet and it’s frames?", "yesno", "warning", 0);
    if (confirm_delete == 0) { return 0; }
    if (ctx->mouse_selected_spritesheet_id == ctx->curr_sheet_id) {
        ctx->curr_sheet_id = -1; ctx->curr_frame_id = 0;
        ctx->mouse_selected_spritesheet_id = -1;
    }
    editor_reset_selection(ctx);
    Spritesheet_free(sheet);
    return 0;
}


int action_spritesheet_reload(Ctx *ctx) {
    Spritesheet *sheet = Vec_Spritesheet_get_safe(&ctx->spritesheet_list, ctx->mouse_selected_spritesheet_id);
    if (sheet == NULL) { return -1; }
    SpritesheetFrame *frame = Vec_SpritesheetFrame_get_safe(&sheet->frames, 0);
    printfd("Why would it be not valid? "PRIstrw, PRIstrarg(strbuf_view2(frame->path)));
    if (frame == NULL && !strview_is_valid(strbuf_view2(frame->path))) { return -1; }
    strbuf_t *path = strbuf_create_with_arena(strbuf_view2(frame->path), &ctx->frame_arena.arena);
    ctx->mouse_selected_spritesheet_id = -1;
    return open_image_as_spritesheet(ctx, strbuf_view2(path));
}


int action_spritesheet_toggle_fold(Ctx *ctx) {
    Spritesheet *sheet = Vec_Spritesheet_get_safe(&ctx->spritesheet_list, ctx->mouse_selected_spritesheet_id);
    if (sheet == NULL) { return -1; }
    sheet->unfolded = !sheet->unfolded;
    ctx->mouse_selected_spritesheet_id = -1;
    return 0;
}


#endif // !OPERATIONS_H
