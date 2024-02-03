#pragma once

#include "gfx/gfx_data/object_data.h"

enum PREVIEW_MODE {
	PREVIEW_GUN = 0,
	PREVIEW_BASE_EXT,
	PREVIEW_BASE,
	NUM_PREVIEWABLE_ITEMS,

	PREVIEW_NONE
};

struct preview_state_t {
    PREVIEW_MODE cur_mode = PREVIEW_NONE;
    bool preview_selector_open = false;
    PREVIEW_MODE cur_preview_selector_selected = PREVIEW_NONE;
    
    static render_object_data preview_render_data;
    
    int circle_tex_handle = -1;
    int transform_handle = -1;
    int quad_render_handle = -1;

    glm::vec2 window_rel_size = glm::vec2(0);

    static const float START_WIDTH;
    static const float START_HEIGHT;

    float cur_width = 0;
    float cur_height = 0;
};

void init_preview_mode();
void update_preview_mode();
void render_preview_mode();