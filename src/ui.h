#ifndef UI
#define UI

#include "operations.h"
#include "raylib_extra.h"
#include "la_extra.h"
#include "raylib.h"
#include "la.h"
#include "better_mouse_input.h"
#include "rlgl.h"

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

void ui_draw_sprite_list(Ctx *ctx, Rect2i area) {

	static strbuf_t *aux_str = NULL;
	static bool aux_str_setup = false;
	if (!aux_str_setup) { aux_str_setup = true; aux_str = strbuf_create(0, NULL); }

	const int line_height = ctx->draw.line_height * 2;
	const int text_pad = 3;
	const int thumbnail_pad = 3;

	Rect2i item_area;
	Rect2i thumbnail_area;
	V2i text_offset;

	//bool show_preview = false;
	//Texture preview_texture = { 0 };

	DrawRectangleReci(area, LIGHTGRAY);
	ui_draw_text(ctx, cstr_SL("Sprites:"), area.pos, BLACK);
	area.y += ctx->draw.line_height;

	for (int i = 0; i < ctx->sprites.size; ++i)
	{
		Sprite *sprite = &ctx->sprites.items[i];

		item_area = (Rect2i) {{ area.x, area.y + i * line_height, area.width, line_height }};
		thumbnail_area = item_area;
		thumbnail_area.width = thumbnail_area.height;
		thumbnail_area = Rect2i_add_padding_all(thumbnail_area, thumbnail_pad);
		text_offset = (V2i) {{ thumbnail_area.x + thumbnail_area.width + text_pad, item_area.y + text_pad }};

		if (CheckCollisionPointReci(GetMousePositioni(), item_area)) {
			DrawRectangleReci(item_area, BLUE);

			//if (BetterMouse_is_held(MOUSE_BUTTON_LEFT)) {
				//show_preview = true;
				//preview_texture = sheet->texture;
			//}
		}

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
		ui_draw_text(ctx, strbuf_view2(aux_str), text_offset, BLACK);

		//DrawRectangleReci(Rect2i_add_padding_all(thumbnail_area, -1), BLACK);
		//DrawRectangleReci(thumbnail_area, LIGHTGRAY);
		//DrawTextureScaled(sheet->texture, thumbnail_area);
	}

	//if (show_preview) {
		//Rect2i preview_area = { .pos = GetMousePositioni(), .size = {{ preview_texture.width, preview_texture.height }} };
		//DrawRectangleReci(Rect2i_add_padding_all(preview_area, -1), BLACK);
		//DrawRectangleReci(preview_area, LIGHTGRAY);
		//DrawTextureScaled(preview_texture, preview_area);
	//}
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

void ui_draw_spritesheet(Ctx *ctx, Rect2i area) {

	static strbuf_t *aux_str = NULL;
	static bool aux_str_setup = false;
	if (!aux_str_setup) { aux_str_setup = true; aux_str = strbuf_create(0, NULL); }

	V2i mouse = GetMousePositioni();

	DrawRectangleReci(area, LIGHTGRAY);

	if (ctx->spritesheet_list.size <= 0) { return; }

	// Spritesheet viewport ↓↓↓

	const Texture texture = ctx->spritesheet_list.items[0].texture;

	Rect2i draw_area = Rect2i_add_padding_all(area, 10);
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
	V2i mouse_pos_in_image = v2f_2i(v2f_mul(
		v2f_div(v2i_2f(mouse_from_image_origin), v2i_2f(scaled_size)),
		v2i_2f(texture_size)));

	// Draw on screen.

	BeginTextureMode(ctx->draw.aux_viewport);
	ClearBackground(BLANK);
	DrawTextureScaled(texture, final);
	DrawRectangleLinesEx((Rect2i_to_Rect2(draw_area)).rect, 1, RED);


	{
		// Draw current sprites.
		for (dyna_foreach(Sprite, i, ctx->sprites)) {
			Sprite *sprite = i.ref;
			ui__spritesheet_draw_scaled_rect_lines2(sprite->rect, panned_origin, scale, YELLOW, 1.5);
		}

		// Draw selected sprites.
		for (dyna_foreach(int, i, ctx->editor.selected_sprites_cursor)) {
			int sprite_id = *i.ref;
			Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, sprite_id);
			if (sprite == NULL) { continue; }
			ui__spritesheet_draw_scaled_rect_lines2(sprite->rect, panned_origin, scale, BLUE, 1.5);
		}
		for (dyna_foreach(int, i, ctx->editor.selected_sprites)) {
			int sprite_id = *i.ref;
			Sprite *sprite = Sprite_Dyna_get_safe(&ctx->sprites, sprite_id);
			if (sprite == NULL) { continue; }
			ui__spritesheet_draw_scaled_rect_lines2(sprite->rect, panned_origin, scale, BLUE, 1.5);
		}
	}



	// Selection

	{
		bool mouse_inside = CheckCollisionPointReci(mouse, draw_area);

		switch(ctx->editor.cursor) {
			case SHEETEDITOR_CURSOR_TWEAK:
			case SHEETEDITOR_CURSOR_ADD:
			{
				if (!ctx->editor.is_selecting &&
					BetterMouse_is_pressed(MOUSE_BUTTON_LEFT) &&
					mouse_inside
				) {
					ctx->editor.is_selecting = true;
					ctx->editor.selection_origin = mouse_pos_in_image;

					if (ctx->editor.cursor == SHEETEDITOR_CURSOR_TWEAK) {
						bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
						if (!shift) { spritesheet_clear_selection(ctx); }
					}
				}
				if (ctx->editor.is_selecting &&
					BetterMouse_is_released(MOUSE_BUTTON_LEFT)
				) {
					ctx->editor.is_selecting = false;

					// Register sprite.

					Rect2i selection = Rect2i_from_two_positions(
							ctx->editor.selection_origin, mouse_pos_in_image);
					selection.size = v2i_add(selection.size, v2ii(1));

					if (ctx->editor.cursor == SHEETEDITOR_CURSOR_ADD) {
						if (selection.width > 1 && selection.height > 1) {
							register_sprite(ctx, selection);
						}
					}
					else if (ctx->editor.cursor == SHEETEDITOR_CURSOR_TWEAK) {
						spritesheet_commit_selection(ctx);
					}

				}
				if (ctx->editor.is_selecting) {

					Rect2i selection = Rect2i_from_two_positions(
							ctx->editor.selection_origin, mouse_pos_in_image);
					selection.size = v2i_add(selection.size, v2ii(1));

					ui__spritesheet_draw_scaled_rect_lines(
						selection,
						panned_origin, scale, GREEN, 1);
					
					strview_t text = strbuf_printf(&aux_str, "%d x %d", selection.size.x, selection.size.y);
					ui_draw_text_highlighted(
						ctx, text,
						v2f_2i(v2f_translate_scale(v2i_add(selection.pos, v2ii(1)), panned_origin, scale)),
						WHITE, BLACK);

					if (ctx->editor.cursor == SHEETEDITOR_CURSOR_TWEAK) {
						spritesheet_select_sprites(ctx, selection);
					}
				}
				break;
			}
			case SHEETEDITOR_CURSOR_DELETE:
			{ break; }
			default: { break; }

		}
	}

	// Draw cursor position scaled.

	ui__spritesheet_draw_scaled_rect((Rect2i){.pos = mouse_pos_in_image, .size = v2ii(1)}, panned_origin, scale, GREEN);

	EndTextureMode();

	DrawTextureRec_flipped(ctx->draw.aux_viewport.texture,
			draw_area, draw_area.pos, WHITE);

	// Spritesheet controls

	const int btn_width = 30;
	Rect2i btn_area = {{
		.x = draw_area.x + draw_area.width -btn_width, .y = draw_area.y,
		.width = btn_width, .height = btn_width
	}};

	for (int i = 0; i < SHEETEDITOR_CURSOR__COUNT; ++i) {
		SHEETEDITOR_CURSOR mode = (SHEETEDITOR_CURSOR)i;
		Color bg_color = ctx->editor.cursor == mode ? BLUE : LIGHTGRAY;
		
		DrawRectangleReci(btn_area, BLACK);
		DrawRectangleReci(Rect2i_add_padding_all(btn_area, 1), bg_color);

		if (CheckCollisionPointReci(mouse, btn_area)) {
			if (BetterMouse_is_pressed(MOUSE_BUTTON_LEFT)) {
				ctx->editor.cursor = mode;
			}
		}

		btn_area.y += btn_area.height;
	}

	if (BetterMouse_is_held(MOUSE_BUTTON_LEFT)) {
		printfd("Selected pixel is: "V2i_Fmt"", V2i_Arg(mouse_pos_in_image));
	}
}

void ui_draw_all(Ctx *ctx) {

	// Collect draw functions.

    void (*functions[])(Ctx *ctx, Rect2i area) = {
		&ui_draw_options,
		&ui_draw_spritesheet_list,
		&ui_draw_sprite_list,
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

	//Texture t
}

#endif // !UI
