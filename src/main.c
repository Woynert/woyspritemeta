#include "stdio.h"
#include "portable_utils.h"
#include "raylib.h"
#include "GLFW/glfw3.h"
#include "better_mouse_input.h"
#include "state.h"
#include "ui.h"
#include "tinyfiledialogs.h"

#define LA_IMPLEMENTATION
#include "la.h"

void glfw_mouse_callback(GLFWwindow* w, int button, int action, int mods) {
    BetterMouse_glfw_mouse_button_callback(w, button, action, mods);
}

void glfw_scroll_callback(GLFWwindow* w, double xoffset, double yoffset) {
    BetterMouse_glfw_scroll_callback(w, xoffset, yoffset);
}

void hook_glfw_callbacks(Ctx *ctx) {
    GLFWwindow* window = (GLFWwindow*)GetWindowHandle();
    glfwSetWindowUserPointer(window, ctx);
    ASSERT(glfwSetMouseButtonCallback(window, glfw_mouse_callback));
    ASSERT(glfwSetScrollCallback(window, glfw_scroll_callback));
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

    const int init_win_w = 1200;
    const int init_win_h = 500;

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(init_win_w, init_win_h, "woyspritemeta");

    // Post InitWindow setup:

    SetTargetFPS(30);
    SetExitKey(KEY_NULL);

    hook_glfw_callbacks(&ctx);
    ctx_load_assets(&ctx);

    while(!WindowShouldClose())
    {
        process_quit_key_combo();

        BeginDrawing();
        ClearBackground(BLACK);

        // ...
        /*DrawFPS(0,0);*/
        /*if (BetterMouse_is_pressed(MOUSE_BUTTON_LEFT)) {*/
            /*printfd("HERE!");*/
            /*tinyfd_messageBox("My title", "My message", "okcancel", "info", 0);*/
            /*char * result = tinyfd_inputBox("My title", "Insert name", " ");*/
            /*printfd("[%s]", result);*/
        /*}*/

        ui_draw_all(&ctx);
        editor_process_keybinds(&ctx);
        editor_process_cursor_drag(&ctx);

        BetterMouse_consume_all();
        EndDrawing();
    }

    free_ctx(&ctx);
    CloseWindow();

    return 0;
}
