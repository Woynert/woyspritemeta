#ifndef WOYGUITREE_H
#define WOYGUITREE_H

#include "arena_extra.h"
#include "raylib_extra.h"
#include "strview.h"

#ifndef DRAW_FUNC_ARG_TYPE
#define DRAW_FUNC_ARG_TYPE void
#endif


typedef enum {
    ContainerRoot, /* Just has one children */
    ContainerSplitH,
    ContainerSplitV,
    ContainerListH, /* Lists will render all children until one has a NULL draw_func. */
    ContainerListV,
    ContainerStack,
} ContainerType;

typedef struct wguitree_WidgetState {
    int scroll;
} wguitree_WidgetState;

#define STRMAP__TYPE wguitree_WidgetState
#define STRMAP__NAMESPACE wguitree_strmap_state
#include "../subprojects/woycontainer/src/strmap.h"

typedef struct Node Node;
//typedef struct Content Content;
//typedef struct Container Container;

typedef struct Content {
    int data;
    /* ↓↓↓ draw_func == NULL means this node is inactive. */
    void (*draw_func)(Ctx *ctx, const WidgetDraw widget, WidgetReq *req);
    //void (*draw_func)(DRAW_FUNC_ARG_TYPE);
} Content;

typedef struct DrawInfo {
    Content widget;
    Rect2i area;
    wguitree_WidgetState *state;
} DrawInfo;

//#define DYNA__TYPE DrawInfo
//#define DYNA__NAMESPACE wgtr_Vec_DrawInfo
//#include "da.h"

//#define DYNA__TYPE wgtr_Vec_DrawInfo
//#define DYNA__NAMESPACE wgtr_Vec_Vec_DrawInfo
//#include "da.h"

#define LIST__TYPE DrawInfo
#define LIST__NAMESPACE wgtr_List_DrawInfo
#include "list_simple.h"

#define DYNA__TYPE wgtr_List_DrawInfo
#define DYNA__NAMESPACE wgtr_Vec_List_DrawInfo
#include "da.h"

#define DYNA__TYPE Node
#define DYNA__NAMESPACE wguitree_node_da
#define DYNA__ONLY_HEADER
#include "da.h"

typedef struct Container {
    ContainerType type;
    strview_t identifier;
    wguitree_node_da children;
    //default_state; @Todo.
} Container;

typedef struct Node {
    bool is_container;
    union {
        Container container;
        Content content;
    };
} Node;

#define DYNA__TYPE Node
#define DYNA__NAMESPACE wguitree_node_da
#define DYNA__ONLY_IMP
#include "da.h"

typedef struct Wguitree {
    ArenaRoot arenaroot;
    Arena arena;

    /*
    Each state has an "int frame;" member. If we find this title on
    the current cycle we update it. Then, at the end of frame. If we find
    any state with outdate frames then we remove it.
    */
    // Maps a title to a state. We might add or delete from this every frame.
    wguitree_strmap_state title_to_state;

    int frame;

    Node root_node;
    Rect2i screen;

    // Only populated on "frame_end".
    // Layers -> Lists -> DrawInfo.
    //wgtr_Vec_DrawInfo out_draw_list;
    wgtr_List_DrawInfo out_draw_list;

} Wguitree;

/// @Returns error.
int wguitree_create(Wguitree *t) {
    *t = (Wguitree) { 0 };
    t->arenaroot = ArenaRoot_create(1 << 20);
    //t->draws_da = wguitree_drawinfo_da_create();
    return wguitree_strmap_state_create(&t->title_to_state);
}

void wguitree_free(Wguitree *t) {
    ArenaRoot_free(&t->arenaroot);
    wguitree_strmap_state_free(&t->title_to_state);
    //wguitree_drawinfo_da_free(&t->draws_da);
    *t = (Wguitree) { 0 };
}

void wguitree_build_start(Wguitree *t, Rect2i screen) {
    t->arena = ArenaRoot_get_arena(t->arenaroot);
    t->screen = screen;
}

void wguitree_print_tree(Node *node, int level) {
    for (int i = 0; i < level; ++i) {
        printf("   ");
    }
    printf("%s", node->is_container ? "Container" : "Widget");

    if (!node->is_container) {
        printf("\n");
        return;
    }

    printf(" type:%d", node->container.type);
    if (node->container.identifier.size > 0) {
        printf(" id:%"PRIstr, PRIstrarg(node->container.identifier));
    }
    printf("\n");

    for (int i = 0; i < node->container.children.size; ++i) {
        wguitree_print_tree(&node->container.children.items[i], level+1);
    }
}

Rect2i wguitree_calculate_area_for_child_n(ContainerType type, Rect2i source, int children_count, int i) {
    Rect2i area = source;
    switch (type) {
        case ContainerRoot: break;
        case ContainerListV:
        {
            //area.x = source.x;
            //area.width = source.width;
            area.height = (source.height / children_count);
            area.y = source.y + area.height * i;
            if (i == (children_count-1)) { // Fixes missing pixels.
                area.height = source.height - area.height * (children_count -1);
            }
        } break;
        case ContainerListH:
        {
            //area.y = source.y;
            //area.height = source.height;
            area.width = (source.width / children_count);
            area.x = source.x + area.width * i;
            if (i == (children_count-1)) { // Fixes missing pixels.
                area.width = source.width - area.width * (children_count -1);
            }
        } break;
        default: break;
    }
    return area;
}



/* @Todo: Find state that corresponds to this widget.
    @How? It is clear this node has no identifier. So, what
    should we do in this scenario? That is solved by asigning
    random identifiers if one is not supplied.

    But how can you identify the same node through cycles? If you
    do for example an ID that starts in zero and then upwards you might
    miss identify it. The only way it seems it's to provide identifiers
    manually. Same issue would happen if you make and ID based on the
    hierarchy example: "root>split_v>widget1" ← That wouldn't work.

    Questions to answer:
    * Q: What if I want to have a custom split_v and draw the children myself?
    * A: Maybe provide an API to register your own stuff???
    * Q: What if I want a split that can be manually adjusted?
    * A: Maybe some API like:

        register_container(container_draw);

        void container_draw(source area, int child_count, DrawInfo *children) {
        }

    * Q: Why the separation between Containers and Widgets?
    * A: Well the idea is to be able to build the entire tree declaratively and
    then at the end draw it. So there has to be two steps and for now I have
    hardcoded the available containers. But that's right that maybe a container
    is just another type of widget... But it shouldn't be responsible for
    drawing it's own children I think... So two steps are required, a
    build step, and a draw/process/logic step. Currently containers only have a
    build step but no draw step, and Widgets have (almost) no build step, but
    have a draw step. So a "true" node would require those two things.

    * Q: If I make a custom container, where can I set the children's positions?
    * A: I guess you would modify it during draw step but during build step it
    would be fixed to the last value you set.

    Plan: Unify Containers and Widgets into one. A Node can be marked as a
    container and if so it will receive a populated array of children's
    positions that it can modify every frame. It would be a reference that then
    will be used for drawing further nodes down the tree.

    So, the only job of a "Widget" now is to 1. Draw itself and 2. If it's a
    container then modify it's childrens positions accordingly on however it
    wants to. Example:

        void widget_v_split(Rect2i area, bool focused, void *user_ctx, int child_count, Rect2i **children) {
            DrawRectangle(area, GRAY);

            for (int i = 0; i < child_count; ++i) {
                Rec2i *child = &children[i];
                child->height = area.height / child_count;
                child->y = area.y + child->height * i;
            }
        }

        void widget_v_split_adjustable(Rect2i area, bool focused, void *user_ctx, int child_count, Rect2i **children) {
            DrawRectangle(area, GRAY);
            DrawText(area.pos, (user_ctx as Ctx)->title);

            if (mouse_relase(LEFT)) {
                node_state->is_draggin == false;
            }
            if (focused) {
                if (mouse_press(LEFT)) {
                    node_state->is_draggin == true;
                }
                if (node_state.is_draggin) {
                    node_state->percentage = clamp(0, (mouse.x - area.x) / area.width, 1);
                }
            }

            Rec2i *left = &children[0];
            Rec2i *right = &children[1];
            *left = (Rect2) { .width = node_state->percentage * area.width, ... };
            *right = (Rect2) { .width = (1 - node_state->percentage) * area.width, ... };
        }

    Note: The focused argument doesn't really mean what you think it does.
    I think that solves the issue about the custom containers.
    And the issue about determining the current widget which has "focus" is a
    matter of checking the mouse against the last tree. So no problem there.

    Final problem: Q: How to solve the ID problem? Should we enforce the user to
    always provide and ID? Or maybe there is some way to this automagically? In
    a way that is reliable? Or what about this: What about if there is no ID set
    then you will through node_state a reference to a zerod state and that state
    won't persist. The only way for it to persist is if you set and ID. If it
    notices that you did modify it then it prints a warning about: "Modified 
    state of Node without ID. Please provide an ID if you want state to persist".

    Problem: What about containers created through an iterator? They can't have
    compile time constant ids. Well I think you can create a custom id very
    easily like:

        char id[20];
        sprintf(id, "mimos%d", i);

    Yeah but it requires sprintf which is like super slow, also this feels like
    an overkill. You could just convert your numeric i into a thing.... I have
    an idea: Why don't we use a combination of both: An automatically
    set increasing id and the function pointer to identify them?. Because the
    only thing that differentiates Nodes is their function and their context.
    It could even be something like: Tree depth + Auto id + Function pointer.

    Here is a improvement over the counter id idea:

        for (int i = 0; i < 10; ++i) {
            Node con_hlist = {
                .is_container = true,
                .container.type = ContainerListH,
                .container.identifier = make_id("MIMO", i),
            };
        }

    make_id would be a macro/function that just append the i into the prefix.
    Problem solved right?

    Furthermore the node_state could actually be an arena from where you can
    get your state from. Consider:

        void draw_split_v_container(...) {
            float percentage = node_state_get_float(node_state);
            bool is_clicked = node_state_get_bool(node_state);
        }

    Problem: Consume inputs front-to-back when the tree is drawn back-to-front.

    There is a solution to this but the problem is that it would introduce a
    delay of one frame and I'm not sure if I want that.
*/

typedef struct {
    Node *node;
    int child_idx;
    Rect2i area;
    int depth;
} StackItem;
#define DYNA__TYPE StackItem
#define DYNA__NAMESPACE wguitree_stack_da
#include "da.h"

void wguitree_build_end(Wguitree *t) {
    ++t->frame;

    // For now let's just print the tree.
    printfd("PRINTING TREE:");
    wguitree_print_tree(&t->root_node, 0);

    //t->draws_da = wguitree_drawinfo_da_create_with_allocator(arena_allocator, &t->arena);
    //t->layers = wgtr_Vec_Vec_DrawInfo_create_with_allocator(arena_allocator, &t->arena);

    wgtr_Vec_List_DrawInfo layers = wgtr_Vec_List_DrawInfo_create_with_allocator(arena_allocator, &t->arena);

    // Let's say I wanna access id 0

    // Ok. Now let's calculate the sizes of each.
    printfd("Starting size calculation.");
    {
        // Let's maybe only save containers in our stack.

        wguitree_stack_da stack = wguitree_stack_da_create_with_allocator(arena_allocator, &t->arena);
        StackItem *curr_item = NULL;

        StackItem new_item = { // First item.
            .node = &t->root_node,
            .child_idx = 0,
            .area = t->screen,
            .depth = 0,
        };
        goto NEW_ITEM;

        NEW_ITEM:
        {
            //printfd("[Container type %d area "Rect2i_Fmt"]", new_item.node->container.type, Rect2i_Arg(new_item.area));
            wguitree_stack_da_append(&stack, new_item);
        }

        CONTINUE:
        {
            curr_item = wguitree_stack_da_get_safe(&stack, stack.size-1);
            Container *self = &curr_item->node->container;
            for (; curr_item->child_idx < self->children.size; ++curr_item->child_idx) {
                printfd(ANSI_MAG"[Container type %d area "Rect2i_Fmt"] child %d/%d", self->type, Rect2i_Arg(curr_item->area), curr_item->child_idx+1, self->children.size);
                Node *node = &self->children.items[curr_item->child_idx];
                Rect2i area = wguitree_calculate_area_for_child_n(self->type, curr_item->area, self->children.size, curr_item->child_idx);
                if (!node->is_container) {

                    // Add to collection of DrawInfos.

                    int depth = curr_item->depth;
                    while(depth >= layers.size) {
                        wgtr_List_DrawInfo new_layer = wgtr_List_DrawInfo_create_with_allocator(arena_allocator, &t->arena);
                        wgtr_Vec_List_DrawInfo_append(&layers, new_layer);
                    }
                    wgtr_List_DrawInfo *draw_layer = &layers.items[depth];

                    printfd("(i %d) Widget right here! :) "Rect2i_Fmt, curr_item->child_idx, Rect2i_Arg(area));

                    wgtr_List_DrawInfo_append(draw_layer,
                    (DrawInfo) {
                        .area = area,
                        .widget = node->content
                    });
                } else {
                    ++curr_item->child_idx;
                    new_item = (StackItem) {
                        .node = node,
                        .child_idx = 0,
                        .area = area,
                        .depth = curr_item->depth +1,
                    };
                    goto NEW_ITEM;
                }
            }
            // Finished with the children. Let's pop and go up.
            wguitree_stack_da_remove_at(&stack, stack.size-1);
            if (stack.size > 0) {
                goto CONTINUE;
            }
        }
    }
    printfd("TREE HAS BEEN UHH PROCESSED");
    printfd("Printing layered things");

    // Populate final list.

    t->out_draw_list = wgtr_List_DrawInfo_create_with_allocator(arena_allocator, &t->arena);

    for (int i = layers.size -1; i > -1; i += -1) {
        wgtr_List_DrawInfo_It it = { 0 };
        while(wgtr_List_DrawInfo_it_next(&layers.items[i], &it)) {
            DrawInfo draw = *it.item;
            wgtr_List_DrawInfo_append(&t->out_draw_list, draw);
            printfd("(depth %d) Widget right here! :) "Rect2i_Fmt, i, Rect2i_Arg(draw.area));
        }
    }

    printfd("This is the end");
}

/*
v2f
v2i
v3f
v3i
*/

/*
void wguitree_build_end(Wguitree *t) {
    ++t->frame;

    // For now let's just print the tree.
    printfd("PRINTING TREE:");
    wguitree_print_tree(&t->root_node, 0);

    //t->draws_da = wguitree_drawinfo_da_create_with_allocator(arena_allocator, &t->arena);
    t->layers = wgtr_Vec_Vec_DrawInfo_create_with_allocator(arena_allocator, &t->arena);

    // Let's say I wanna access id 0

    // Ok. Now let's calculate the sizes of each.
    printfd("Starting size calculation.");
    {
        // Let's maybe only save containers in our stack.

        wguitree_stack_da stack = wguitree_stack_da_create_with_allocator(arena_allocator, &t->arena);
        StackItem *curr_item = NULL;

        StackItem new_item = { // First item.
            .node = &t->root_node,
            .child_idx = 0,
            .area = t->screen,
        };
        goto NEW_ITEM;

        NEW_ITEM:
        {
            //printfd("[Container type %d area "Rect2i_Fmt"]", new_item.node->container.type, Rect2i_Arg(new_item.area));
            wguitree_stack_da_append(&stack, new_item);
        }

        CONTINUE:
        {
            curr_item = wguitree_stack_da_get_safe(&stack, stack.size-1);
            Container *self = &curr_item->node->container;
            for (; curr_item->child_idx < self->children.size; ++curr_item->child_idx) {
                printfd(ANSI_MAG"[Container type %d area "Rect2i_Fmt"] child %d/%d", self->type, Rect2i_Arg(curr_item->area), curr_item->child_idx+1, self->children.size);
                Node *node = &self->children.items[curr_item->child_idx];
                Rect2i area = wguitree_calculate_area_for_child_n(self->type, curr_item->area, self->children.size, curr_item->child_idx);
                if (!node->is_container) {
                    printfd("(i %d) Widget right here! :) "Rect2i_Fmt, curr_item->child_idx, Rect2i_Arg(area));

                    wguitree_drawinfo_da_append(&t->draws_da,
                    (DrawInfo) {
                        .area = area,
                        .widget = node->content
                    });
                } else {
                    ++curr_item->child_idx;
                    new_item = (StackItem) {
                        .node = node,
                        .child_idx = 0,
                        .area = area,
                    };
                    goto NEW_ITEM;
                }
            }
            // Finished with the children. Let's pop and go up.
            wguitree_stack_da_remove_at(&stack, stack.size-1);
            if (stack.size > 0) {
                goto CONTINUE;
            }
        }
    }
    printfd("It's the end!");
}
   */

void wguitree_container_add_child(Wguitree *t, Node *parent_node, Node child_node) {
    if (!parent_node->is_container) {
        printfd("WAR: Refusing to add child to non-container node.");
        return;
    }
    Container *parent = &parent_node->container;

    if (parent->children.items == NULL) {
        parent->children = wguitree_node_da_create_with_allocator(arena_allocator, &t->arena);
    }

    wguitree_node_da_append(&parent->children, child_node);
}

#endif
