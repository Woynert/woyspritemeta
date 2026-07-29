#ifndef UI
#define UI

#include "operations.h"
#include "raylib_drawbuffer.h"
#include "raylib_extra.h"
#include "la_extra.h"
#include "raylib.h"
#include "la.h"
#include "winput.h"
#include "rlgl.h"
#include "portable_utils.h"
#include "./standalone/uitree.h"

#define UI_WIDGET_DEFAULT_REQUEST_HANDLER(req) do {            \
    if (req != NULL) {                                             \
        if (req->focus_area_request && !req->focus_area_success) { \
            req->focus_area_success = true;                        \
            req->focus_area = widget.area;                         \
        }                                                          \
        /* return; */                                              \
    }} while (0)

#define DEFAULT_BG LIGHTGRAY
#define DEFAULT_FG BLACK
#define ANIMATION_TICKS_PER_FRAME 15



int ui__calculate_focus(const Widget_view widgets);
void ui__calculate_focus_and_draw_widgets(Ctx *ctx, Widget *widgets, int count);



void ui_draw_text(Ctx *ctx, strview_t str, V2i pos, Color tint) {
    DrawTextEx_strview_i(ctx->draw.font, str, pos, ctx->draw.font_size,
        ctx->draw.char_spacing, ctx->draw.line_spacing, tint);
}

void drawbuf_ui_draw_text(Ctx *ctx, strview_t str, V2i pos, Color tint) {
    drawbuf_DrawTextEx(ctx->draw.font, str, pos, ctx->draw.font_size,
        ctx->draw.char_spacing, ctx->draw.line_spacing, tint);
}

void ui_draw_text_highlighted(Ctx *ctx, strview_t str, V2i pos, Color tint, Color highlight) {
    V2i measure = Vector2_to_v2i(MeasureTextEx_woy(ctx->draw.font, str, (float)ctx->draw.font_size, (float)ctx->draw.char_spacing, (float)ctx->draw.line_spacing));
    DrawRectangleReci((Rect2i){.pos = pos, .size = measure}, highlight);
    DrawTextEx_strview_i(ctx->draw.font, str, pos, ctx->draw.font_size,
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

bool ui__simple_button(const int id, Rect2i rect) {
    static int pressed_was_on_btn_with_id = -1;
    bool pressed = false;
    bool hover_highlight = false;
    bool held_highlight = false;
    if (CheckCollisionPointReci(GetMousePositioni(), rect)) {
        hover_highlight = true;
        if (winput_mice_pressed(MouseLeft)) {
            pressed_was_on_btn_with_id = id;
            held_highlight = true;
        }
        if (pressed_was_on_btn_with_id == id) {
            if (winput_mice_held(MouseLeft)) {
                held_highlight = true;
            }
            if (winput_mice_released(MouseLeft)) {
                pressed = true;
            }
        }
    }
    Color bg = held_highlight ? DARKBLUE : hover_highlight ? BLUE : LIGHTGRAY;
    DrawRectangleReci(rect, bg);
    DrawRectangleLinesi(rect, BLACK, 1);
    return pressed;
}

#define ui_simple_button(rect) ui__simple_button(__COUNTER__,rect)


/*
   @Note: Draws sprite correctly scaled into container.
   @Returns final calculated transform.
*/
Rect2i ui_draw_sprite(Ctx *ctx, Sprite *sprite, Rect2i area) {
    int frame = (ctx->ticks % (sprite->frames * ANIMATION_TICKS_PER_FRAME)) / ANIMATION_TICKS_PER_FRAME;
    Spritesheet *sheet = Spritesheet_Dyna_get_safe(&ctx->spritesheet_list, frame);
    if (sheet == NULL) {
        return (Rect2i) { 0 };
    }

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

    DrawTextureScaled2(sheet->texture, final, sprite->rect);
    return final;
}


void ui_widget_scroll(Widget *widget) {
    if (!widget->scroll_enabled) { return; }

    if (widget->focused) {
        enum { SCROLL_PX = 60 };
        int scroll = (int)winput_wheel();
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


void ui__spritesheet_draw_scaled_rect(Rect2i r, V2i translate, float scale, Color tint) {
    DrawRectangleRecf((Rect2) {
        .pos = v2f_translate_scale(r.pos, translate, scale),
        .size = v2f_mul(v2i_2f(r.size), v2ff(scale))
    }, tint);
}

void ui__spritesheet_draw_scaled_rect_lines2(Rect2i r, V2i translate, float scale, Color tint, float thick) {
    DrawRectangleLinesEx((Rect2) {
        .pos = v2f_translate_scale(r.pos, translate, scale),
        .size = v2f_mul(v2i_2f(r.size), v2ff(scale))
    }.rect, thick, tint);
}

void ui__spritesheet_draw_scaled_rect_lines(Rect2i r, V2i translate, float scale, Color tint, int thick) {
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

    //UI_WIDGET_DEFAULT_REQUEST_HANDLER(req);

    Rect2i area = info.area;
    bool mouse_focus = CheckCollisionPointReci(GetMousePositioni(), area);
    const int line_height = ctx->draw.line_height;
    int line = 0;
    V2i pos = { 0 };
    const int pad = 10;

    pos = (V2i) {{ area.x + pad, area.y + line * line_height }};
    ++line;

    Rect2i line_area = {{ pos.x, pos.y, area.width -pad, line_height }};
    line_area.y += line_height;

    drawbuf_DrawRectangle(area, DEFAULT_BG);
    drawbuf_ui_draw_text(ctx, cstr_SL("Options:"), pos, DEFAULT_FG);

    for (int i = 0; i < ctx->actions.size; ++i)
    {
        Action *action = &ctx->actions.items[i];
        pos = (V2i) {{ (int)area.x + pad, (int)area.y + line * ctx->draw.line_height }};

        if (mouse_focus && CheckCollisionPointReci(GetMousePositioni(), line_area)) {
            drawbuf_DrawRectangle(line_area, BLUE);

            if (winput_mice_pressed(MouseLeft)) {
                call_action(ctx, action);
            }
        }

        drawbuf_ui_draw_text(ctx, strbuf_view2(action->name), pos, DEFAULT_FG);

        ++line;
        line_area.y += line_height;
    }

    // DELME: Simple example of state mutation.

    if (winput_wheel() > 0) {
        ++info.state->scroll;
    }

    ui_draw_text(ctx, cstr(TextFormat("%d", info.state->scroll)), area.pos, BLUE);
    drawbuf_DrawRectangleLines(area, MAGENTA, 1);

    // DELME
}


void ui_widget_vsplit(Ctx *ctx, uitree_DrawInfo info) {

    Rect2i area = info.area;

    drawbuf_DrawRectangle(area, DEFAULT_BG);

    if (winput_wheel() != 0) {
        info.state->scroll -= int_sign((int)winput_wheel()) * 3;
        info.state->scroll = int_clamp(-40, 40, info.state->scroll);
        // ↑↑↑ This ensures at least 10% is visible at minimum.
    }

    drawbuf_ui_draw_text(ctx, cstr(TextFormat("%d", info.state->scroll)), v2i_add(area.pos, v2ii(3)), GREEN);

    drawbuf_DrawRectangleLines(area, BLUE, 8);
}

void ui_widget_vsplit_drag(Ctx *ctx, uitree_DrawInfo info) {

    Rect2i area = info.area;
    Rect2i drag_area = info.state->rect_a;
    int *is_dragging = &info.state->int_b;

    drawbuf_DrawRectangle(area, DEFAULT_BG);

    if (CheckCollisionPointReci(GetMousePositioni(), drag_area) || *is_dragging) {
        drawbuf_DrawRectangle(drag_area, ORANGE);
        if (winput_mice_pressed(MouseLeft)) {
            *is_dragging = true;
        }
    }

    if (*is_dragging) {

        int mouse_y = GetMousePositioni().y;
        float factor = ((float)mouse_y - ((float)area.y + (float)area.height / 2.0f)) / (float)area.height;
        printfd("FACTOR %f", factor);
        info.state->scroll = (int)(factor * 100.0f);
        info.state->scroll = int_clamp(-40, 40, info.state->scroll);
        // ↑↑↑ This ensures at least 10% is visible at minimum.

        if (winput_mice_released(MouseLeft)) {
            *is_dragging = false;
        }
    }

    drawbuf_ui_draw_text(ctx, cstr(TextFormat("%d", info.state->scroll)), v2i_add(area.pos, v2ii(3)), GREEN);

    //drawbuf_DrawRectangleLines(area, BLUE, 8);
}

void ui_widget_spritesheet_list(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {

    UI_WIDGET_DEFAULT_REQUEST_HANDLER(req);

    Rect2i area = widget.area;
    const int line_height = ctx->draw.line_height * 2;
    const int text_pad = 3;
    const int thumbnail_pad = 3;

    Rect2i item_area;
    Rect2i thumbnail_area;
    V2i text_offset;

    bool show_preview = false;
    Texture preview_texture = { 0 };

    DrawRectangleReci(area, DEFAULT_BG);
    ui_draw_text(ctx, cstr_SL("Spritesheets:"), area.pos, DEFAULT_FG);
    area.y += ctx->draw.line_height;

    for (int i = 0; i < ctx->spritesheet_list.size; ++i)
    {
        Spritesheet *sheet = &ctx->spritesheet_list.items[i];

        item_area = (Rect2i) {{ area.x, area.y + i * line_height, area.width, line_height }};
        thumbnail_area = item_area;
        thumbnail_area.width = thumbnail_area.height;
        thumbnail_area = Rect2i_add_padding_all(thumbnail_area, thumbnail_pad);
        text_offset = (V2i) {{ thumbnail_area.x + thumbnail_area.width + text_pad, item_area.y + text_pad }};

        if (widget.focused && CheckCollisionPointReci(GetMousePositioni(), item_area)) {
            DrawRectangleReci(item_area, BLUE);

            if (winput_mice_held(MouseLeft)) {
                show_preview = true;
                preview_texture = sheet->texture;
            }
        }

        ui_draw_text(ctx, strbuf_view2(sheet->path), text_offset, DEFAULT_FG);

        DrawRectangleReci(Rect2i_add_padding_all(thumbnail_area, -1), BLACK);
        DrawRectangleReci(thumbnail_area, DEFAULT_BG);
        DrawTextureScaled(sheet->texture, thumbnail_area);
    }

    if (show_preview) {
        Rect2i preview_area = { .pos = GetMousePositioni(), .size = {{ preview_texture.width, preview_texture.height }} };
        DrawRectangleReci(Rect2i_add_padding_all(preview_area, -1), BLACK);
        DrawRectangleReci(preview_area, DEFAULT_BG);
        DrawTextureScaled(preview_texture, preview_area);
    }
}

void ui_widget_sprite_list(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {

    UI_WIDGET_DEFAULT_REQUEST_HANDLER(req);

    strbuf_t *aux_str = strbuf_create(0, &ctx->frame_arena.strbuf_alloc);

    Rect2i area = widget.area;

    int selected_sprite = -1;
    {
        int *selected_sprite_ref = get_selected_sprite(ctx);
        if (selected_sprite_ref != NULL) {
            selected_sprite = *selected_sprite_ref;
        }
    }

    const int line_height = ctx->draw.line_height * 2;
    const int text_pad = 3;
    const int thumbnail_pad = 3;

    Rect2i item_area;
    Rect2i thumbnail_area;
    V2i text_offset;

    DrawRectangleReci(area, DEFAULT_BG);
    ui_draw_text(ctx, cstr_SL("Sprites:"), area.pos, DEFAULT_FG);
    area.y += ctx->draw.line_height;

    for (int i = 0; i < ctx->sprites.size; ++i)
    {
        Sprite *sprite = &ctx->sprites.items[i];

        item_area = (Rect2i) {{ area.x, area.y + i * line_height, area.width, line_height }};
        thumbnail_area = item_area;
        thumbnail_area.width = thumbnail_area.height;
        thumbnail_area = Rect2i_add_padding_all(thumbnail_area, thumbnail_pad);
        text_offset = (V2i) {{ thumbnail_area.x + thumbnail_area.width + text_pad, item_area.y + text_pad }};

        bool highlight = false;

        if (selected_sprite == i) { highlight = true; }

        if (widget.focused && CheckCollisionPointReci(GetMousePositioni(), item_area)) {
            highlight = true;
            if (winput_mice_pressed(MouseLeft)) {
                // Select
                spritesheet_clear_selection(ctx);
                int_Dyna_append(&ctx->editor.selected_sprites, i);
            }
        }

        if (highlight) { DrawRectangleReci(item_area, BLUE); }

        strbuf_assign(&aux_str, cstr_SL(""));
        if (sprite->frames > 1) {
            strbuf_append_printf(&aux_str, "(%d frames) ", sprite->frames);
        }
        strbuf_append_printf(&aux_str,"%dx%d offset %d,%d\n%"PRIstr,
            sprite->size.x, sprite->size.y, sprite->offset.x, sprite->offset.y,
            PRIstrargbuf(sprite->name)
        );

        ui_draw_text(ctx, strbuf_view2(aux_str), text_offset, DEFAULT_FG);

        // Draw texture.

        DrawRectangleLinesi(Rect2i_add_padding_all(thumbnail_area, -1), BLACK, 1);
        ui_draw_sprite(ctx, sprite, thumbnail_area);
    }
}


void ui_widget_sprite_preview(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {
    UI_WIDGET_DEFAULT_REQUEST_HANDLER(req);

    int *selected_sprite_ref = get_selected_sprite(ctx);
    if (selected_sprite_ref == NULL) { return; }
    int selected_sprite = *selected_sprite_ref;

    Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, selected_sprite);
    if (sprite == NULL) { return; }

    // Calculate current frame.

    DrawRectangleReci(widget.area, DARKGRAY);
    Rect2i area = widget.area;

    // Draw name controls.
    {
        Rect2i line_box = area;
        line_box.height = ctx->draw.line_height;
        line_box.y      = area.y + area.height - line_box.height *2;

        DrawRectangleReci(line_box, DEFAULT_BG);

        Rect2i label_box = line_box;
        label_box.width -= label_box.height;

        Rect2i btn_edit_box = line_box;
        btn_edit_box.width = label_box.height;
        btn_edit_box.x += label_box.width;

        // Name.
        {
            DrawRectangleLinesi(label_box, BLACK, 1);
            strview_t view;
            if (sprite->name->size == 0) {
                view = cstr_SL("<unnamed>");
            } else {
                view = cstr(TextFormat("\"%"PRIstr"\"", PRIstrargbuf(sprite->name)));
            }
            ui_draw_text(ctx, view, label_box.pos, DEFAULT_FG);
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

        ui_draw_text(ctx, cstr_SL("Edit"), btn_edit_box.pos, DEFAULT_FG);
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

        DrawRectangleReci(line_box, DEFAULT_BG);
        ui_draw_text(ctx, cstr(TextFormat("Frames %d", sprite->frames)), label_box.pos, DEFAULT_FG);
        DrawRectangleLinesi(label_box, BLACK, 1);

        if (ui_simple_button(frame_minus_box)) {
            if (sprite->frames > 1) {
                --sprite->frames;
            }
        }
        ui_draw_text(ctx, cstr_SL("-"), frame_minus_box.pos, DEFAULT_FG);

        if (ui_simple_button(frame_plus_box)) {
            if (sprite->frames < ctx->spritesheet_list.size) {
                ++sprite->frames;
            }
        }
        ui_draw_text(ctx, cstr_SL("+"), frame_plus_box.pos, DEFAULT_FG);
    }

    area.height -= ctx->draw.line_height * 2;
    area = Rect2i_add_padding_all(area, 2);

    // Draw sprite.
    {
        BeginTextureMode(ctx->draw.aux_viewport);
        DrawRectangleReci(area, DARKGRAY);
        ui_draw_sprite(ctx, sprite, area);
        EndTextureMode();
        DrawTextureRec_flipped(ctx->draw.aux_viewport.texture, area, area.pos, WHITE);
    }
}


void ui_widget_spritesheet_cursors(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {

    Rect2i area = widget.area;
    V2i mouse = GetMousePositioni();

    const int btn_width = 30;
    Rect2i btn_area = {{
        .x = area.x + area.width -btn_width, .y = area.y,
        .width = btn_width, .height = btn_width
    }};

    {
        // Report focusable area.
        if (req != NULL && req->focus_area_request) {
            req->focus_area_success = true;
            req->focus_area = btn_area;
            req->focus_area.height *= SHEETEDITOR_CURSOR__COUNT;
        }
        UI_WIDGET_DEFAULT_REQUEST_HANDLER(req);
    }

    SHEETEDITOR_CURSOR mode;
    bool pressed;
    Color bg_color;

    // Tweak cursor. (DEFAULT)
    mode = SHEETEDITOR_CURSOR_TWEAK;
    pressed = winput_mice_pressed(MouseLeft) && widget.focused && CheckCollisionPointReci(mouse, btn_area);
    bg_color = pressed ? DARKBLUE : ctx->editor.cursor == mode ? BLUE : DEFAULT_BG;
    DrawRectangleReci(btn_area, BLACK);
    DrawRectangleReci(Rect2i_add_padding_all(btn_area, 1), bg_color);
    ui_draw_text(ctx, cstr_SL("Tweak"), btn_area.pos, DEFAULT_FG);
    if (pressed) {
        spritesheet_try_set_cursor_mode(ctx, mode);
    }
    btn_area.y += btn_area.height;

    // Add cursor.
    mode = SHEETEDITOR_CURSOR_ADD;
    pressed = winput_mice_pressed(MouseLeft) && widget.focused && CheckCollisionPointReci(mouse, btn_area);
    bg_color = pressed ? DARKBLUE : ctx->editor.cursor == mode ? BLUE : DEFAULT_BG;
    DrawRectangleReci(btn_area, BLACK);
    DrawRectangleReci(Rect2i_add_padding_all(btn_area, 1), bg_color);
    ui_draw_text(ctx, cstr_SL("Add"), btn_area.pos, DEFAULT_FG);
    if (pressed) {
        spritesheet_try_set_cursor_mode(ctx, mode);
    }
    btn_area.y += btn_area.height;

    // Reset zoom and pan button.
    bg_color = DEFAULT_BG;
    pressed = winput_mice_pressed(MouseLeft) && widget.focused && CheckCollisionPointReci(mouse, btn_area);
    bg_color = pressed ? DARKBLUE : DEFAULT_BG;
    DrawRectangleReci(btn_area, BLACK);
    DrawRectangleReci(Rect2i_add_padding_all(btn_area, 1), bg_color);
    ui_draw_text(ctx, cstr_SL("Reset\nZoom"), btn_area.pos, DEFAULT_FG);
    if (pressed) {
        zoompanel_reset_zoom_and_pan(&ctx->zoompanel);
    }
    btn_area.y += btn_area.height;
}

void ui_widget_spritesheet_viewport(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {
    UI_WIDGET_DEFAULT_REQUEST_HANDLER(req);

    strbuf_t *aux_str = strbuf_create(0, &ctx->frame_arena.strbuf_alloc);

    Rect2i area = widget.area;

    V2i mouse = GetMousePositioni();

    if (ctx->spritesheet_list.size <= 0) {
        DrawRectangleReci(widget.area, DEFAULT_BG);
        ui_draw_text(ctx, cstr_SL("No spritesheet loaded."), v2i_add(widget.area.pos, v2ii(10)), DEFAULT_FG);
        return;
    }

    // Spritesheet viewport ↓↓↓

    const Texture texture = ctx->spritesheet_list.items[0].texture;

    //Rect2i draw_area = Rect2i_add_padding_all(area, 10);
    Rect2i draw_area = area;
    V2i texture_size = v2i(texture.width, texture.height);

    zoompanel_process(&ctx->zoompanel, texture_size, draw_area);

    V2i panned_origin = v2i_add(draw_area.pos, ctx->zoompanel.offset_from_origin);
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

    BeginTextureMode(ctx->draw.aux_viewport);
    DrawRectangleReci(draw_area, DARKGRAY);
    DrawRectangleReci(final, DEFAULT_BG);
    DrawCheckerboard(final, (Color){ 0, 0, 0, 10 }, (int)(((float)16 * ctx->zoompanel.zoom)));
    DrawTextureScaled(texture, final);

    {
        // Draw current sprites.
        for (dyna_foreach(Sprite, i, ctx->sprites)) {
            Sprite *sprite = i.ref;
            Color color = Rect2i_is_out_of_bounds(sprite->rect, ctx->spritesheet_image_rect) ? RED : YELLOW;
            ui__spritesheet_draw_scaled_rect_lines2(sprite->rect, panned_origin, scale, color, 2);
        }

        // Draw selected sprites.
        for (dyna_foreach(int, i, ctx->editor.selected_sprites)) {
            int sprite_id = *i.ref;
            Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, sprite_id);
            if (sprite == NULL) { continue; }
            ui__spritesheet_draw_scaled_rect_lines(sprite->rect, panned_origin, scale, BLUE, 1);
        }
        for (dyna_foreach(int, i, ctx->editor.selected_sprites_cursor)) {
            int sprite_id = *i.ref;
            Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, sprite_id);
            if (sprite == NULL) { continue; }
            ui__spritesheet_draw_scaled_rect_lines(sprite->rect, panned_origin, scale, BLUE, 1);
        }
    }


    // Draw selection.

    if (ctx->editor.cursor == SHEETEDITOR_CURSOR_TWEAK
        || ctx->editor.cursor == SHEETEDITOR_CURSOR_ADD
        || ctx->editor.cursor == SHEETEDITOR_CURSOR_RESIZE
    ) {
        ctx->editor.mouse_inside = widget.focused && CheckCollisionPointReci(mouse, draw_area);

        // Holding.
        if (ctx->editor.mouse_is_selecting) {
            Rect2i selection = ctx->editor.selection;
            ui__spritesheet_draw_scaled_rect_lines(
                selection,
                panned_origin, scale, GREEN, 1);
            
            strview_t text = strbuf_printf(&aux_str, "%d x %d", selection.size.x, selection.size.y);
            ui_draw_text_highlighted(
                ctx, text,
                v2f_2i(v2f_translate_scale(v2i_add(selection.pos, v2ii(1)), panned_origin, scale)),
                WHITE, BLACK);
        }
    }

    // Draw cursor position scaled.

    ui__spritesheet_draw_scaled_rect((Rect2i){.pos = ctx->editor.mouse_pos, .size = v2ii(1)}, panned_origin, scale, GREEN);

    EndTextureMode();

    DrawTextureRec_flipped(ctx->draw.aux_viewport.texture,
            draw_area, draw_area.pos, WHITE);
}


void ui_widget_spritesheet_hints(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {
    {
        // This widget is unfocusable.
        // Report back as having NO focus area at all.
        if (req != NULL && req->focus_area_request) {
            req->focus_area_success = 1;
            req->focus_area = (Rect2i) { 0 };
            return;
        }
    }

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
    case SHEETEDITOR_CURSOR_RESIZE:
    {
        text = cstr_SL(
            "Left click to confirm.\n"
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
    ui_draw_text(ctx, text, v2i_sub(v2i_add(widget.area.pos, widget.area.size), measure), DEFAULT_FG);
}

void ui_widget_spritesheet(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {

    UI_WIDGET_DEFAULT_REQUEST_HANDLER(req);

    static Widget widgets[] = {
        { .draw_function = ui_widget_spritesheet_viewport, },
        { .draw_function = ui_widget_spritesheet_cursors   },
        { .draw_function = ui_widget_spritesheet_hints     },
    };

    for (foreach_auto(Widget, w, widgets)) {
        w.ref->screen_area = widget.area;
    }

    ui__calculate_focus_and_draw_widgets(ctx, widgets, countof(widgets));
}


/*
/// @returns Index of widget with focus or -1.
int ui__calculate_focus(const Widget_view widgets) {
    for (int i = widgets.size-1; i > -1; --i) {
        Rect2i area = widgets.items[i].focus_area;
        if (Rect2i_collides_V2i(area, GetMousePositioni())) { return i; }
    }
    return -1;
}
*/


void ui__calculate_focus_and_draw_widgets(Ctx *ctx, Widget *widgets, const int count) {

    // Query focus areas.

    /*
    for (int i = 0; i < count; ++i)
    {
        Widget *w = &widgets[i];
        WidgetReq req = { .focus_area_request = true };
        w->draw_function(ctx, (WidgetDraw){ .area = w->screen_area }, &req);

        if (req.focus_area_success) {
            w->focus_area = req.focus_area;
        } else {
            printfd("WAR: Widget %d didn't respond.", i);
            w->focus_area = w->screen_area;
        }
    }
    */

    // Reset focus.

    for (int i = 0; i < count; ++i) {
        Widget *w = &widgets[i];
        w->focused = false;
    }

    // Calculate focus in reverse.

    for (int i = count -1; i > -1; i += -1) {
        Widget *w = &widgets[i];
        if (Rect2i_collides_V2i(w->focus_area, GetMousePositioni())) {
            w->focused = true;
            break;
        }
    }

    // Sync draw area.

    for (int i = 0; i < count; ++i) {
        Widget *w = &widgets[i];
        w->draw_info = (WidgetDraw) {
            .focused = w->focused,
            .area = w->screen_area,
        };
    }

    // Calculate scroll.

    for (int i = 0; i < count; ++i) {
        Widget *w = &widgets[i];
        ui_widget_scroll(w);
    }

    // Draw.

    for (int i = 0; i < count; ++i) {
        Widget *w = &widgets[i];

        // Prepare some request to get some info about these widgets.

        WidgetReq req = {
            .focus_area_request = true,
            .scroll_max_px_request = w->scroll_enabled,
        };

        // Finally draw on screen.

        w->draw_function(ctx, w->draw_info, &req);

        // Handle query responses.

        if (req.scroll_max_px_success) {
            w->scroll_max_px = req.scroll_max_px;
        }
        if (req.focus_area_success) {
            w->focus_area = req.focus_area;
        } else {
            printfd("WAR: Widget %d didn't respond.", i);
            w->focus_area = w->screen_area;
        }
    }
}

/*
void ui_draw_all(Ctx *ctx) {

    // Collect draw functions.

    static Widget widgets[] = {
        [0] = { .draw_function = ui_widget_options         ,                         },
        [1] = { .draw_function = ui_widget_spritesheet_list, .scroll_enabled = true, },
        [2] = { .draw_function = ui_widget_sprite_list     , .scroll_enabled = true, },
        [3] = { .draw_function = ui_widget_sprite_preview  ,                         },
        [4] = { .draw_function = ui_widget_spritesheet     ,                         },
    };

    // Subdivide window horizontally.

    Rect2i area = {{ 0, 0, GetScreenWidth(), GetScreenHeight() }};
    Rect2i chunks[4];
    Rect2i_split_horizontally(area, 4, chunks, { 0.1f, 0.15f, 0.20f });

    Rect2i chunk_sprite_list = chunks[2];
    Rect2i up = chunk_sprite_list;
    up.height /= 2;
    Rect2i down = chunk_sprite_list;
    down.y += up.height;
    down.height = chunk_sprite_list.height - up.height;

    widgets[0].screen_area = chunks[0];
    widgets[1].screen_area = chunks[1];
    widgets[2].screen_area = up;
    widgets[3].screen_area = down;
    widgets[4].screen_area = chunks[3];

    const int PAD = 2;
    for(foreach_auto(Widget, widget, widgets)) {
        widget.ref->screen_area = Rect2i_add_padding_all(widget.ref->screen_area, PAD);
    }

    ui__calculate_focus_and_draw_widgets(ctx, widgets, countof(widgets));
}
*/

/*
void ui_split_h(Ctx *ctx, const Rect2i area, int child_count, Widget *children, WidgetContainer *container) {

    // State here
    //float split = data.split;

    Rect2i chunk_area = {{ 0, 0, 0, area.height }};
    for (int i = 0; i < 0; ++i) {
        chunk_area.x += area.width / child_count;
        children[i].screen_area = chunk_area;
    }

    DrawRectangleReci(area, DEFAULT_BG);
    ui__calculate_focus_and_draw_widgets(ctx, children, child_count);
}

void ui_draw_all_mimo(Ctx *ctx) {
    static bool setup = false;

    WidgetMeta spritesheet_list = {
        .type = WIDGET_TYPE_NORMAL,
        .normal.draw_function = ui_widget_spritesheet_list,
    };

    WidgetMeta sprite_list = {
        .type = WIDGET_TYPE_NORMAL,
        .normal.draw_function = ui_widget_sprite_list,
    };

    WidgetMeta editor = {
        .type = WIDGET_TYPE_CONTAINER,
        .container.container_function = ui_split_h,
    };

    WidgetMeta widgets[2] = { spritesheet_list, sprite_list };

    ui_split_h(ctx, countof(2), widgets);


    //for (int
}
*/

/*
void ui_split_options_AND_sprite_list(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {
    static Widget widgets[] = {
        [0] = { .draw_function = ui_widget_options    ,                         },
        [2] = { .draw_function = ui_widget_sprite_list, .scroll_enabled = true, },
    };
    static WidgetContainer split_state = {
        .type = CONTAINER_TYPE_SPLIT_H_PERCENTAGE,
    };
    ui_split_h(ctx, widget.area, countof(widgets), widgets, &split_state);
}


void ui_draw_all2(Ctx *ctx) {

    static Widget widgets[] = {
        { .draw_function = ui_split_options_AND_sprite_list,                         },
        { .draw_function = ui_widget_spritesheet           ,                         },
        { .draw_function = ui_split_options_AND_sprite_list,                         },
    };

    static WidgetContainer split_state = {
        .type = CONTAINER_TYPE_SPLIT_H_PERCENTAGE,
        .split.adjustable = true,
        .split.percentage = 0.2f,
    };

    Rect2i area = {{ 0, 0, GetScreenWidth(), GetScreenHeight() }};

    ui_split_h(ctx, area, countof(widgets), widgets, &split_state);
}
*/


//#include "./src/standalone/uitree.h"


void widget_vlist(Rect2i area, int child_count, Rect2i *children, void *user_ctx, uitree_WidgetState *state) {
    (void)user_ctx, (void)state;
    for (int i = 0; i < child_count; ++i) {
        Rect2i *child = &children[i];
        child->width = area.width;
        child->x = area.x;
        child->height = area.height / child_count;
        child->y = area.y + child->height * i;
    }
}

// Only two children supported.
void widget_vsplit(Rect2i area, int child_count, Rect2i *children, void *user_ctx, uitree_WidgetState *state) {
    const int pad = 2;
    float percentage = (float)(state->scroll + 50) / 100.f;
    (void)user_ctx;

    if (child_count > 0) {
        Rect2i *child = &children[0];
        child->width = area.width;
        child->x = area.x;
        child->height = (int)((float)area.height * percentage);
        child->y = area.y;
    }
    if (child_count > 1) {
        Rect2i *child = &children[1];
        child->width = area.width;
        child->x = area.x;
        child->height = (int)((float)area.height * (1.0f - percentage));
        child->y = area.y + children[0].height;
    }
    children[0] = Rect2i_add_padding_all(children[0], pad);
    children[1] = Rect2i_add_padding_all(children[1], pad);

    // This rect will be used for dragging.
    state->rect_a = (Rect2i) {
        .x      = children[0].x,
        .width  = children[0].width,
        .y      = children[0].y + children[0].height,
        .height = pad * 2,
    };

    if (child_count > 2) { printfd("WAR: Too many children."); }
}




void widget_simple_text(Rect2i area, bool focused, Ctx *ctx, int child_count, Rect2i *children) {
    DrawRectangleReci(area, BLUE);
    DrawRectangleLinesi(area, BLUE, 2);
    ui_draw_text(ctx, cstr_SL("HELLO"), area.pos, RED);
}


#define WIDGET__TABLE \
X( UI_WIDGET_OPTIONS    , ui_widget_options      ) \
X( UI_WIDGET_SPRITE_LIST, ui_widget_options      ) \
X( UI_WIDGET_VSPLIT     , ui_widget_vsplit       ) \
X( UI_WIDGET_VSPLIT_DRAG, ui_widget_vsplit_drag  )

enum UI_WIDGET {
    #define X(A, ...) A,
    WIDGET__TABLE
    #undef X
    UI_WIDGET_AMOUNT
};

// ↓↓↓ Maps UI_WIDGET to function pointer.

void (*widget_func[]) (Ctx *ctx, uitree_DrawInfo info) = {
    #define X(A, B) B,
    WIDGET__TABLE
    #undef X
};


void ui_draw_all3(Ctx *ctx) {

    static Uitree tree = { 0 };
    static bool setup = false;
    if (!setup) {
        setup = true;
        uitree_create(&tree);
    }

    uitree_build_start(&tree, (Rect2i){{.width = GetScreenWidth(), .height = GetScreenHeight()+1}});

    uitree_Node con_tree = uitree_dumb_container(&tree, cstr_SL(""), widget_vlist);

    uitree_Node con_vlist = uitree_dumb_container(&tree, cstr_SL("MAIN_V_LIST"), widget_vlist);

    {
        uitree_Node widget_config = uitree_widget(UI_WIDGET_OPTIONS);
        uitree_container_add_child(&tree, &con_vlist, widget_config);
    }

    {
        uitree_Node con_hlist = uitree_container(&tree, cstr_SL("SECONDARY_H_SPLIT"),
                widget_vsplit, UI_WIDGET_VSPLIT_DRAG);

        {
            uitree_Node widget_sprite_list = uitree_widget(UI_WIDGET_SPRITE_LIST);
            uitree_container_add_child(&tree, &con_hlist, widget_sprite_list);

            widget_sprite_list = uitree_widget(UI_WIDGET_SPRITE_LIST);
            uitree_container_add_child(&tree, &con_hlist, widget_sprite_list);
        }

        uitree_container_add_child(&tree, &con_vlist, con_hlist);
    }

    {
        uitree_Node widget_config = uitree_widget(UI_WIDGET_OPTIONS);
        uitree_container_add_child(&tree, &con_vlist, widget_config);
    }

    uitree_container_add_child(&tree, &con_tree, con_vlist);

    tree.root_node = con_tree;
    uitree_build_end(&tree);

    uitree_List_DrawInfo_It it = { 0 };
    while(uitree_List_DrawInfo_it_next(&tree.out_draw_list, &it)) {
        uitree_DrawInfo draw = *it.item;
        drawbuf_set_layer((uint8_t)draw.layer);
        printfd("(user space) Widget right here! :) "Rect2i_Fmt" user_fun_id %d,", Rect2i_Arg(draw.area), draw.user_draw_func_id);

        widget_func[it.item->user_draw_func_id](ctx, draw);
    }
    drawbuf_draw_all();

    printfd("Arena consumption is "PRIbyte" out of "PRIbyte, PRIbytearg((1 << 20) - (tree.arena.end - tree.arena.beg)), PRIbytearg(1 << 20));
}

#endif // !UI
