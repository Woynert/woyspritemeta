#include "arena.h"
#include "strbuf.h"

static void* arena_strbuf_allocator(
    strbuf_allocator_t* this_allocator, void* ptr, size_t size
) {
    // New allocation: ptr == NULL && size > 0
    // Reallocation:   ptr != NULL && size > 0
    // Free:           ptr != NULL && size == 0

    if (size == 0) return NULL; // No freeing for arena.

    Arena *arena = (Arena*)this_allocator->app_data;
    void *result = arena_new_align(arena, char, ptrdiff_t, (ptrdiff_t)size);

    if (ptr != NULL) { // Reallocation must copy what we had.
        // Reallocation must copy what we had.
        // Don't use memcpy.
        memmove(result, ptr, size);
        /*
           TODO: Check if there would be overlap in this operation and
           print the delta. If there's overlap that would indicate there's
           a bug with the arena_alloc I think.
        */
    }

    return result;
}

static strbuf_allocator_t make_arena_strbuf_allocator(Arena *arena) {
    return (strbuf_allocator_t) { .app_data = arena, .allocator = &arena_strbuf_allocator };
}

static void* arena_allocator(void* user_data, void* ptr, size_t size, int align) {
    // New allocation: ptr == NULL && size > 0
    // Reallocation:   ptr != NULL && size > 0
    // Free:           ptr != NULL && size == 0

    if (size == 0) return NULL; // No freeing for arena.

    Arena *arena = (Arena*)user_data;
    void *result = arena_alloc(arena, sizeof(char), align, (i64)size);

    if (ptr != NULL) {
        // Reallocation must copy what we had.
        // Don't use memcpy.
        memmove(result, ptr, size);
    }

    return result;
}
