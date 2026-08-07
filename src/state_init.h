#ifndef STATE_INIT_H
#define STATE_INIT_H

#include "state.h"
#include "arena_extra.h"

void SpritesheetFrame_free(SpritesheetFrame *frame) {
    strbuf_destroy(&frame->path);
    if (IsImageValid(frame->image)) { UnloadImage(frame->image); }
    if (IsTextureValid(frame->texture)) { UnloadTexture(frame->texture); }
    *frame = (SpritesheetFrame) {0};
}

int SpritesheetFrame_make(strview_t file_path, SpritesheetFrame *out_frame) {
    SpritesheetFrame frame = { 0 };
    frame.path = strbuf_create(file_path, NULL);
    frame.image = LoadImage(frame.path->cstr);
    if (!IsImageValid(frame.image)) {
        printfd("WAR: Couldn't load image [%"PRIstr"].", PRIstrarg(file_path));
        goto abort;
    }

    if (frame.image.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        printfd("WAR: Image isn't R8G8B8A8, converting...");
        ImageFormat(&frame.image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    }

    frame.texture = LoadTextureFromImage(frame.image);
    if (!IsTextureValid(frame.texture)) {
        printfd("WAR: Couldn't load texture [%"PRIstr"].", PRIstrarg(file_path));
        goto abort;
    };

    if ((0)) {
    abort:
        SpritesheetFrame_free(&frame);
        return -1;
    }
    *out_frame = frame;
    return 0;
}


Spritesheet Spritesheet_make(void) {
    Spritesheet sheet = { 0 };
    sheet.frames = Vec_SpritesheetFrame_create();
    sheet.sprites = Vec_Sprite_create();
    return sheet;
}

void Spritesheet_clear_frames(Spritesheet *sheet) {
    for (dyna_foreach(SpritesheetFrame, iter, sheet->frames)) {
        SpritesheetFrame *s = iter.ref;
        SpritesheetFrame_free(s);
    }
    Vec_SpritesheetFrame_clear_freeing(&sheet->frames);
}

void Spritesheet_free(Spritesheet *sheet) {
    for (dyna_foreach(SpritesheetFrame, iter, sheet->frames)) {
        SpritesheetFrame *s = iter.ref;
        SpritesheetFrame_free(s);
    }
    Vec_SpritesheetFrame_free(&sheet->frames);

    for (dyna_foreach(Sprite, iter, sheet->sprites)) {
        Sprite *s = iter.ref;
        strbuf_destroy(&s->name);
    }
    Vec_Sprite_free(&sheet->sprites);

    *sheet = (Spritesheet) {0};
}


int _ctx_init(Ctx *ctx) {
    ctx->mouse_selected_spritesheet_id = -1;
    ctx->menu.actions = Vec_Action_create();
    ctx->spritesheet_list = Vec_Spritesheet_create();
    ctx->project.path = strbuf_create_empty(0, NULL);
    ctx->editor.selected_sprites_cursor = int_Dyna_create();
    ctx->editor.selected_sprites = int_Dyna_create();
    ctx->frame_arena_root = ArenaRoot_create(1024 * 1024 * 10); // 10 MB.
    ctx->frame_arena = ArenaRoot_get_arena(ctx->frame_arena_root);
    return 0;
}

void _ctx_free(Ctx *ctx) {

    // Free draw stuf.
    {
        UnloadFont(ctx->draw.font);
    }

    strbuf_destroy(&ctx->project.path);

    for (int i = 0; i < ctx->menu.actions.size; ++i) {
        strbuf_destroy(&ctx->menu.actions.items[i].name);
    }
    Vec_Action_free(&ctx->menu.actions);

    {
        // Free spritesheet list.
        for (dyna_foreach(Spritesheet, kter, ctx->spritesheet_list)) {
            Spritesheet *sheet = kter.ref;
            Spritesheet_free(sheet);
        }
        Vec_Spritesheet_free(&ctx->spritesheet_list);
    }

    int_Dyna_free(&ctx->editor.selected_sprites_cursor);
    int_Dyna_free(&ctx->editor.selected_sprites);
    ArenaRoot_free(&ctx->frame_arena_root);
}

#endif // !STATE_INIT_H
