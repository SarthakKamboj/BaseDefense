#pragma once

#include "constants.h"
#include "ui_hash.h"
#include "ui.h"

#include <vector>

enum UI_PROPERTIES : int {
    UI_PROP_NONE = 0,
    UI_PROP_CLICKABLE = 1<<0,
    UI_PROP_HOVERABLE = 1<<1,
    UI_PROP_FOCUSABLE = 1<<2,
    UI_PROP_CURRENTLY_FOCUSED = 1<<3
};
OR_ENUM_DEFINITION(UI_PROPERTIES)
void print_ui_properties(UI_PROPERTIES props);

struct text_t {
    char text[256]{};
    int font_size = 25;
};
void create_text(const char* text, int font_size = 0, bool focusable = false);
bool create_button(const char* text, int font_size = 0, int user_handle = -1);

struct image_container_t {
    int texture_handle = -1;
    float width = 0.f;
    float height = 0.f;
};
void create_image_container(int texture_handle, float width, float height, WIDGET_SIZE widget_size_width, WIDGET_SIZE widget_size_height, const char* img_name);
bool create_selector(int selected_option, const char** options, int num_options, float width, float height, int& updated_selected_option, const char* selector_summary, int left_arrow_user_handle, int right_arrow_user_handle);

typedef int(*stacked_nav_handler_func_t)(bool right, bool left, bool up, bool down);

struct style_t;
struct widget_t {
    int handle = -1;
    bool absolute = false;

    int z_pos = 0;

    bool text_based = false;
    text_t text_info;

    bool image_based = false;
    int texture_handle = -1;

    std::vector<int> children_widget_handles;
    int parent_widget_handle = NULL;

    char key[256]{};
    hash_t hash;
    int index_to_prev_for_same_widget = -1;

    UI_PROPERTIES properties = UI_PROP_NONE;
    style_t style;

    int attached_hover_anim_player_handle = -1;
    std::vector<int> attached_anim_player_handles;

    // all specified in pixels with (x, y) using top left as the pt
    float x = -1.f;
    float y = -1.f;

    // does not include margins, just base width plus padding
    float render_width = -1.f;
    float render_height = -1.f;

    // includes base width, padding, and margins
    float content_width = -1.f;
    float content_height = -1.f;

    // extra info for stacked navigation
    bool stacked_navigation = false;
    stacked_nav_handler_func_t stack_nav_handler_func = NULL;
    int user_handle = -1;

};
widget_t create_widget();

void pop_widget();

void create_panel(const char* panel_name, int z_pos);
void end_panel();

void create_absolute_panel(const char* panel_name, int z_pos);
void end_absolute_panel();

void create_container(float width, float height, WIDGET_SIZE widget_size_width, WIDGET_SIZE widget_size_height, const char* container_name, bool focusable = false, stacked_nav_handler_func_t func = NULL, UI_PROPERTIES ui_properties = UI_PROP_NONE);
void end_container();

void create_absolute_container(float x, float y, float width, float height, WIDGET_SIZE widget_size_width, WIDGET_SIZE widget_size_height, const char* container_name);

widget_t* get_widget(int widget_handle);
