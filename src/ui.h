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


#define DEFAULT_BG LIGHTGRAY
#define DEFAULT_FG BLACK
#define ANIMATION_TICKS_PER_FRAME 15


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

bool ui__simple_button(const int id, Rect2i rect) {
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

Rect2i b_ui_draw_sprite(Ctx *ctx, Sprite *sprite, Rect2i area) {
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

    b_DrawTextureScaled2(sheet->texture, final, sprite->rect);
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


void widget_stack(Rect2i area, int child_count, Rect2i *children, void *user_ctx, uitree_WidgetState *state) {
    (void)user_ctx, (void)state;
    for (int i = 0; i < child_count; ++i) { children[i] = area; }
}


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

typedef struct widget_vsplit_state_t {
    float *size;
    int *is_percentage_or_px;
    int *is_dragging;
    Rect2i *drag_area;
} widget_vsplit_state_t;

widget_vsplit_state_t widget_vsplit_get_state(uitree_WidgetState *state) {
    return (widget_vsplit_state_t) {
        .size                = &state->float_a,
        .is_percentage_or_px = &state->int_b,
        .is_dragging         = &state->int_c,
        .drag_area           = &state->rect_a,
    };
}

void widget_vsplit_set_user_default_state(Uitree *tree, uitree_Node *node, int size, bool is_percentage_or_px) {
    const float PAD = 2;
    uitree_WidgetState *state = arena_new(&tree->arena, uitree_WidgetState, 1);
    if (state == NULL) { return; }
    widget_vsplit_state_t vars = widget_vsplit_get_state(state);

    *vars.is_percentage_or_px = is_percentage_or_px;
    if (is_percentage_or_px == 0) {
        *vars.size = (float)(size - 50) / 100.f; // %
    } else {
        *vars.size = (float)size + PAD;          // px
    }
    node->user_default_state = state;
}

void widget_vsplit(Rect2i area, int child_count, Rect2i *children, void *user_ctx, uitree_WidgetState *state) {
    const int PAD = 2;
    const widget_vsplit_state_t vars = widget_vsplit_get_state(state);
    int first_child_height;

    // Determines whether the sizes are percentages (relative) or pixels (absolute).
    if (*vars.is_percentage_or_px == 0) {
        //float percentage = (float)(*vars.size + 50) / 100.f;
        first_child_height = (int)((float)area.height * (*vars.size + 0.5f));
    } else {
        first_child_height = (int)*vars.size;
    }
    (void)user_ctx;

    if (child_count > 0) {
        Rect2i *child = &children[0];
        child->width = area.width;
        child->x = area.x;
        child->height = first_child_height;
        child->y = area.y;
    }
    if (child_count > 1) {
        Rect2i *child = &children[1];
        child->width = area.width;
        child->x = area.x;
        child->height = area.height - children[0].height;
        child->y = area.y + children[0].height;
    }
    children[0] = Rect2i_add_padding_all(children[0], PAD);
    children[1] = Rect2i_add_padding_all(children[1], PAD);

    // This rect will be used for dragging.
    state->rect_a = (Rect2i) {
        .x      = children[0].x,
        .width  = children[0].width,
        .y      = children[0].y + children[0].height,
        .height = PAD * 2,
    };

    if (child_count > 2) { printfd("WAR: Too many children."); }
}

void ui_widget_vsplit_drag(Ctx *ctx, uitree_DrawInfo info) {
    (void)ctx;
    Rect2i area = info.area;
    widget_vsplit_state_t vars = widget_vsplit_get_state(info.state);

    if (mice_in_rect(*vars.drag_area) || *vars.is_dragging) {
        b_DrawRectangle(*vars.drag_area, ORANGE);
        if (mice_pressed(MouseLeft)) {
            mice_consume(MouseLeft);
            *vars.is_dragging = true;
        }
    }

    if (*vars.is_dragging) {

        int mouse_y = GetMousePositioni().y;

        if (*vars.is_percentage_or_px == 0) {
            *vars.size = ((float)mouse_y - ((float)area.y + (float)area.height / 2.0f)) / (float)area.height;
            *vars.size = float_clamp(-0.45f, 0.45f, *vars.size);
        } else {
            float factor = ((float)mouse_y - (float)area.y) / (float)area.height;
            *vars.size = factor * (float)area.height;
            *vars.size = (float)int_clamp(30, area.height-30, (int)*vars.size);
        }
        // ↑↑↑ This ensures at least a % is visible at minimum.

        if (mice_released(MouseLeft)) {
            *vars.is_dragging = false;
        }
    }
}


typedef struct widget_3hsplit_state_t {
    int *is_setup;
    int *is_dragging;
    float *percentage1;
    float *percentage2;
    Rect2i *drag_area1;
    Rect2i *drag_area2;
} widget_3hsplit_state_t;

widget_3hsplit_state_t widget_3hsplit_get_state(uitree_WidgetState *state) {
    return (widget_3hsplit_state_t) {
        .is_setup = &state->int_a,
        .is_dragging = &state->int_b,
        .percentage1 = &state->float_a,
        .percentage2 = &state->float_b,
        .drag_area1 = &state->rect_a,
        .drag_area2 = &state->rect_b,
    };
}

void widget_3hsplit_set_user_default_state(Uitree *tree, uitree_Node *node, int p_percentage1, int p_percentage2) {
    uitree_WidgetState *state = arena_new(&tree->arena, uitree_WidgetState, 1);
    if (state == NULL) { return; }
    widget_3hsplit_state_t vars = widget_3hsplit_get_state(state);
    node->user_default_state = state;
    *vars.is_setup = true;
    *vars.percentage1 = (float)p_percentage1/100.f;
    *vars.percentage2 = (float)p_percentage2/100.f;
}

void widget_3hsplit(Rect2i area, int child_count, Rect2i *children, void *user_ctx, uitree_WidgetState *state) {
    (void)user_ctx;
    const int pad = 2;
    const widget_3hsplit_state_t vars = widget_3hsplit_get_state(state);
    if (child_count > 0) {
        Rect2i *child = &children[0];
        child->height = area.height;
        child->y = area.y;
        child->width = (int)((float)area.width * *vars.percentage1);
        child->x = area.x;
    }
    if (child_count > 1) {
        Rect2i *child = &children[1];
        child->height = area.height;
        child->y = area.y;
        child->width = (int)((float)area.width * *vars.percentage2) - children[0].width;
        child->x = children[0].x + children[0].width;
    }
    if (child_count > 2) {
        Rect2i *child = &children[2];
        child->height = area.height;
        child->y = area.y;
        child->width = area.width - children[0].width - children[1].width;
        child->x = children[1].x + children[1].width;
    }
    children[0] = Rect2i_add_padding_all(children[0], pad);
    children[1] = Rect2i_add_padding_all(children[1], pad);
    children[2] = Rect2i_add_padding_all(children[2], pad);
    *vars.drag_area1 = (Rect2i) {
        .y      = children[0].y,
        .height = children[0].height,
        .x      = children[0].x + children[0].width,
        .width = pad * 2,
    };
    *vars.drag_area2 = (Rect2i) {
        .y      = children[0].y,
        .height = children[0].height,
        .x      = children[1].x + children[1].width,
        .width = pad * 2,
    };
    if (child_count > 3) { printfd("WAR: Too many children."); }
}

void ui_widget_3hsplit_drag(Ctx *ctx, uitree_DrawInfo info) {

    enum { NOT_DRAGGING, IS_DRAGGING_1ND, IS_DRAGGING_2ND };

    const float PAD = 0.02f;
    Rect2i area = info.area;
    widget_3hsplit_state_t vars = widget_3hsplit_get_state(info.state);

    if (!*vars.is_setup) {
        *vars.is_setup = true;
        *vars.percentage1 = 0.33f;
        *vars.percentage2 = 0.66f;
    }

    if (*vars.percentage2 == 0) { *vars.percentage2 = 50; }

    if (mice_in_rect(*vars.drag_area1) || (*vars.is_dragging == IS_DRAGGING_1ND)) {
        b_DrawRectangle(*vars.drag_area1, BLUE);
        if (mice_pressed_consume(MouseLeft)) {
            *vars.is_dragging = IS_DRAGGING_1ND;
        }
    }

    else if (mice_in_rect(*vars.drag_area2) || (*vars.is_dragging == IS_DRAGGING_2ND)) {
        b_DrawRectangle(*vars.drag_area2, RED);
        if (mice_pressed_consume(MouseLeft)) {
            *vars.is_dragging = IS_DRAGGING_2ND;
        }
    }

    if (*vars.is_dragging == IS_DRAGGING_1ND) {
        int mouse_x = GetMousePositioni().x;
        *vars.percentage1 = ((float)mouse_x - (float)area.x) / (float)area.width;
        *vars.percentage1 = float_clamp(PAD, *vars.percentage2 - PAD, *vars.percentage1);
    }
    if (*vars.is_dragging == IS_DRAGGING_2ND) {
        int mouse_x = GetMousePositioni().x;
        *vars.percentage2 = ((float)mouse_x - (float)area.x) / (float)area.width;
        *vars.percentage2 = float_clamp(*vars.percentage1 + PAD, 1.f - PAD, *vars.percentage2);
    }
    if (*vars.is_dragging != NOT_DRAGGING && mice_released(MouseLeft)) {
        *vars.is_dragging = NOT_DRAGGING;
    }

    b_ui_draw_text(ctx, cstr(TextFormat("%d:%d", *vars.percentage1, *vars.percentage2)), v2i_add(area.pos, v2ii(3)), GREEN);
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

    for (int i = 0; i < ctx->spritesheet_list.size; ++i)
    {
        Spritesheet *sheet = &ctx->spritesheet_list.items[i];

        item_area = (Rect2i) {{ area_scroll.x, area_scroll.y + i * item_height, area_scroll.width, item_height }};
        thumbnail_area = item_area;
        thumbnail_area.width = thumbnail_area.height;
        thumbnail_area = Rect2i_add_padding_all(thumbnail_area, thumbnail_pad);
        text_offset = (V2i) {{ thumbnail_area.x + thumbnail_area.width + text_pad, item_area.y + text_pad }};

        if (mouse_focus && mice_in_rect(item_area)) {
            b_DrawRectangle(item_area, BLUE);

            if (mice_held(MouseLeft)) {
                uint8_t layer_bk = drawbuf_get_layer();
                drawbuf_set_layer(200);
                Rect2i preview_area = { .pos = GetMousePositioni(), .size = {{ sheet->texture.width, sheet->texture.height }} };
                preview_area = Rect2i_stay_within_Rect2i(preview_area, (Rect2i){{.width = GetScreenWidth(), .height = GetScreenHeight()}});
                b_DrawRectangle(Rect2i_add_padding_all(preview_area, -1), BLACK);
                b_DrawRectangle(preview_area, DEFAULT_BG);
                b_DrawTextureScaled(sheet->texture, preview_area);
                drawbuf_set_layer(layer_bk);
            }
        }

        b_ui_draw_text(ctx, strbuf_view2(sheet->path), text_offset, DEFAULT_FG);

        b_DrawRectangle(Rect2i_add_padding_all(thumbnail_area, -1), BLACK);
        b_DrawRectangle(thumbnail_area, DEFAULT_BG);
        b_DrawTextureScaled(sheet->texture, thumbnail_area);
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

    area_s.y += ctx->draw.line_height;
    area_scroll_viewport = area_s;
    area_scroll_viewport.height -= ctx->draw.line_height;
    b_BeginScissorMode(area_scroll_viewport);

    bool mouse_focus = mice_in_rect(area_s);
    if (mouse_focus) {
        ui__calculate_fancy_scroll_px(
                scroll_px, scroll_vel_px,
                area_scroll_viewport.height,
                item_height * (ctx->sprites.size + 1), int_sign(mice_wheel()));
    };
    area_s.y += *scroll_px;

    for (int i = 0; i < ctx->sprites.size; ++i)
    {
        Sprite *sprite = &ctx->sprites.items[i];

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
        b_ui_draw_sprite(ctx, sprite, thumbnail_area);
    }
    b_EndScissorMode();

    b_DrawRectangleLines(area, MAGENTA, 1);
    b_DrawRectangleLines(area_scroll_viewport, MAGENTA, 1);
}


void ui_widget_sprite_preview(Ctx *ctx, uitree_DrawInfo info) {

    int *selected_sprite_ref = get_selected_sprite(ctx);
    if (selected_sprite_ref == NULL) { return; }
    int selected_sprite = *selected_sprite_ref;

    Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, selected_sprite);
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
            b_ui_draw_text(ctx, view, label_box.pos, DEFAULT_FG);
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

        b_ui_draw_text(ctx, cstr_SL("Edit"), btn_edit_box.pos, DEFAULT_FG);
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
        b_ui_draw_text(ctx, cstr(TextFormat("Frames %d", sprite->frames)), label_box.pos, DEFAULT_FG);
        b_DrawRectangleLines(label_box, BLACK, 1);

        if (ui_simple_button(frame_minus_box)) {
            if (sprite->frames > 1) {
                --sprite->frames;
            }
        }
        b_ui_draw_text(ctx, cstr_SL("-"), frame_minus_box.pos, DEFAULT_FG);

        if (ui_simple_button(frame_plus_box)) {
            if (sprite->frames < ctx->spritesheet_list.size) {
                ++sprite->frames;
            }
        }
        b_ui_draw_text(ctx, cstr_SL("+"), frame_plus_box.pos, DEFAULT_FG);
    }

    area.height -= ctx->draw.line_height * 2;
    area = Rect2i_add_padding_all(area, 2);

    // Draw sprite.
    {
        BeginTextureMode(ctx->draw.aux_viewport);
        DrawRectangleReci(area, DARKGRAY);
        ui_draw_sprite(ctx, sprite, area);
        EndTextureMode();
        b_DrawTextureRec_flipped(ctx->draw.aux_viewport.texture, area, area.pos, WHITE);
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

    if (ctx->spritesheet_list.size <= 0) {
        b_DrawRectangle(area, DEFAULT_BG);
        b_ui_draw_text(ctx, cstr_SL("No spritesheet loaded."), v2i_add(area.pos, v2ii(10)), DEFAULT_FG);
        return;
    }

    // Spritesheet viewport ↓↓↓

    const Texture texture = ctx->spritesheet_list.items[0].texture;

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
    b_DrawRectangle(final, DEFAULT_BG);
    b_DrawCheckerboard(final, (Color){ 0, 0, 0, 10 }, (int)(((float)16 * ctx->zoompanel.zoom)));
    b_DrawTextureScaled(texture, final);

    {
        // Draw current sprites.
        for (dyna_foreach(Sprite, i, ctx->sprites)) {
            Sprite *sprite = i.ref;
            Color color = Rect2i_is_out_of_bounds(sprite->rect, ctx->spritesheet_image_rect) ? RED : YELLOW;
            ui__spritesheet_draw_scaled_rect_lines2(sprite->rect, panned_origin, (int)scale, color, 2);
        }

        // Draw selected sprites.
        for (dyna_foreach(int, i, ctx->editor.selected_sprites)) {
            int sprite_id = *i.ref;
            Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, sprite_id);
            if (sprite == NULL) { continue; }
            ui__spritesheet_draw_scaled_rect_lines(sprite->rect, panned_origin, (int)scale, BLUE, 1);
        }
        for (dyna_foreach(int, i, ctx->editor.selected_sprites_cursor)) {
            int sprite_id = *i.ref;
            Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, sprite_id);
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
        widget_3hsplit_set_user_default_state(t, &con_3split, 10, 90);

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
