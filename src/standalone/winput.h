#ifndef WINPUT_H
#define WINPUT_H

#include "GLFW/glfw3.h"
#include "raylib.h"
#include "stdbool.h"
#include "assert.h"

/*
    USAGE:

    Step 1: Hook GLFW events from your application:

        void glfw_mouse_callback(GLFWwindow* w, int button, int action, int mods) {
            winput_glfw_mouse_button_callback(w, button, action, mods);
        }

        void glfw_scroll_callback(GLFWwindow* w, double xoffset, double yoffset) {
            winput_glfw_scroll_callback(w, xoffset, yoffset);
        }

        void hook_glfw_callbacks(void) {
            assert(glfwSetMouseButtonCallback(window, glfw_mouse_callback));
            assert(glfwSetScrollCallback(window, glfw_scroll_callback));
            return 0;
        }

        main (...) {
            ...
            InitWindow(...);
            hook_glfw_callbacks(); <-- Call after InitWindow.
            ...
        }

    Step 2: Modify your main loop to include this functions:

        while (!WindowShouldClose()) {
            ... Drawing ...
            winput_consume_all(); <-- Call it just before EndDrawing.
            EndDrawing();
        }
 */

typedef enum WinputMice {
    MouseLeft   = MOUSE_BUTTON_LEFT,
    MouseRight  = MOUSE_BUTTON_RIGHT,
    MouseMiddle = MOUSE_BUTTON_MIDDLE,
} WinputMice;


typedef struct WinputFrame {
    struct {
        bool pressed;
        bool held;
        bool released;
        bool ignore_next_release;
    } button[MouseMiddle+1];
    float wheel_x;
    float wheel_y;
} WinputFrame;


WinputFrame winput__state = { 0 };


void winput_glfw_mouse_button_callback(GLFWwindow* w, int button, int action, int mods)
{
    int local_button;
    if      (button == GLFW_MOUSE_BUTTON_LEFT)   { local_button = MouseLeft; }
    else if (button == GLFW_MOUSE_BUTTON_MIDDLE) { local_button = MouseMiddle; }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT)  { local_button = MouseRight; }
    else { return; }

    if (action == GLFW_PRESS) {
        winput__state.button[local_button].pressed = true;
        winput__state.button[local_button].held = true;
    } else if (action == GLFW_RELEASE) {
        if (winput__state.button[local_button].ignore_next_release) {
            winput__state.button[local_button].ignore_next_release = false;
        } else {
            winput__state.button[local_button].released = true;
        }
    }
}


void winput_glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    winput__state.wheel_y = (float)yoffset;
    winput__state.wheel_x = (float)xoffset;
}


void winput_sync_frame(WinputFrame *frame) {
    frame->wheel_x = winput__state.wheel_x;
    frame->wheel_y = winput__state.wheel_y;
    for (int i = MouseLeft; i <= MouseMiddle; ++i) {
        frame->button[i].pressed  = winput__state.button[i].pressed;
        if (!frame->button[i].ignore_next_release) {
            frame->button[i].held = winput__state.button[i].held;
        }
        frame->button[i].released = winput__state.button[i].released;
        if (frame->button[i].released && frame->button[i].ignore_next_release) {
            frame->button[i].ignore_next_release = false;
            frame->button[i].released = false;
        }
    }
}


/* Call at frame end. */
void winput_consume_all(void) {
    for (int i = MouseLeft; i <= MouseMiddle; ++i) {
        winput__state.button[i].pressed = false;
        if (winput__state.button[i].released) {
            /* Held is reset only when Released was triggered. */
            winput__state.button[i].held = false;
        }
        winput__state.button[i].released = false;
    }

    winput__state.wheel_x = 0;
    winput__state.wheel_y = 0;
}

void winput_consume(WinputFrame *frame, WinputMice button, bool trigger_release) {
    if (frame == NULL) { frame = &winput__state; }
    assert(button <= MouseMiddle);
    frame->button[button].pressed             = false;
    frame->button[button].held                = false;
    frame->button[button].ignore_next_release = true;
    frame->button[button].released            = trigger_release;
}

bool winput_frame_mice_pressed(WinputFrame *frame, WinputMice button) {
    assert(button <= MouseMiddle);
    return frame->button[button].pressed;
}

bool winput_frame_mice_held(WinputFrame *frame, WinputMice button) {
    assert(button <= MouseMiddle);
    return frame->button[button].held;
}

bool winput_frame_mice_released(WinputFrame *frame, WinputMice button) {
    assert(button <= MouseMiddle);
    return frame->button[button].released;
}

bool winput_mice_pressed(WinputMice button) {
    return winput_frame_mice_pressed(&winput__state, button);
}

bool winput_mice_held(WinputMice button) {
    return winput_frame_mice_held(&winput__state, button);
}

bool winput_mice_released(WinputMice button) {
    return winput_frame_mice_released(&winput__state, button);
}

float winput_wheel(void) {
    return winput__state.wheel_y;
}

float winput_wheel_h(void) {
    return winput__state.wheel_x;
}


#endif // !WINPUT_H
