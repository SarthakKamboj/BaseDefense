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
    
    static render_object_data preview_render_data;
    
    int circle_tex_handle = -1;
    int transform_handle = -1;
    int quad_render_handle = -1;
};

void init_preview_mode();
void update_preview_mode();
void render_preview_mode();