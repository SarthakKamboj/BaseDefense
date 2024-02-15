#pragma once

#include "gfx/gfx_data/object_data.h"
#include "gos.h"

enum PREVIEW_MODE {
	PREVIEW_GUN = 0,
	PREVIEW_BASE_EXT,
	PREVIEW_BASE,
	NUM_PREVIEWABLE_ITEMS,
};

struct att_summary_info_t {
    int att_handle = -1;
    int att_transform_handle = -1;
    float x_pos = 0;
    float y_pos = 0;
    ATTACHMENT_TYPE attachment_types = ATTMNT_NONE;
};

enum PREVIEW_SELECTOR_MODE {
    PREVIEW_SELECTOR_ITEM = 0,
    PREVIEW_SELECTOR_BASE_EXT_TYPE,
	PREVIEW_SELECTOR_NONE
};

struct preview_state_t {
    PREVIEW_MODE cur_mode = PREVIEW_BASE;
    bool preview_selector_open = false;
    PREVIEW_MODE cur_preview_selector_selected = PREVIEW_BASE;

	bool first_base_previewed = false;

    BASE_EXT_TYPE cur_base_ext_select_mode = TWO_ATT_HORIZONTAL;
    PREVIEW_SELECTOR_MODE selector_mode = PREVIEW_SELECTOR_NONE;
    time_count_t last_time_selector_open_press = -TIME_UNTIL_BASE_EXT_SELECTOR_CAN_OPEN;

    static const float TIME_UNTIL_BASE_EXT_SELECTOR_CAN_OPEN;
    
    static render_object_data preview_render_data;
    
    int item_selector_tex_handle = -1;
    int base_ext_selector_tex_handle = -1;

    int transform_handle = -1;

    // int quad_render_handle = -1;

    glm::vec2 window_rel_size = glm::vec2(0);

    static const float START_WIDTH;
    static const float START_HEIGHT;

    float cur_width = 0;
    float cur_height = 0;

    gun_t preview_gun;
    base_extension_t preview_base_ext;
    bool preview_base_valid = true;
    base_t preview_base;

    std::vector<att_summary_info_t> sorted_att_infos;
    int active_att_idx = -1;
};

void init_preview_mode();
void update_preview_mode();
void render_preview_mode();

void init_preview_base();
void update_preview_base();

void init_base_ext_preview();
void init_preview_gun();

void update_attachable_preview_item();
void add_attachment_to_preview_manager(attachment_t& att);
void delete_attachment_from_preview_manager(int att_handle);

void move_att_selection_left();
void move_att_selection_right();