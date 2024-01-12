#include "update.h"
#include "utils/time.h"
#include <cassert>
#include "animation/animation.h"
#include "physics/physics.h"
#include "gameobjects/gos.h"
#include "camera.h"

void update() {
    update_camera();
    update_rigidbodies();
    update_image_anim_players();
    gos_update();
}