// TODO: Rename to arena_strbuf_extra.h
#ifndef ARENA_EXTRA_H
#define ARENA_EXTRA_H

#include "arena.h"
#include "portable_utils.h"
#include "wstrview.h"
#include "strbuf.h"


strbuf_t strbuf_create_with_arena(Arena *perm) {
    return strbuf_create_with_allocator(arena_allocator, perm);
}

strbuf_t *strbuf_create_with_arena_init(strview_t init_content, Arena *perm) {
    strbuf_t *buf = arena_new(perm, strbuf_t, 1);
    if (!buf) { return NULL; }
    *buf = strbuf_create_with_arena(perm);
    int err = strbuf_assign(buf, init_content);
    if (err) { return NULL; }
    return buf;
}

#define strview_to_buf strbuf_create_with_arena_init

// Quick string format.
__attribute__((format(printf, 2, 3)))
strview_t SF(Arena *perm, const char *fmt, ...) {
    strbuf_t buf = strbuf_create_with_arena(perm);
    va_list va;
    va_start(va, fmt);
    int err = strbuf_append_vprintf(&buf, fmt, va);
    if (err) { return STRVIEW_INVALID; }
    va_end(va);
    return strbuf_view(buf);
}

// Quick string concatenation.
#define SC(perm, ...) _SC(perm, STRBUF__PP_NARG(__VA_ARGS__), (strview_t[]){__VA_ARGS__})
strview_t _SC(Arena *perm, int view_count, strview_t *views) {
    strbuf_t buf = strbuf_create_with_arena(perm);
    int err = strbuf__cat(perm, &buf, view_count, views);
    if (err) { return STRVIEW_INVALID; }
    return strbuf_view(buf);
}


//strbuf_t* strbuf_create_with_arena_empty(size_t initial_capacity, Arena *arena) {
    //if (arena == NULL) { return NULL; }
    //if (initial_capacity > INT_MAX) { return NULL; }
    //strbuf_allocator_t allocator = { .app_data = arena, .allocator = arena_strbuf_allocator };
    //strbuf_t *buf = (strbuf_t*)allocator.allocator(&allocator, NULL, sizeof(strbuf_t)+initial_capacity+1);
    //buf->capacity = (int)initial_capacity;
    //buf->allocator = allocator;
    //buf->size = 0;
    //buf->cstr[0] = 0;
    //return buf;
//}

//strbuf_t *strbuf_create_with_arena_init(strview_t initial_content, Arena *arena) {
    //strbuf_t buf = strbuf_create_with_arena(arena);
    //if (arena == NULL) { return NULL; }
    //if (initial_content.size < 0) { return NULL; }
    ////strbuf_t *buf = strbuf_create_with_arena_empty((size_t)initial_content.size, arena);
    //strbuf_assign(&buf, initial_content);
    //return buf;
//}

#endif // !ARENA_EXTRA_H

