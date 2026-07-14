#include "arena.h"
#include "strbuf.h"

static void* arena_strbuf_allocator(
    strbuf_allocator_t* this_allocator, void* ptr_to_free, size_t size
) {
    // New allocation: ptr_to_free == NULL && size > 0
    // Reallocation:   ptr_to_free != NULL && size > 0
    // Free:           ptr_to_free != NULL && size == 0
    // For this arena we don't free nor reallocate. We always allocate new space.

    (void)ptr_to_free;
    Arena *arena = (Arena*)this_allocator->app_data;
    return size > 0 ? arena_new_align(arena, char, ptrdiff_t, (ptrdiff_t)size) : NULL;
}

static strbuf_allocator_t make_arena_strbuf_allocator(Arena *arena) {
    return (strbuf_allocator_t) { .app_data = arena, .allocator = &arena_strbuf_allocator };
}
