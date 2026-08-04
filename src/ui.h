#ifndef UI
#define UI

#include "operations.h"
#include "raylib_drawbuffer.h"
#include "raylib_extra.h"
#include "raylib_extra2.h"
#include "la_extra.h"
#include "raylib.h"
#include "la.h"
#include "rlgl.h"
#include "portable_utils.h"
#include "./standalone/uitree.h"
#include "ui_common.h"


#define DEFAULT_BG LIGHTGRAY
#define DEFAULT_FG BLACK
#define ANIMATION_TICKS_PER_FRAME 15
#define CHECKBOARD_BG (Color){ 200, 200, 200, 255 }
#define CHECKBOARD_FG (Color){ 0, 0, 0, 20 }


#define WIDGET__TABLE \
X( UI_WIDGET_OPTIONS              , ui_widget_options              ) \
X( UI_WIDGET_SPRITE_LIST          , ui_widget_sprite_list          ) \
X( UI_WIDGET_SPRITE_PREVIEW       , ui_widget_sprite_preview       ) \
X( UI_WIDGET_SPRITESHEET_LIST     , ui_widget_spritesheet_list     ) \
X( UI_WIDGET_SPRITESHEET_VIEWPORT , ui_widget_spritesheet_viewport ) \
X( UI_WIDGET_SPRITESHEET_CURSORS  , ui_widget_spritesheet_cursors  ) \
X( UI_WIDGET_SPRITESHEET_HINTS    , ui_widget_spritesheet_hints    ) \
X( UI_WIDGET_VSPLIT_DRAG          , ui_widget_vsplit_drag          ) \
X( UI_WIDGET_3HSPLIT_DRAG         , ui_widget_3hsplit_drag         )


enum UI_WIDGET {
    #define X(A, ...) A,
    WIDGET__TABLE
    #undef X
    UI_WIDGET_AMOUNT
};



void ui_draw_text(Ctx *ctx, strview_t str, V2i pos, Color tint) {
    DrawTextEx_strview_i(ctx->draw.font, str, pos, ctx->draw.font_size,
        ctx->draw.char_spacing, ctx->draw.line_spacing, tint);
}

void b_ui_draw_text(Ctx *ctx, strview_t str, V2i pos, Color tint) {
    b_DrawTextEx(ctx->draw.font, str, pos, ctx->draw.font_size,
        ctx->draw.char_spacing, ctx->draw.line_spacing, tint);
}

void b_ui_draw_text_pad(Ctx *ctx, strview_t str, V2i pos, Color tint) {
    const int PAD = 2;
    b_DrawTextEx(ctx->draw.font, str, (V2i){{pos.x + PAD, pos.y}},
        ctx->draw.font_size, ctx->draw.char_spacing, ctx->draw.line_spacing, tint);
}

void ui_draw_text_highlighted(Ctx *ctx, strview_t str, V2i pos, Color tint, Color highlight) {
    V2i measure = Vector2_to_v2i(MeasureTextEx_woy(ctx->draw.font, str, (float)ctx->draw.font_size, (float)ctx->draw.char_spacing, (float)ctx->draw.line_spacing));
    DrawRectangleReci((Rect2i){.pos = pos, .size = measure}, highlight);
    DrawTextEx_strview_i(ctx->draw.font, str, pos, ctx->draw.font_size,
        ctx->draw.char_spacing, ctx->draw.line_spacing, tint);
}

void b_ui_draw_text_highlighted(Ctx *ctx, strview_t str, V2i pos, Color tint, Color highlight) {
    V2i measure = Vector2_to_v2i(MeasureTextEx_woy(ctx->draw.font, str, (float)ctx->draw.font_size, (float)ctx->draw.char_spacing, (float)ctx->draw.line_spacing));
    b_DrawRectangle((Rect2i){.pos = pos, .size = measure}, highlight);
    b_DrawTextEx(ctx->draw.font, str, pos, ctx->draw.font_size,
        ctx->draw.char_spacing, ctx->draw.line_spacing, tint);
}

void ui_draw_text_outlined(Ctx *ctx, strview_t str, V2i pos, Color tint, int thick, Color outline) {
    /*
        Note: Fixes alpha blending.
        Additionally. This should make font rendering way better ?
        https://stackoverflow.com/a/77160530/18796401
        https://github.com/raysan5/raylib/issues/4181
    */
    rlSetBlendFactorsSeparate(0x0302, 0x0303, 1, 0x0303, 0x8006, 0x8006);
    BeginBlendMode(BLEND_CUSTOM_SEPARATE);

    for (int x = -thick; x <= thick ; ++x) {
    for (int y = -thick; y <= thick ; ++y) {
        DrawTextEx_strview_i(ctx->draw.font, str,(V2i){{.x=pos.x + x, .y=pos.y + y}}, ctx->draw.font_size, ctx->draw.char_spacing, ctx->draw.line_spacing, outline);
    }}
    DrawTextEx_strview_i(ctx->draw.font, str, (V2i){{.x=pos.x,.y=pos.y}}, ctx->draw.font_size, ctx->draw.char_spacing, ctx->draw.line_spacing, tint);

    EndBlendMode();
}

#define ui_simple_button(rect) ui__simple_button(rect,__COUNTER__)
bool ui__simple_button(Rect2i rect, const int id) {
    static int pressed_was_on_btn_with_id = -1;
    bool pressed = false;
    bool hover_highlight = false;
    bool held_highlight = false;
    if (mice_in_rect(rect)) {
        hover_highlight = true;
        if (mice_pressed_consume(MouseLeft)) {
            pressed_was_on_btn_with_id = id;
            held_highlight = true;
        }
        if (pressed_was_on_btn_with_id == id) {
            if (mice_held(MouseLeft)) {
                held_highlight = true;
            }
            if (mice_released(MouseLeft)) {
                pressed = true;
            }
        }
    }
    Color bg = held_highlight ? DARKBLUE : hover_highlight ? BLUE : LIGHTGRAY;
    b_DrawRectangle(rect, bg);
    b_DrawRectangleLines(rect, BLACK, 1);
    return pressed;
}


/*
   @Note: Draws sprite correctly scaled into container.
   @Returns final calculated transform.
*/
Rect2i b_ui_draw_sprite(Ctx *ctx, Spritesheet *sheet, Sprite *sprite, Rect2i area) {
    int frame_id = (ctx->ticks % (sprite->frames * ANIMATION_TICKS_PER_FRAME)) / ANIMATION_TICKS_PER_FRAME;
    SpritesheetFrame *frame = Vec_SpritesheetFrame_get_safe(&sheet->frames, frame_id);
    Rect2i final = {{ 0 }};

    int scale_x = find_multiple_max_fit(sprite->rect.size.x, area.size.x);
    int scale_y = find_multiple_max_fit(sprite->rect.size.y, area.size.y);
    if (scale_x <= 0 || scale_y <= 0) {
        // Fallback to fraction scaling.
        final.size = Rect_fit_in_Rect_and_preserve_aspect_ratio(area.size, sprite->rect.size);
    } else {
        // Pixel perfect scale.
        final.size = v2i_mul(sprite->rect.size, v2ii(int_min(scale_x, scale_y)));
    }
    // Center.
    final.pos.x = area.x + ((area.width - final.width) / 2);
    final.pos.y = area.y + ((area.height - final.height) / 2);

    b_DrawTextureScaled2(frame->texture, final, sprite->rect);
    return final;
}


int ui__calculate_scroll_px(int curr_scroll_px, int container_px, int child_px) {
    int max_scroll = int_max(container_px, child_px) - container_px;
    return int_clamp(-max_scroll, 0, curr_scroll_px);
}

void ui__calculate_fancy_scroll_px(int *scroll_px, float *vel_px, int container_px, int child_px, int force_up_or_down) {
    enum { IMPULSE=40 };
    if (force_up_or_down != 0) {
        *vel_px = (float)(force_up_or_down * IMPULSE);
    }
    if (*vel_px != 0) {
        *scroll_px += (int)*vel_px;
        *vel_px *= 0.5f;
        if (fabsf(*vel_px) < 0.1f) {
            *vel_px = 0;
        }
    }
    int max_scroll = int_max(container_px, child_px) - container_px;
    *scroll_px = int_clamp(-max_scroll, 0, *scroll_px);
}



void ui_widget_scroll(Widget *widget) {
    if (!widget->scroll_enabled) { return; }

    if (widget->focused) {
        enum { SCROLL_PX = 60 };
        int scroll = mice_wheel();
        printfd("Got scroll %d", scroll);
        if (scroll != 0) {
            widget->scroll_px -= scroll * SCROLL_PX;
        }
        int max_scroll =
            int_max(widget->scroll_max_px, widget->screen_area.height)
            - widget->screen_area.height;

        printvalnum(widget->scroll_max_px);
        printvalnum(widget->screen_area.height);
        printvalnum(max_scroll);

        widget->scroll_px = int_clamp(0, max_scroll, widget->scroll_px);
    }

    /* Apply scroll. */
    widget->draw_info.area.y -= widget->scroll_px;

    /*
       TODO: Show scrollbar.
        widget->show_scrollbar = max_scroll > 0;

        if (widget->show_scrollbar) {
            widget_draw.area.width -= SCROLLBAR_WIDTH_PX;
        }
    */
}


void ui__spritesheet_draw_scaled_rect(Rect2i r, V2i translate, int scale, Color tint) {
    b_DrawRectangle((Rect2i) {
        .pos = v2i_translate_scale(r.pos, translate, (float)scale),
        .size = v2i_mul(r.size, v2ii(scale))
    }, tint);
}

void ui__spritesheet_draw_scaled_rect_lines2(Rect2i r, V2i translate, int scale, Color tint, int thick) {
    b_DrawRectangleLinesEx((Rect2i) {
        .pos = v2i_translate_scale(r.pos, translate, (float)scale),
        .size = v2i_mul(r.size, v2ii(scale))
    }, thick, tint);
}

void ui__spritesheet_draw_scaled_rect_lines(Rect2i r, V2i translate, int scale, Color tint, int thick) {
    Rect2i line;
    line = (Rect2i) {{ .x = r.pos.x, .y = r.pos.y, .width = r.size.x, .height = thick }}; // Top
    ui__spritesheet_draw_scaled_rect(line, translate, scale, tint);
    line = (Rect2i) {{ .x = r.pos.x, .y = r.pos.y, .width = thick, .height = r.size.y }}; // Left
    ui__spritesheet_draw_scaled_rect(line, translate, scale, tint);
    line = (Rect2i) {{ .x = r.pos.x + r.size.x - thick, .y = r.pos.y, .width = thick, .height = r.size.y }}; // Right
    ui__spritesheet_draw_scaled_rect(line, translate, scale, tint);
    line = (Rect2i) {{ .x = r.pos.x, .y = r.pos.y + r.size.y - thick, .width = r.size.x, .height = thick }}; // Bottom
    ui__spritesheet_draw_scaled_rect(line, translate, scale, tint);
}

void ui_widget_options(Ctx *ctx, uitree_DrawInfo info) {
    const int PAD = 2;
    const int line_height = ctx->draw.line_height;
    const Rect2i area = info.area;
    int *is_menu_open = &info.state->int_a;

    {
        Rect2i btn_area = { .x = area.x, .y = area.y, .width = area.width, .height = line_height };
        bool mouse_in_options_btn = mice_in_rect(btn_area);
        b_DrawRectangle(btn_area, DEFAULT_BG);
        if (mouse_in_options_btn) {
            b_DrawRectangle(btn_area, BLUE);
            if (mice_pressed(MouseLeft)) {
                mice_consume(MouseLeft);
                *is_menu_open = !*is_menu_open;
            }
        }
        b_DrawRectangleLines(btn_area, MAGENTA, 1);
        b_ui_draw_text(ctx, cstr_SL("Options"), (V2i){{ area.pos.x + PAD, area.pos.y }}, DEFAULT_FG);
    }

    if (!*is_menu_open) { return; }

    // Drawing menu.

    Rect2i menu_area = { .x = area.x, .y = area.y + line_height, .height = line_height * ctx->actions.size, .width = 400, };
    bool mouse_focus = mice_in_rect(menu_area);
    Rect2i line_area;
    b_BeginScissorMode(menu_area);
    b_DrawRectangle(menu_area, DEFAULT_BG);

    for (int i = 0; i < ctx->actions.size; ++i)
    {
        Action *action = &ctx->actions.items[i];
        line_area = (Rect2i) {{
            .x = menu_area.x, .y = menu_area.y + i * ctx->draw.line_height,
            .width = menu_area.width, .height = line_height
        }};

        if (mouse_focus && mice_in_rect(line_area)) {
            b_DrawRectangle(line_area, BLUE);

            if (mice_pressed(MouseLeft)) {
                mice_consume(MouseLeft);
                
                call_action(ctx, action);
                *is_menu_open = false;
            }
        }

        b_ui_draw_text(ctx, strbuf_view2(action->name), (V2i){{ line_area.pos.x+PAD, line_area.pos.y }}, DEFAULT_FG);
    }
    if (mice_pressed(MouseLeft)) { *is_menu_open = false; }

    b_DrawRectangleLines(menu_area, BLACK, 1);
    b_EndScissorMode();
}



void ui_widget_spritesheet_list(Ctx *ctx, uitree_DrawInfo info) {

    const Rect2i area = info.area;
    int *scroll_px = &info.state->int_a;
    float *scroll_vel = &info.state->float_a;
    Rect2i area_scroll = area;
    const int item_height = ctx->draw.line_height * 2;
    const int text_pad = 3;
    const int thumbnail_pad = 3;

    Rect2i item_area;
    Rect2i thumbnail_area;
    V2i text_offset;

    b_DrawRectangle(area, DEFAULT_BG);
    b_ui_draw_text(ctx, cstr_SL("Spritesheets:"), area.pos, DEFAULT_FG);
    area_scroll.y += ctx->draw.line_height;
    Rect2i area_scroll_viewport = area_scroll;
    area_scroll_viewport.height -= ctx->draw.line_height;
    b_BeginScissorMode(area_scroll_viewport);

    bool mouse_focus = mice_in_rect(area);
    {
        // Scrolling.
        int scroll_wheel = mouse_focus ? int_sign(mice_wheel()) : 0;
        ui__calculate_fancy_scroll_px(scroll_px, scroll_vel, area_scroll_viewport.height,
                ctx->spritesheet_list.size * item_height, scroll_wheel);
        area_scroll.y += *scroll_px;
    }

    int i = -1;
    for (dyna_foreach(Spritesheet, kter, ctx->spritesheet_list)) {
        Spritesheet *sheet = kter.ref;
    for (dyna_foreach(SpritesheetFrame, iter, sheet->frames)) {
        SpritesheetFrame *frame = iter.ref;
        ++i;

        item_area = (Rect2i) {{
            area_scroll.x + (iter.index > 0 ? ctx->draw.font_size : 0),
            area_scroll.y + i * item_height, area_scroll.width, item_height }};
        thumbnail_area = item_area;
        thumbnail_area.width = thumbnail_area.height;
        thumbnail_area = Rect2i_add_padding_all(thumbnail_area, thumbnail_pad);
        text_offset = (V2i) {{ thumbnail_area.x + thumbnail_area.width + text_pad, item_area.y + text_pad }};

        if (mouse_focus && mice_in_rect(item_area)) {
            b_DrawRectangle(item_area, BLUE);

            if (mice_held(MouseLeft)) {
                uint8_t layer_bk = drawbuf_get_layer();
                drawbuf_set_layer(200);
                Rect2i preview_area = { .pos = GetMousePositioni(), .size = {{ frame->texture.width, frame->texture.height }} };
                preview_area = Rect2i_stay_within_Rect2i(preview_area, (Rect2i){{.width = GetScreenWidth(), .height = GetScreenHeight()}});
                b_DrawRectangle(Rect2i_add_padding_all(preview_area, -1), BLACK);
                b_DrawRectangle(preview_area, DEFAULT_BG);
                b_DrawTextureScaled(frame->texture, preview_area);
                drawbuf_set_layer(layer_bk);
            }
        }

        b_ui_draw_text(ctx, strbuf_view2(frame->path), text_offset, DEFAULT_FG);

        b_DrawRectangle(Rect2i_add_padding_all(thumbnail_area, -1), BLACK);
        b_DrawRectangle(thumbnail_area, DEFAULT_BG);
        b_DrawTextureScaled(frame->texture, thumbnail_area);
    }
    }

    b_EndScissorMode();
    b_DrawRectangleLines(area_scroll_viewport, MAGENTA, 1);
}


void ui_widget_sprite_list(Ctx *ctx, uitree_DrawInfo info) {

    const Rect2i area = info.area;
    Rect2i area_s = info.area; // Area scroll.
    Rect2i area_scroll_viewport = info.area;
    int *scroll_px = &info.state->int_a;
    float *scroll_vel_px = &info.state->float_a;
    strbuf_t *aux_str = strbuf_create(0, &ctx->frame_arena.strbuf_alloc);

    const int item_height = ctx->draw.line_height * 2;
    const int text_pad = 3;
    const int thumbnail_pad = 3;

    Rect2i item_area;
    Rect2i thumbnail_area;
    V2i text_offset;

    int selected_sprite = -1;
    {
        int *selected_sprite_ref = get_selected_sprite(ctx);
        if (selected_sprite_ref != NULL) {
            selected_sprite = *selected_sprite_ref;
        }
    }

    b_DrawRectangle(area, DEFAULT_BG);
    b_ui_draw_text(ctx, cstr_SL("Sprites:"), area.pos, DEFAULT_FG);

    Spritesheet *sheet = get_current_spritesheet(ctx);
    if (!sheet) { return; }
    Vec_Sprite *sprites = &sheet->sprites;

    area_s.y += ctx->draw.line_height;
    area_scroll_viewport = area_s;
    area_scroll_viewport.height -= ctx->draw.line_height;
    b_BeginScissorMode(area_scroll_viewport);

    bool mouse_focus = mice_in_rect(area_s);
    if (mouse_focus) {
        ui__calculate_fancy_scroll_px(
                scroll_px, scroll_vel_px,
                area_scroll_viewport.height,
                item_height * (sprites->size + 1), int_sign(mice_wheel()));
    };
    area_s.y += *scroll_px;

    for (int i = 0; i < sprites->size; ++i)
    {
        Sprite *sprite = &sprites->items[i];

        item_area = (Rect2i) {{ area_s.x, area_s.y + i * item_height, area_s.width, item_height }};
        thumbnail_area = item_area;
        thumbnail_area.width = thumbnail_area.height;
        thumbnail_area = Rect2i_add_padding_all(thumbnail_area, thumbnail_pad);
        text_offset = (V2i) {{ thumbnail_area.x + thumbnail_area.width + text_pad, item_area.y + text_pad }};

        bool highlight = false;

        if (selected_sprite == i) { highlight = true; }

        if (mouse_focus && mice_in_rect(item_area)) {
            highlight = true;
            if (mice_pressed(MouseLeft)) {
                mice_consume(MouseLeft);
                // Select
                spritesheet_clear_selection(ctx);
                int_Dyna_append(&ctx->editor.selected_sprites, i);
            }
        }

        if (highlight) { b_DrawRectangle(item_area, BLUE); }

        strbuf_assign(&aux_str, cstr_SL(""));
        if (sprite->frames > 1) {
            strbuf_append_printf(&aux_str, "(%d frames) ", sprite->frames);
        }
        strbuf_append_printf(&aux_str,"%dx%d offset %d,%d\n%"PRIstr,
            sprite->size.x, sprite->size.y, sprite->offset.x, sprite->offset.y,
            PRIstrargbuf(sprite->name)
        );

        b_ui_draw_text(ctx, strbuf_view2(aux_str), text_offset, DEFAULT_FG);

        // Draw texture.

        b_DrawRectangleLines(Rect2i_add_padding_all(thumbnail_area, -1), BLACK, 1);
        b_ui_draw_sprite(ctx, sheet, sprite, thumbnail_area);
    }
    b_EndScissorMode();

    b_DrawRectangleLines(area, MAGENTA, 1);
    b_DrawRectangleLines(area_scroll_viewport, MAGENTA, 1);
}


void ui_widget_sprite_preview(Ctx *ctx, uitree_DrawInfo info) {

    int *selected_sprite_ref = get_selected_sprite(ctx);
    if (selected_sprite_ref == NULL) { return; }
    int selected_sprite = *selected_sprite_ref;

    Spritesheet *sheet = get_current_spritesheet(ctx);
    if (!sheet) { return; }
    Sprite *sprite = Vec_Sprite_get_safe(&sheet->sprites, selected_sprite);
    if (sprite == NULL) { return; }

    // Calculate current frame.

    Rect2i area = info.area;
    b_DrawRectangle(area, DARKGRAY);

    // Draw name controls.
    {
        Rect2i line_box = area;
        line_box.height = ctx->draw.line_height;
        line_box.y      = area.y + area.height - line_box.height *2;

        b_DrawRectangle(line_box, DEFAULT_BG);

        Rect2i label_box = line_box;
        label_box.width -= label_box.height;

        Rect2i btn_edit_box = line_box;
        btn_edit_box.width = label_box.height;
        btn_edit_box.x += label_box.width;

        // Name.
        {
            b_DrawRectangleLines(label_box, BLACK, 1);
            strview_t view;
            if (sprite->name->size == 0) {
                view = cstr_SL("<unnamed>");
            } else {
                view = cstr(TextFormat("\"%"PRIstr"\"", PRIstrargbuf(sprite->name)));
            }
            b_ui_draw_text_pad(ctx, view, label_box.pos, DEFAULT_FG);
        }

        // Edit name button.
        if (ui_simple_button(btn_edit_box)) {
            const char *default_input = sprite->name->size > 0 ? sprite->name->cstr : "name";
            const char *new_name_cstr = tinyfd_inputBox("Rename", "Insert new name for Sprite", default_input);
            if (new_name_cstr != NULL) {
                strview_t new_name = cstr(new_name_cstr);
                if (sprite_name_already_exists(ctx, new_name)) {
                    tinyfd_messageBox("Error", "Name already exists", "ok", "error", 1);
                } else {
                    strbuf_assign(&sprite->name, new_name);
                }
            }
        }

        b_ui_draw_text_pad(ctx, cstr_SL("Edit"), btn_edit_box.pos, DEFAULT_FG);
    }

    // Draw frame controls.
    {
        Rect2i line_box = area;
        line_box.height = ctx->draw.line_height;
        line_box.y      = area.y + area.height - line_box.height;

        Rect2i chunks[3];
        Rect2i_split_horizontally(line_box, 3, chunks, { 1/2.f, 1/4.f });
        Rect2i label_box       = chunks[0];
        Rect2i frame_minus_box = chunks[1];
        Rect2i frame_plus_box  = chunks[2];

        b_DrawRectangle(line_box, DEFAULT_BG);
        b_ui_draw_text_pad(ctx, cstr(TextFormat("Frames %d", sprite->frames)), label_box.pos, DEFAULT_FG);
        b_DrawRectangleLines(label_box, BLACK, 1);

        if (ui_simple_button(frame_minus_box)) {
            if (sprite->frames > 1) {
                --sprite->frames;
            }
        }
        b_ui_draw_text_pad(ctx, cstr_SL("-"), frame_minus_box.pos, DEFAULT_FG);

        if (ui_simple_button(frame_plus_box)) {
            if (sprite->frames < sheet->frames.size) {
                ++sprite->frames;
            }
        }
        b_ui_draw_text_pad(ctx, cstr_SL("+"), frame_plus_box.pos, DEFAULT_FG);
    }

    area.height -= ctx->draw.line_height * 2;
    area = Rect2i_add_padding_all(area, 2);

    // Draw sprite.
    {
        b_DrawRectangle(area, DARKGRAY);
        Rect2i sprite_area = b_ui_draw_sprite(ctx, sheet, sprite, area);
        b_DrawRectangle(sprite_area, CHECKBOARD_BG);
        b_DrawCheckerboard(sprite_area, CHECKBOARD_FG, (sprite_area.width / sprite->size.x) * 8);
        b_ui_draw_sprite(ctx, sheet, sprite, area);
    }
}


void ui_widget_spritesheet_cursors(Ctx *ctx, uitree_DrawInfo info) {

    Rect2i area = info.area;

    const int btn_width = 30;
    Rect2i btn_area = {{
        .x = area.x + area.width -btn_width, .y = area.y,
        .width = btn_width, .height = btn_width
    }};

    SHEETEDITOR_CURSOR mode;
    bool pressed;
    Color bg_color;

    // Tweak cursor. (DEFAULT)
    mode = SHEETEDITOR_CURSOR_TWEAK;
    pressed = mice_pressed_inside_and_consume(MouseLeft, btn_area);
    bg_color = pressed ? DARKBLUE : ctx->editor.cursor == mode ? BLUE : DEFAULT_BG;
    b_DrawRectangle(btn_area, BLACK);
    b_DrawRectangle(Rect2i_add_padding_all(btn_area, 1), bg_color);
    b_ui_draw_text(ctx, cstr_SL("Tweak"), btn_area.pos, DEFAULT_FG);
    if (pressed) {
        spritesheet_try_set_cursor_mode(ctx, mode);
    }
    btn_area.y += btn_area.height;

    // Add cursor.
    mode = SHEETEDITOR_CURSOR_ADD;
    pressed = mice_pressed_inside_and_consume(MouseLeft, btn_area);
    bg_color = pressed ? DARKBLUE : ctx->editor.cursor == mode ? BLUE : DEFAULT_BG;
    b_DrawRectangle(btn_area, BLACK);
    b_DrawRectangle(Rect2i_add_padding_all(btn_area, 1), bg_color);
    b_ui_draw_text(ctx, cstr_SL("Add"), btn_area.pos, DEFAULT_FG);
    if (pressed) {
        spritesheet_try_set_cursor_mode(ctx, mode);
    }
    btn_area.y += btn_area.height;

    // Reset zoom and pan button.
    bg_color = DEFAULT_BG;
    pressed = mice_pressed_inside_and_consume(MouseLeft, btn_area);
    bg_color = pressed ? DARKBLUE : DEFAULT_BG;
    b_DrawRectangle(btn_area, BLACK);
    b_DrawRectangle(Rect2i_add_padding_all(btn_area, 1), bg_color);
    b_ui_draw_text(ctx, cstr_SL("Reset\nZoom"), btn_area.pos, DEFAULT_FG);
    if (pressed) {
        zoompanel_reset_zoom_and_pan(&ctx->zoompanel);
    }
    btn_area.y += btn_area.height;
}


void ui_widget_spritesheet_viewport(Ctx *ctx, uitree_DrawInfo info) {

    Rect2i area = info.area;
    bool mouse_focus = mice_in_rect(area);
    strbuf_t *aux_str = strbuf_create(0, &ctx->frame_arena.strbuf_alloc);
    V2i mouse = GetMousePositioni();

    Spritesheet *sheet = get_current_spritesheet(ctx);
    SpritesheetFrame *frame = get_selected_spritesheet_frame(ctx);
    if (!sheet || !frame) {
        b_DrawRectangle(area, DEFAULT_BG);
        b_ui_draw_text(ctx, cstr_SL("No spritesheet."), v2i_add(area.pos, v2ii(10)), DEFAULT_FG);
        return;
    }

    // Spritesheet viewport ↓↓↓

    const Texture texture = frame->texture;

    V2i texture_size = v2i(texture.width, texture.height);

    zoompanel_process(&ctx->zoompanel, texture_size, area);

    V2i panned_origin = v2i_add(area.pos, ctx->zoompanel.offset_from_origin);
    V2i scaled_size = v2f_2i(v2f_mul(v2i_2f(texture_size), v2ff(ctx->zoompanel.zoom)));
    Rect2i final = (Rect2i){
        .pos = panned_origin,
        .size = scaled_size,
    };

    // Get cursor position

    float scale = ctx->zoompanel.zoom;
    V2i mouse_from_image_origin = v2i_sub(mouse, panned_origin);
    ctx->editor.mouse_pos = v2f_2i(v2f_mul(
        v2f_div(v2i_2f(mouse_from_image_origin), v2i_2f(scaled_size)),
        v2i_2f(texture_size)));

    // Draw on screen.

    b_BeginScissorMode(area);
    b_DrawRectangle(area, DARKGRAY);
    b_DrawRectangle(final, CHECKBOARD_BG);
    b_DrawCheckerboard(final, CHECKBOARD_FG, (int)(((float)16 * ctx->zoompanel.zoom)));
    b_DrawTextureScaled(texture, final);

    {
        // Draw current sprites.
        for (dyna_foreach(Sprite, i, sheet->sprites)) {
            Sprite *sprite = i.ref;
            Color color = Rect2i_is_out_of_bounds(sprite->rect, ctx->curr_spritesheet_rect) ? RED : YELLOW;
            ui__spritesheet_draw_scaled_rect_lines2(sprite->rect, panned_origin, (int)scale, color, 2);
        }

        // Draw selected sprites.
        for (dyna_foreach(int, i, ctx->editor.selected_sprites)) {
            int sprite_id = *i.ref;
            Sprite *sprite = Vec_Sprite_get_safe(&sheet->sprites, sprite_id);
            if (sprite == NULL) { continue; }
            ui__spritesheet_draw_scaled_rect_lines(sprite->rect, panned_origin, (int)scale, BLUE, 1);
        }
        for (dyna_foreach(int, i, ctx->editor.selected_sprites_cursor)) {
            int sprite_id = *i.ref;
            Sprite *sprite = Vec_Sprite_get_safe(&sheet->sprites, sprite_id);
            if (sprite == NULL) { continue; }
            ui__spritesheet_draw_scaled_rect_lines(sprite->rect, panned_origin, (int)scale, BLUE, 1);
        }
    }

    // Draw selection.

    if (ctx->editor.cursor == SHEETEDITOR_CURSOR_TWEAK
        || ctx->editor.cursor == SHEETEDITOR_CURSOR_ADD
        || ctx->editor.cursor == SHEETEDITOR_CURSOR_RESIZE
    ) {
        ctx->editor.mouse_inside = mouse_focus && mice_in_rect(area);

        // Holding.
        if (ctx->editor.mouse_is_selecting) {
            Rect2i selection = ctx->editor.selection;
            ui__spritesheet_draw_scaled_rect_lines(
                selection,
                panned_origin, (int)scale, GREEN, 1);
            
            strview_t text = strbuf_printf(&aux_str, "%d x %d", selection.size.x, selection.size.y);
            b_ui_draw_text_highlighted(
                ctx, text,
                v2f_2i(v2f_translate_scale(v2i_add(selection.pos, v2ii(1)), panned_origin, scale)),
                WHITE, BLACK);
        }
    }

    // Draw cursor position scaled.

    ui__spritesheet_draw_scaled_rect((Rect2i){.pos = ctx->editor.mouse_pos, .size = v2ii(1)}, panned_origin, (int)scale, GREEN);

    b_EndScissorMode();
}


void ui_widget_spritesheet_hints(Ctx *ctx, uitree_DrawInfo info) {
    const Rect2i area = info.area;

    if (ctx->spritesheet_list.size <= 0) { return; }

    strview_t text = STRVIEW_INVALID;

    switch(ctx->editor.cursor) {
    case SHEETEDITOR_CURSOR_TWEAK:
    {
        if (ctx->editor.selected_sprites.size > 0) {
            text = cstr_SL(
                "G to move.\n"
                "S to resize.\n"
                "X to delete."
            );
        }
        break;
    }
    case SHEETEDITOR_CURSOR_DRAG:
    {
        text = cstr_SL(
            "Left click to confirm.\n"
            "Right click, Escape, Q to cancel."
        );
        break;
    }
    case SHEETEDITOR_CURSOR_RESIZE:
    {
        text = cstr_SL(
            "Left click to confirm.\n"
            "Shift for square selection.\n"
            "Right click, Escape, Q to cancel."
        );
        break;
    }
    case SHEETEDITOR_CURSOR_ADD:
    {
        if (ctx->editor.add_can_undo) {
            text = cstr_SL(
                "Ctrl + Z to undo."
            );
        }
        break;
    }
    default: { return; }
    }

    V2i measure = MeasureTextEx_woyi(ctx->draw.font, text, ctx->draw.font_size, ctx->draw.char_spacing, ctx->draw.line_spacing);
    b_ui_draw_text(ctx, text, v2i_sub(v2i_add(area.pos, area.size), measure), DEFAULT_FG);
}

uitree_Node ui_widget_spritesheet(Uitree *tree) {
    uitree_Node con_stack = uitree_container_dumb(widget_stack);
    uitree_Node widget = uitree_widget(UI_WIDGET_SPRITESHEET_VIEWPORT);
    uitree_container_add_child(tree, &con_stack, widget);
    widget = uitree_widget(UI_WIDGET_SPRITESHEET_CURSORS);
    uitree_container_add_child(tree, &con_stack, widget);
    widget = uitree_widget(UI_WIDGET_SPRITESHEET_HINTS);
    uitree_container_add_child(tree, &con_stack, widget);
    return con_stack;
}


// ↓↓↓ Maps UI_WIDGET to function pointer.

void (*widget_func[]) (Ctx *ctx, uitree_DrawInfo info) = {
    #define X(A, B) B,
    WIDGET__TABLE
    #undef X
};

void ui_draw_all(Ctx *ctx) {

    static Uitree tree = { 0 };
    static Uitree *t = &tree;
    static bool setup = false;
    if (!setup) {
        setup = true;
        uitree_create(t);
    }

    uitree_build_start(t, (Rect2i){{.width = GetScreenWidth(), .height = GetScreenHeight()}});

    uitree_Node widget;
    uitree_Node con_tree = uitree_container_dumb(widget_vlist);

    {
        uitree_Node con_3split = uitree_container(t, cstr_SL("MainHSplit"),
                widget_3hsplit, UI_WIDGET_3HSPLIT_DRAG);
        widget_3hsplit_set_user_default_state(t, &con_3split, 20, 80);

        {
            {
                uitree_Node con_vsplit = uitree_container_dumb(widget_vsplit);
                widget_vsplit_set_user_default_state(t, &con_vsplit, ctx->draw.line_height, true);

                {
                    {
                        uitree_Node con_stack = uitree_container_dumb(widget_stack);
                        widget = uitree_widget_id(t, UI_WIDGET_OPTIONS, cstr_SL("options"));
                        uitree_container_add_child(t, &con_stack, widget);
                        uitree_container_add_child(t, &con_vsplit, con_stack);
                    }

                    widget = uitree_widget_id(t, UI_WIDGET_SPRITE_LIST, cstr_SL("WidgetSpriteList"));
                    uitree_container_add_child(t, &con_vsplit, widget);
                }

                uitree_container_add_child(t, &con_3split, con_vsplit);
            }

            widget = ui_widget_spritesheet(t);
            uitree_container_add_child(t, &con_3split, widget);

            {
                uitree_Node con_vsplit = uitree_container(t, cstr_SL("SecondVsplit"),
                    widget_vsplit, UI_WIDGET_VSPLIT_DRAG);
                widget_vsplit_set_user_default_state(t, &con_vsplit, 80, false);

                {
                    widget = uitree_widget_id(t, UI_WIDGET_SPRITESHEET_LIST, cstr_SL("SpriteSheetList"));
                    uitree_container_add_child(t, &con_vsplit, widget);

                    widget = uitree_widget(UI_WIDGET_SPRITE_PREVIEW);
                    uitree_container_add_child(t, &con_vsplit, widget);
                }

                uitree_container_add_child(t, &con_3split, con_vsplit);
            }
        }
        uitree_container_add_child(t, &con_tree, con_3split);
    }

    t->root_node = con_tree;
    uitree_build_end(t);

    uitree_List_DrawInfo_It it = { 0 };
    while(uitree_List_DrawInfo_it_next(&t->out_draw_list, &it)) {
        uitree_DrawInfo draw = *it.item;
        drawbuf_set_layer((uint8_t)draw.layer);
        widget_func[it.item->user_draw_func_id](ctx, draw);
    }
    drawbuf_draw_all();

    //printfd("Arena consumption is "PRIbyte" out of "PRIbyte, PRIbytearg((1 << 20) - (t->arena.end - t->arena.beg)), PRIbytearg(1 << 20));
}

#endif // !UI
