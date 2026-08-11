#include "stdio.h"
#include "portable_utils.h"
#include "raylib.h"
#include "GLFW/glfw3.h"
#include "winput.h"
#include "state.h"
#include "ui.h"
#include "quick_monitor.h"
#include "raylib_drawbuffer.h"
#include "kinput.h"

#define LA_IMPLEMENTATION
#include "la.h"



GLFWmousebuttonfun raylib_mouse_callback = NULL;
GLFWscrollfun raylib_scroll_callback = NULL;
GLFWkeyfun raylib_key_callback = NULL;

void glfw_mouse_callback(GLFWwindow* w, int button, int action, int mods) {
    winput_glfw_mouse_button_callback(w, button, action, mods);
    uimouseinput_glfw_mouse_button_callback(w, button, action, mods);
    raylib_mouse_callback(w, button, action, mods);
}

void glfw_scroll_callback(GLFWwindow* w, double xoffset, double yoffset) {
    winput_glfw_scroll_callback(w, xoffset, yoffset);
    raylib_scroll_callback(w, xoffset, yoffset);
}

void glfw_key_callback(GLFWwindow* w, int key, int scancode, int action, int mods) {
    kinput_glfw_key_callback(w, key, scancode, action, mods);
    raylib_key_callback(w, key, scancode, action, mods);
}

void hook_glfw_callbacks(void) {
    GLFWwindow* window = (GLFWwindow*)GetWindowHandle();
    raylib_mouse_callback = glfwSetMouseButtonCallback(window, glfw_mouse_callback);
    raylib_scroll_callback = glfwSetScrollCallback(window, glfw_scroll_callback);
    raylib_key_callback = glfwSetKeyCallback(window, glfw_key_callback);
}

void process_quit_key_combo(void) {
    if (kinput_key_held_CTRL() && kinput_key_pressed(KEY_ESCAPE))
    {
        GLFWwindow* window = (GLFWwindow*)GetWindowHandle();
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

int main(void) {
    printf("Hello there!\n");
    printfd("HI!");

    Ctx ctx = { 0 };
    ctx_init(&ctx);

    int init_win_w = 1300;
    int init_win_h = 800;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(init_win_w, init_win_h, "woyspritemeta");

    // Post InitWindow setup:

    SetTargetFPS(30);
    SetExitKey(KEY_NULL);

    drawbuf_init();
    hook_glfw_callbacks();
    ctx_load_assets(&ctx);

    while(!WindowShouldClose() && !WINDOW_SHOULD_CLOSE)
    {
        ++ctx.ticks;
        process_quit_key_combo();

        {
            winput_sync_frame(&ui_winput_frame); // TODO: Move me.
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawRectangleLinesi(
            (Rect2i){{.width=GetScreenWidth(),.height= GetScreenHeight()}},
            WINDOW_BORDER_COLOR, 1);

        // Reset arena.
        ctx.frame_arena = ArenaRoot_get_arena(ctx.frame_arena_root);

        process_shortcuts(&ctx);
        ui_draw_all(&ctx);
        editor_process_cursor_logic(&ctx);
        editor_process_delete(&ctx);

        {
            // TODO: Make it so we can add lines from anywhere.
            static bool show_quickmonitor = false;
            if (IsKeyPressed(KEY_F3)) {
                show_quickmonitor = !show_quickmonitor;
            }
            if (show_quickmonitor) {
                quickmonitor_draw();
                quickmonitor_line("fps %d", GetFPS());
                ptrdiff_t used = (ptrdiff_t)(ctx.frame_arena.beg - ctx.frame_arena_root.buf);
                ptrdiff_t total = (ptrdiff_t)(ctx.frame_arena_root.cap);
                quickmonitor_line("frame_arena %td/%td (%.3f%%)", used, total, ((float)used/(float)total)*100.0);
            }
        }

        winput_consume_all();
        kinput_frame_end();
        uimouseinput__frame_end();
        EndDrawing();
    }

    ctx_free(&ctx);
    CloseWindow();
    drawbuf_deinit();

    return 0;
}
