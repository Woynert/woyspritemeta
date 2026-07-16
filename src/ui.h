#ifndef UI
#define UI

#include "operations.h"
#include "raylib_extra.h"
#include "la_extra.h"
#include "raylib.h"
#include "la.h"
#include "winput.h"
#include "rlgl.h"

#define UI_WIDGET_HANDLE_REQUEST_AND_RETURN(req) do {            \
    if (req != NULL) {                                             \
        if (req->focus_area_request && !req->focus_area_success) { \
            req->focus_area_success = true;                        \
            req->focus_area = widget.area;                         \
        }                                                          \
        if (req->max_area_request && !req->max_area_success) {     \
            req->max_area_success = true;                          \
            req->max_area = widget.area;                           \
        }                                                          \
        return;                                                    \
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

void ui_widget_options(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {

    UI_WIDGET_HANDLE_REQUEST_AND_RETURN(req);

    Rect2i area = widget.area;
    const int line_height = ctx->draw.line_height;
    int line = 0;
    V2i pos = { 0 };
    const int pad = 10;

    pos = (V2i) {{ area.x + pad, area.y + line * line_height }};
    ++line;

    Rect2i line_area = {{ pos.x, pos.y, area.width -pad, line_height }};
    line_area.y += line_height;

    DrawRectangleReci(area, DEFAULT_BG);
    ui_draw_text(ctx, cstr_SL("Options:"), pos, DEFAULT_FG);

    for (int i = 0; i < ctx->actions.size; ++i)
    {
        Action *action = &ctx->actions.items[i];
        pos = (V2i) {{ (int)area.x + pad, (int)area.y + line * ctx->draw.line_height }};

        if (widget.focused && CheckCollisionPointReci(GetMousePositioni(), line_area)) {
            DrawRectangleReci(line_area, BLUE);

            if (winput_mice_pressed(MouseLeft)) {
                call_action(ctx, action);
            }
        }

        ui_draw_text(ctx, strbuf_view2(action->name), pos, DEFAULT_FG);

        ++line;
        line_area.y += line_height;
    }
}

void ui_widget_spritesheet_list(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {

    UI_WIDGET_HANDLE_REQUEST_AND_RETURN(req);

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

    UI_WIDGET_HANDLE_REQUEST_AND_RETURN(req);

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

        if (sprite->frames == 1) {
            strbuf_printf(&aux_str, "%"PRIstr" %dx%d offset %d,%d",
                PRIstrargbuf(sprite->name),
                sprite->size.x, sprite->size.y, sprite->offset.x, sprite->offset.y
            );
        } else {
            strbuf_printf(&aux_str, "%"PRIstr" (%d frames) %dx%d offset %d,%d",
                PRIstrargbuf(sprite->name), sprite->frames,
                sprite->size.x, sprite->size.y, sprite->offset.x, sprite->offset.y
            );
        }
        ui_draw_text(ctx, strbuf_view2(aux_str), text_offset, DEFAULT_FG);

        // Draw texture.

        DrawRectangleLinesi(Rect2i_add_padding_all(thumbnail_area, -1), BLACK, 1);
        ui_draw_sprite(ctx, sprite, thumbnail_area);
    }
}


void ui_widget_sprite_preview(Ctx *ctx, const WidgetDraw widget, WidgetReq *req) {
    UI_WIDGET_HANDLE_REQUEST_AND_RETURN(req);

    int *selected_sprite_ref = get_selected_sprite(ctx);
    if (selected_sprite_ref == NULL) { return; }
    int selected_sprite = *selected_sprite_ref;

    Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, selected_sprite);
    if (sprite == NULL) { return; }

    // Calculate current frame.

    DrawRectangleReci(widget.area, DARKGRAY);
    Rect2i area = widget.area;

    // Draw controls
    {
        Rect2i line_box = area;
        line_box.height = ctx->draw.line_height;
        line_box.y      = area.y + area.height - line_box.height;

        Rect2i chunks[3];
        Rect2i_split_horizontally(line_box, 3, chunks, { 1/2.f, 1/4.f });

        Rect2i label_box       = chunks[0];
        Rect2i frame_minus_box = chunks[1];
        Rect2i frame_plus_box  = chunks[2];

        DrawRectangleLinesi(line_box, DEFAULT_BG, 1);
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
        UI_WIDGET_HANDLE_REQUEST_AND_RETURN(req);
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
    UI_WIDGET_HANDLE_REQUEST_AND_RETURN(req);

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

    UI_WIDGET_HANDLE_REQUEST_AND_RETURN(req);

    Widget widgets[] = {
        { .draw = { .area = widget.area }, .draw_function = ui_widget_spritesheet_viewport, },
        { .draw = { .area = widget.area }, .draw_function = ui_widget_spritesheet_cursors },
        { .draw = { .area = widget.area }, .draw_function = ui_widget_spritesheet_hints },
    };

    ui__calculate_focus_and_draw_widgets(ctx, widgets, countof(widgets));
}


/// @returns Index of widget with focus or -1.
int ui__calculate_focus(const Widget_view widgets) {
    for (int i = widgets.size-1; i > -1; --i) {
        Rect2i area = widgets.items[i].focus_area;
        if (Rect2i_collides_V2i(area, GetMousePositioni())) { return i; }
    }
    return -1;
}


void ui__calculate_focus_and_draw_widgets(Ctx *ctx, Widget *widgets, int count) {
    // Update focus areas.

    for (int i = 0; i < count; ++i)
    {
        Widget *w = &widgets[i];
        WidgetReq req = { .focus_area_request = true };

        (w->draw_function)(ctx, w->draw, &req);

        if (req.focus_area_success) {
            w->focus_area = req.focus_area;
        } else {
            printfd("WAR: Widget %d didn't respond.", i);
            w->focus_area = w->draw.area;
        }
    }

    int focus = ui__calculate_focus((Widget_view){ widgets, count });
    if (int_in_range_inclusive(0, count-1, focus)) {
        widgets[focus].draw.focused = true;
    }

    // Draw.

    for (int i = 0; i < count; ++i) {
        Widget *w = &widgets[i];
        (w->draw_function)(ctx, w->draw, NULL);
    }
}

void ui_draw_all(Ctx *ctx) {

    // Collect draw functions.

    Widget widgets[] = {
        { .draw_function = ui_widget_options,          },
        { .draw_function = ui_widget_spritesheet_list, },
        { .draw_function = ui_widget_sprite_list,      },
        { .draw_function = ui_widget_sprite_preview,   },
        { .draw_function = ui_widget_spritesheet,      },
    };
    int percentages[] = {
        10,
        15,
        15,
        10,
        50
    };
    wstatic_assert(countofi(widgets) == countofi(percentages));

    // Subdivide window horizontally.

    const int PAD = 2;
    Rect2i area = {{ 0, 0, 0, GetScreenHeight() }};

    for(foreach_auto(Widget, widget, widgets))
    {
        area.x += area.width;
        area.width = (int)(((float)percentages[widget.index] / 100.f) * (float)GetScreenWidth());
        widget.ref->draw.area = Rect2i_add_padding_all(area, PAD);
    }

    ui__calculate_focus_and_draw_widgets(ctx, widgets, countof(widgets));
}

#endif // !UI
