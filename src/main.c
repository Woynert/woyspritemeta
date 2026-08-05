#include "stdio.h"
#include "portable_utils.h"
#include "raylib.h"
#include "GLFW/glfw3.h"
#include "winput.h"
#include "state.h"
#include "ui.h"
#include "quick_monitor.h"
#include "raylib_drawbuffer.h"

#define LA_IMPLEMENTATION
#include "la.h"

void glfw_mouse_callback(GLFWwindow* w, int button, int action, int mods) {
    winput_glfw_mouse_button_callback(w, button, action, mods);
    uimouseinput_glfw_mouse_button_callback(w, button, action, mods);
}

void glfw_scroll_callback(GLFWwindow* w, double xoffset, double yoffset) {
    winput_glfw_scroll_callback(w, xoffset, yoffset);
}

void hook_glfw_callbacks(Ctx *ctx) {
    GLFWwindow* window = (GLFWwindow*)GetWindowHandle();
    glfwSetWindowUserPointer(window, ctx);
    wassert(glfwSetMouseButtonCallback(window, glfw_mouse_callback));
    wassert(glfwSetScrollCallback(window, glfw_scroll_callback));
}

void process_quit_key_combo(void) {
    if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
        && IsKeyPressed(KEY_ESCAPE))
    {
        GLFWwindow* window = (GLFWwindow*)GetWindowHandle();
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(void) {
    printf("Hello there!\n");
    printfd("HI!");

    Ctx ctx = { 0 };
    init_ctx(&ctx);

    int init_win_w = 1300;
    int init_win_h = 800;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(init_win_w, init_win_h, "woyspritemeta");

    // Post InitWindow setup:

    SetTargetFPS(30);
    SetExitKey(KEY_NULL);

    drawbuf_init();
    hook_glfw_callbacks(&ctx);
    ctx_load_assets(&ctx);

    while(!WindowShouldClose())
    {
        ++ctx.ticks;
        process_quit_key_combo();

        {
            winput_sync_frame(&ui_winput_frame); // TODO: Move me.
        }

        BeginDrawing();
        ClearBackground(BLACK);
        
        // Reset arena.
        ctx.frame_arena.arena = ArenaRoot_get_arena(ctx.frame_arena.root);

        ui_draw_all(&ctx);
        editor_process_cursor_logic(&ctx);
        editor_process_delete(&ctx);

        quickmonitor_draw();
        quickmonitor_line("fps %d", GetFPS());
        ptrdiff_t used = (ptrdiff_t)(ctx.frame_arena.arena.beg - ctx.frame_arena.root.buf);
        ptrdiff_t total = (ptrdiff_t)(ctx.frame_arena.root.cap);
        quickmonitor_line("frame_arena %td/%td (%.3f%%)", used, total, ((float)used/(float)total)*100.0);

        winput_consume_all();
        uimouseinput__frame_end();
        EndDrawing();
    }

    free_ctx(&ctx);
    CloseWindow();
    drawbuf_deinit();

    return 0;
}
