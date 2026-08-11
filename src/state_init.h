#ifndef STATE_INIT_H
#define STATE_INIT_H

#include "state.h"
#include "arena_extra.h"
#include "raylib_extra.h"


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


Sprite sprite_make(void) {
    return (Sprite) {
        .name = strbuf_create_empty(0, NULL),
        .frames = 1,
    };
}

void sprite_free(Sprite *sprite) {
    strbuf_destroy(&sprite->name);
}


//void vec_sprites_clear_sprites(Vec_Sprite *sprites) {
    //for (dyna_foreach(Sprite, iter, *sprites)) {
        //sprite_free(iter.ref);
    //}
    //Vec_Sprite_clear_preserving(sprites);
//}

void Project_init(Project *p) {
    p->path_absolute = strbuf_create(0, NULL);
    p->spritesheet_list = Vec_Spritesheet_create();
}

Project *Project_make(void) {
    Project *p = (Project*)malloc(sizeof(Project));
    Project_init(p);
    return p;
}

void Project_clear(Project *p) {
    strbuf_assign(&p->path_absolute, cstr_SL(""));
    for (dyna_foreach(Spritesheet, kter, p->spritesheet_list)) {
        Spritesheet_free(kter.ref);
    }
    Vec_Spritesheet_clear_freeing(&p->spritesheet_list);
}

void Project_free(Project *p) {
    if (!p) { return; }
    strbuf_destroy(&p->path_absolute);
    for (dyna_foreach(Spritesheet, kter, p->spritesheet_list)) {
        Spritesheet_free(kter.ref);
    }
    Vec_Spritesheet_free(&p->spritesheet_list);
    free(p);
}

void ctx_clear_curr_project(Ctx *ctx) {
    if (ctx->p != NULL) {
        Project_clear(ctx->p);
    }

    ZERO(ctx->project_loaded);
    ZERO(ctx->has_unsaved_changes);
    ZERO(ctx->curr_frame_id);
    ZERO(ctx->curr_sheet_id);
    ZERO(ctx->curr_sheet_size);
    ctx->mouse_selected_spritesheet_id = -1;

    zoompanel_reset_zoom_and_pan(&ctx->editor.zoompanel);
}


int ctx_init(Ctx *ctx) {
    *ctx = (Ctx) { 0 };
    ctx->frame_arena_root = ArenaRoot_create(1024 * 1024 * 10); // 10 MB.
    ctx->frame_arena      = ArenaRoot_get_arena(ctx->frame_arena_root);
    ctx->menu.actions                   = Vec_Action_create();
    ctx->editor.selected_sprites_cursor = int_Dyna_create();
    ctx->editor.selected_sprites        = int_Dyna_create();
    zoompanel_init(&ctx->editor.zoompanel, ZOOMPANEL_CONF_PIXEL_PERFECT, MouseRight);
    ctx->p = Project_make();
    ctx_clear_curr_project(ctx);
    return 0;
}

void ctx_free(Ctx *ctx) {
    {
        // Free ctx.draw
        UnloadFont(ctx->draw.font);
        UnloadTexture(ctx->draw.splash_art);
    }
    ArenaRoot_free(&ctx->frame_arena_root);
    Project_free(ctx->p);
    for (int i = 0; i < ctx->menu.actions.size; ++i) {
        strbuf_destroy(&ctx->menu.actions.items[i].name);
    }
    Vec_Action_free(&ctx->menu.actions);
    int_Dyna_free(&ctx->editor.selected_sprites_cursor);
    int_Dyna_free(&ctx->editor.selected_sprites);
}

void ctx_load_assets(Ctx *ctx) {

    const int codepoint_ranges[] = { // Ranges are inclusive
        0xFFFD,  0xFFFD,  // (�) codepoint
        32,      127,     // Basic latin
        0x00A1,  0x00FF,  // C1 Controls and Latin-1 Supplement
        0x0100,  0x017F,  // Latin Extended-A
        0x0180,  0x024F,  // Latin Extended-B
        0x1F300, 0x1F5FF, // Miscellaneous Symbols and Pictographs
        0x1F600, 0x1F64F, // Emoticons
    };

    // Draw
    ctx->draw.font_size = 18;
    ctx->draw.line_spacing = 0;
    ctx->draw.char_spacing = 0;
    ctx->draw.line_height = ctx->draw.font_size +ctx->draw.line_spacing +2;
    ctx->draw.font = load_font_with_buncha_codepoints(
            "assets/Roboto-Regular.ttf", ctx->draw.font_size,
            (int*)codepoint_ranges, countofi(codepoint_ranges));
    ctx->draw.splash_art = LoadTexture("assets/splash_art.png");
    wassert(IsTextureValid(ctx->draw.splash_art));
}

#endif // !STATE_INIT_H
