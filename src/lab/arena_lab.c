
#include "arena.h"
#include "strbuf.h"
#include "strbuf_extra.h"
#include "strview.h"
#include "strnum.h"
#include "arena_extra.h"

typedef struct {
    char name[50];
    long double health;
} Entity; // Align 16

typedef struct {
    char x[1];
} Ball; // Align 1

void test1_simple(Arena scratch, int entity_amount)
{
    ptrdiff_t og_beg = (ptrdiff_t)scratch.beg;

    for (int i = 0; i < entity_amount; ++i) {
        Ball *ball = arena_new(&scratch, Ball, 1);
        printfd("ball #%d from %ld to %ld", i, (ptrdiff_t)ball - og_beg, (ptrdiff_t)ball - og_beg + (ptrdiff_t)sizeof(Ball));

        Entity *e = arena_new(&scratch, Entity, 1);
        printfd("entity #%d from %ld to %ld", i, (ptrdiff_t)e - og_beg, (ptrdiff_t)e - og_beg + (ptrdiff_t)sizeof(Entity));
        strcpy(e->name, "Mimo");

    }

    // Check how much space was left.
    printfd("Arena beg: %ld, space left: %ld,", (ptrdiff_t)scratch.beg - og_beg, scratch.end - scratch.beg);
}

strbuf_t *arena_create_fixed_strbuf(Arena *perm, int capacity) {
    // @Note: See create_buf in strbuf.c
    int actual_size = (int)sizeof(strbuf_t) + capacity + 1;
    printfd("-> %d", actual_size);
    char *buff = arena_new_align(perm, char, void*, (ptrdiff_t)actual_size);
    wassert(buff != NULL);
    return strbuf_create_fixed(buff, (size_t)actual_size);
}


void test2_strings(Arena scratch) {
    ptrdiff_t og_beg = (ptrdiff_t)scratch.beg;

    /*sizeof(strbuf_t);*/

    strbuf_t *name = arena_create_fixed_strbuf(&scratch, 1000);
    strbuf_t *number = arena_create_fixed_strbuf(&scratch, 40);
    wassert(name != NULL);
    wassert(number != NULL);

    printf("(size %d cap %d).\n", name->size, name->capacity);
    strbuf_assign(&name, cstr_SL("HELLO THERE :)"));
    printf("(size %d cap %d) Name is %"PRIstr".\n", name->size, name->capacity, PRIstrargbuf(name));

    printf("(size %d cap %d).\n", number->size, number->capacity);
    strbuf_assign(&number, cstr_SL("HELLO THERE :)"));
    printf("(size %d cap %d) Name is %"PRIstr".\n", number->size, number->capacity, PRIstrargbuf(number));


    int mi = 10;
    strbuf_assign(&number, cstr_SL("((:::)"));
    printf("Number is %"PRIstr".\n", PRIstrargbuf(number));
    strbuf_printf(&number, ":::)");
    printf("Number is %"PRIstr".\n", PRIstrargbuf(number));
    strbuf_printf(&number, "(%d)", mi);
    printf("Number is %"PRIstr".\n", PRIstrargbuf(number));

    for (int i = 10; i < 20; ++i) {
        strbuf_append(&name, cstr_SL(", "));
        printf("Next %"PRIstr".\n", PRIstrargbuf(name));
    }


    printfd("Arena beg: %ld, space left: %ld,", (ptrdiff_t)scratch.beg - og_beg, scratch.end - scratch.beg);
}

int main(void) {
    printf("Hello there.\n");

    ArenaRoot arena_root = ArenaRoot_create((ptrdiff_t)1024*1024*1024*4);
    Arena arena = ArenaRoot_get_arena(arena_root);

    for (int i = 0; i < 10; ++i) {
        test1_simple(arena, i);
        printf("Completed i %d\n", i);
    }

    test2_strings(arena);

    ArenaRoot_free(&arena_root);

    return 0;
}
