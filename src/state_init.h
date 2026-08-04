#ifndef STATE_INIT_H
#define STATE_INIT_H

#include "state.h"
#include "arena_extra.h"

void Spritesheet_free(Spritesheet *sheet) {
    strbuf_destroy(&sheet->path);
    if (IsImageValid(sheet->image)) { UnloadImage(sheet->image); }
    if (IsTextureValid(sheet->texture)) { UnloadTexture(sheet->texture); }
    *sheet = (Spritesheet) {0};
}

int Spritesheet_make(strview_t file_path, Spritesheet *out_sheet) {
    Spritesheet sheet = { 0 };
    sheet.path = strbuf_create(file_path, NULL);
    sheet.image = LoadImage(sheet.path->cstr);
    if (!IsImageValid(sheet.image)) {
        printfd("WAR: Couldn't load image [%"PRIstr"].", PRIstrarg(file_path));
        goto abort;
    }

    if (sheet.image.format != PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        printfd("WAR: Image isn't R8G8B8A8, converting...");
        ImageFormat(&sheet.image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    }

    sheet.texture = LoadTextureFromImage(sheet.image);
    if (!IsTextureValid(sheet.texture)) {
        printfd("WAR: Couldn't load texture [%"PRIstr"].", PRIstrarg(file_path));
        goto abort;
    };

    if ((0)) {
    abort:
        Spritesheet_free(&sheet);
        return -1;
    }
    *out_sheet = sheet;
    return 0;
}


int _ctx_init(Ctx *ctx) {
    ctx->actions = Action_Dyna_create();
    ctx->spritesheet_list = VecVec_Spritesheet_create();
    ctx->curr_project_file_path = strbuf_create_empty(0, NULL);
    ctx->editor.selected_sprites_cursor = int_Dyna_create();
    ctx->editor.selected_sprites = int_Dyna_create();
    ctx->frame_arena.root = ArenaRoot_create(1024 * 1024 * 10); // 10 MB.
    ctx->frame_arena.arena = ArenaRoot_get_arena(ctx->frame_arena.root);
    ctx->frame_arena.strbuf_alloc = make_arena_strbuf_allocator(&ctx->frame_arena.arena);
    return 0;
}

void _ctx_free(Ctx *ctx) {

    // Free draw stuf.
    {
        UnloadFont(ctx->draw.font);
    }

    strbuf_destroy(&ctx->curr_project_file_path);
    for (int i = 0; i < ctx->actions.size; ++i) {
        strbuf_destroy(&ctx->actions.items[i].name);
    }
    Action_Dyna_free(&ctx->actions);

    {
        // Free spritesheet list.
        for (dyna_foreach(Vec_Spritesheet, kter, ctx->spritesheet_list)) {
            Vec_Spritesheet *spritesheet_frames = kter.ref;
            for (dyna_foreach(Spritesheet, iter, *spritesheet_frames)) {
                Spritesheet *s = iter.ref;
                Spritesheet_free(s);
            }
            Vec_Spritesheet_free(spritesheet_frames);
        }
        VecVec_Spritesheet_free(&ctx->spritesheet_list);
    }

    {
        // Free sprite list.
        for (int i = 0; i < ctx->sprites.size; ++i) {
            Sprite *sprite = &ctx->sprites.items[i];
            strbuf_destroy(&sprite->name);
        }
        Sprite_Dyna_free(&ctx->sprites);
    }

    int_Dyna_free(&ctx->editor.selected_sprites_cursor);
    int_Dyna_free(&ctx->editor.selected_sprites);
    ArenaRoot_free(&ctx->frame_arena.root);
}

#endif // !STATE_INIT_H
