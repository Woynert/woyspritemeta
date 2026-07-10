#ifndef UI
#define UI

#include "operations.h"
#include "raylib_extra.h"
#include "raylib.h"
#include "la.h"
#include "better_mouse_input.h"

void ui_draw_text(Ctx *ctx, strview_t str, V2i pos, Color tint) {
	DrawTextEx_strview_i(ctx->draw.font, str, pos, ctx->draw.font_size,
		ctx->draw.char_spacing, ctx->draw.line_spacing, tint);
}

void ui_draw_options(Ctx *ctx, Rect2i area) {

	const int line_height = ctx->draw.line_height;
	int line = 0;
	V2i pos = { 0 };
	const int pad = 10;

	pos = (V2i) {{ area.x + pad, area.y + line * line_height }};
	++line;

	Rect2i line_area = {{ pos.x, pos.y, area.width -pad, line_height }};
	line_area.y += line_height;

	DrawRectangleReci(area, LIGHTGRAY);
	ui_draw_text(ctx, cstr_SL("Options:"), pos, BLACK);

	for (int i = 0; i < ctx->actions.size; ++i)
	{
		Action *action = &ctx->actions.items[i];
		pos = (V2i) {{ (int)area.x + pad, (int)area.y + line * ctx->draw.line_height }};

		if (CheckCollisionPointReci(GetMousePositioni(), line_area)) {
			DrawRectangleReci(line_area, BLUE);

			if (BetterMouse_is_pressed(MOUSE_BUTTON_LEFT)) {
				call_action(ctx, action);
			}
		}

		ui_draw_text(ctx, strbuf_view2(action->name), pos, BLACK);

		++line;
		line_area.y += line_height;
	}
}

void ui_draw_spritesheet_list(Ctx *ctx, Rect2i area) {

	const int line_height = ctx->draw.line_height * 2;
	const int text_pad = 3;
	const int thumbnail_pad = 3;

	Rect2i item_area;
	Rect2i thumbnail_area;
	V2i text_offset;

	bool show_preview = false;
	Texture preview_texture = { 0 };

	DrawRectangleReci(area, LIGHTGRAY);
	ui_draw_text(ctx, cstr_SL("Spritesheets:"), area.pos, BLACK);
	area.y += ctx->draw.line_height;

	for (int i = 0; i < ctx->spritesheet_list.size; ++i)
	{
		Spritesheet *sheet = &ctx->spritesheet_list.items[i];

		item_area = (Rect2i) {{ area.x, area.y + i * line_height, area.width, line_height }};
		thumbnail_area = item_area;
		thumbnail_area.width = thumbnail_area.height;
		thumbnail_area = Rect2i_add_padding_all(thumbnail_area, thumbnail_pad);
		text_offset = (V2i) {{ thumbnail_area.x + thumbnail_area.width + text_pad, item_area.y + text_pad }};

		if (CheckCollisionPointReci(GetMousePositioni(), item_area)) {
			DrawRectangleReci(item_area, BLUE);

			if (BetterMouse_is_held(MOUSE_BUTTON_LEFT)) {
				show_preview = true;
				preview_texture = sheet->texture;
			}
		}

		ui_draw_text(ctx, strbuf_view2(sheet->path), text_offset, BLACK);

		DrawRectangleReci(Rect2i_add_padding_all(thumbnail_area, -1), BLACK);
		DrawRectangleReci(thumbnail_area, LIGHTGRAY);
		DrawTextureScaled(sheet->texture, thumbnail_area);
	}

	if (show_preview) {
		Rect2i preview_area = { .pos = GetMousePositioni(), .size = {{ preview_texture.width, preview_texture.height }} };
		DrawRectangleReci(Rect2i_add_padding_all(preview_area, -1), BLACK);
		DrawRectangleReci(preview_area, LIGHTGRAY);
		DrawTextureScaled(preview_texture, preview_area);
	}
}

void ui_draw_spritesheet(Ctx *ctx, Rect2i area) {
	DrawRectangleReci(area, LIGHTGRAY);

	if (ctx->spritesheet_list.size <= 0) { return; }

	// Spritesheet viewport ↓↓↓

	const Texture texture = ctx->spritesheet_list.items[0].texture;

	Rect2i draw_area = Rect2i_add_padding_all(area, 10);

	V2i texture_size = v2i(texture.width, texture.height);

	zoompanel_process(&ctx->zoompanel, texture_size, draw_area);

	V2i panned_origin = v2i_add(draw_area.pos, ctx->zoompanel.offset_from_origin);

	Rect2i final = (Rect2i){
		.pos = panned_origin,
		.size = v2f_2i(v2f_mul(v2i_2f(texture_size), v2ff(ctx->zoompanel.zoom)))
	};

	BeginTextureMode(ctx->draw.aux_viewport);

	ClearBackground(BLANK);
	DrawTextureScaled(texture, final);
	DrawRectangleLinesEx((Rect2i_to_Rect2(draw_area)).rect, 1, RED);

	EndTextureMode();

	DrawTextureRec_flipped(ctx->draw.aux_viewport.texture,
			draw_area, draw_area.pos, WHITE);

	// Spritesheet controls

	const int btn_width = 30;
	Rect2i btn_area = {{
		.x = draw_area.x + draw_area.width -btn_width, .y = draw_area.y,
		.width = btn_width, .height = btn_width
	}};
	V2i mouse = GetMousePositioni();

	for (int i = 0; i < SHEETEDITOR_CURSOR__COUNT; ++i) {
		SHEETEDITOR_CURSOR mode = (SHEETEDITOR_CURSOR)i;
		Color bg_color = ctx->editor_curr_cursor == mode ? BLUE : LIGHTGRAY;
		
		DrawRectangleReci(btn_area, BLACK);
		DrawRectangleReci(Rect2i_add_padding_all(btn_area, 1), bg_color);

		if (CheckCollisionPointReci(mouse, btn_area)) {
			if (BetterMouse_is_pressed(MOUSE_BUTTON_LEFT)) {
				ctx->editor_curr_cursor = mode;
			}
		}

		btn_area.y += btn_area.height;
	}

	
}

void ui_draw_all(Ctx *ctx) {

	// Collect draw functions.

    void (*functions[])(Ctx *ctx, Rect2i area) = {
		&ui_draw_options,
		&ui_draw_spritesheet_list,
		&ui_draw_spritesheet,
	};
	int function_amount = sizeof(functions)/sizeof(functions[0]);

	// Subdivide window horizontally.

	const int pad = 2;
	Rect2i chunk = {{ 0, 0, GetScreenWidth()/function_amount, GetScreenHeight() }};
	Rect2i area = chunk;
	Rect2i area_paded;

	for (int i = 0; i < function_amount; ++i) {
		area_paded = (Rect2i) {{ area.x +pad, area.y +pad, area.width -pad*2, area.height -pad*2 }};

		(*functions[i])(ctx, area_paded);

		area.x += area.width;
	}
}

#endif // !UI
