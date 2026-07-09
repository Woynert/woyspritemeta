#ifndef UI
#define UI

#include "operations.h"
#include "raylib_extra.h"
#include "raylib.h"
#include "la.h"
#include "better_mouse_input.h"

void ui_draw_options(Ctx *ctx, Rect2 area) {

	const int line_height = ctx->draw.line_height;
	int line = 0;
	V2i pos = { 0 };
	pos = (V2i) {{ (int)area.x + 10, (int)area.y + line * line_height }};
	++line;

	Rect2 line_area = {{ (float)pos.x, (float)pos.y, area.width, (float)line_height }};
	line_area.y += (float)line_height;

	DrawRectangleRec(area.rect, GRAY);
	DrawText("Options:", pos.x, pos.y, ctx->draw.font_size, BLACK);

	for (int i = 0; i < ctx->actions.size; ++i, ++line, line_area.y += (float)line_height)
	{
		Action *action = &ctx->actions.items[i];
		pos = (V2i) {{ (int)area.x + 10, (int)area.y + line * ctx->draw.line_height }};

		if (CheckCollisionPointRec(GetMousePosition(), line_area.rect)) {
			DrawRectangleRec(line_area.rect, BLUE);

			if (BetterMouse_is_pressed(MOUSE_BUTTON_LEFT)) {
				call_action(ctx, action);
			}
		}

		DrawText(
			TextFormat("%"PRIstr, PRIstrarg(strbuf_view2(action->name))),
			pos.x, pos.y, ctx->draw.font_size, BLACK);
	}
}

void ui_draw_all(Ctx *ctx) {
	Rect2 area = { 0 };
	area = (Rect2) {{ 10, 10, 200, 200 }};
	ui_draw_options(ctx, area);
}

#endif // !UI
