#ifndef UI
#define UI

#include "operations.h"
#include "raylib_extra.h"
#include "raylib.h"
#include "la.h"
#include "better_mouse_input.h"

void ui_draw_options(Ctx *ctx, Rect2i area) {

	const int pad = 10;
	const int line_height = ctx->draw.line_height;
	int line = 0;
	V2i pos = { 0 };
	pos = (V2i) {{ area.x + pad, area.y + line * line_height }};
	++line;

	Rect2i line_area = {{ pos.x, pos.y, area.width -pad, line_height }};
	line_area.y += line_height;

	DrawRectangleReci(area, GRAY);
	DrawText("Options:", pos.x, pos.y, ctx->draw.font_size, BLACK);

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

		DrawText(
			TextFormat("%"PRIstr, PRIstrarg(strbuf_view2(action->name))),
			pos.x, pos.y, ctx->draw.font_size, BLACK);

		++line;
		line_area.y += line_height;
	}
}

void ui_draw_spritesheet_list(Ctx *ctx, Rect2i area) {

	const int pad = 10;
	const int line_height = ctx->draw.line_height;
	int line = 0;
	V2i pos = { 0 };
	pos = (V2i) {{ area.x + pad, area.y + line * line_height }};
	++line;

	Rect2i line_area = {{ pos.x, pos.y, area.width -pad, line_height }};
	line_area.y += line_height;

	DrawRectangleReci(area, GRAY);
	DrawText("Spritesheets:", pos.x, pos.y, ctx->draw.font_size, BLACK);

	for (int i = 0; i < ctx->spritesheet_list.size; ++i, ++line, line_area.y += line_height)
	{
		Spritesheet *sheet = &ctx->spritesheet_list.items[i];
		pos = (V2i) {{ (int)area.x + pad, (int)area.y + line * ctx->draw.line_height }};

		if (CheckCollisionPointReci(GetMousePositioni(), line_area)) {
			DrawRectangleReci(line_area, BLUE);

			//if (BetterMouse_is_pressed(MOUSE_BUTTON_LEFT)) {
				//call_action(ctx, action);
			//}
		}

		DrawText(
			TextFormat("%"PRIstr, PRIstrarg(strbuf_view2(sheet->path))),
			pos.x, pos.y, ctx->draw.font_size, BLACK);
	}
}

void ui_draw_all(Ctx *ctx) {

	// Collect draw functions.

    void (*functions[])(Ctx *ctx, Rect2i area) = { &ui_draw_options, &ui_draw_spritesheet_list };
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
