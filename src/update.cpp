#include "update.h"

#include <cassert>

#include "utils/time.h"
#include "animation/animation.h"
#include "physics/physics.h"
#include "gameobjects/gos.h"
#include "camera.h"
#include "globals.h"

extern globals_t globals;

void update() {
    update_camera();
    update_rigidbodies();
    update_image_anim_players();
    gos_update();
    scene_manager_update(globals.scene_manager);
}