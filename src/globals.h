#pragma once

#include "window.h"
#include "camera.h"
#include "scene_manger.h"

struct globals_t {
    window_t window;
    camera_t camera;
    bool running = true;
    scene_manager_t scene_manager;
    bool ui_clicked_on = false;
};