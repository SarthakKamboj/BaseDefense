#pragma once

#include "window.h"
#include "camera.h"

struct globals_t {
    window_t window;
    camera_t camera;
    bool running = true;
};