#include "ui.h"

#include <iostream>
#include <unordered_map>
#include <filesystem>

#include "constants.h"
#include "utils/io.h"
#include "gfx/quad.h"
#include "gfx/gfx_data/vertex.h"
#include "gfx/gfx_data/texture.h"
#include "gfx/renderer.h"
#include "globals.h"
#include "utils/general.h"
#include "utils/time.h"
#include "utils/json.h"

#include <glm/gtx/string_cast.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H  

namespace fs = std::filesystem;

static bool panel_left_used = false;

extern globals_t globals;

static int latest_z_pos = -200;

// static char xml_path[256];

// static ui_file_layout_t ui_file;
static std::vector<ui_file_layout_t> ui_files;
static std::vector<int> active_ui_file_handles;

static std::vector<ui_anim_file_t> ui_anim_files;
static std::vector<ui_anim_t> ui_anims;
static std::vector<int> active_ui_anim_file_handles;

static std::vector<ui_anim_player_t> ui_anim_players;

static int cur_focused_internal_handle = -1;
static int cur_final_focused_handle = -1;
static bool stack_nav_cur = false;

static int cur_widget_count = 0;

static std::vector<int> widget_stack1;
static std::vector<widget_t> widgets_arr1;
static ui_element_status_t ui_element_status1;

static std::vector<int> widget_stack2;
static std::vector<widget_t> widgets_arr2;
static ui_element_status_t ui_element_status2;

static std::vector<int>* curframe_widget_stack = &widget_stack2;
static std::vector<widget_t>* curframe_widget_arr = &widgets_arr2;
static ui_element_status_t* curframe_ui_element_status;
static bool stacked_nav_widget_in_stack = false;

static std::vector<int>* prevframe_widget_stack = &widget_stack1;
static std::vector<widget_t>* prevframe_widget_arr = &widgets_arr1;
static ui_element_status_t* prevframe_ui_element_status;

static std::vector<style_t> styles_stack;

static std::unordered_map<int, constraint_var_t> constraint_vars;
static std::vector<constraint_t> constraints;

static int constraint_running_cnt = 0;

static bool ui_will_update = false;

static std::unordered_map<int, hash_t> handle_hashes;

static std::unordered_map<std::string, std::string> ui_text_values;

static std::vector<font_mode_t> font_modes;
render_object_data font_char_t::ui_opengl_data{};

static std::vector<ui_anim_user_info_t> anims_to_add_this_frame;
static std::vector<ui_anim_user_info_t> anims_to_start_this_frame;
static std::vector<ui_anim_user_info_t> anims_to_stop_this_frame;

void update_ui_files() {
    for (int i = 0; i < ui_files.size(); i++) {
        ui_file_layout_t& ui_file = ui_files[i];
        struct stat ui_file_stat {};

        if (stat(ui_file.path, &ui_file_stat) < 0) continue;

        if (ui_file.last_modified_time == ui_file_stat.st_mtime) continue;
        _sleep(10);
        xml_document_free(ui_file.document, true);

        FILE* file = fopen(ui_file.path, "r");
        game_assert_msg(file, "ui file not found");
        ui_file.document = xml_open_document(file);
        ui_file.last_modified_time = ui_file_stat.st_mtime;
    }
}

void ui_start_of_frame() {
    panel_left_used = false;
    latest_z_pos = -200;
#if UI_RELOADING
    update_ui_files();
#endif

    if (!globals.window.user_input.game_controller) {
        cur_focused_internal_handle = -1;
        cur_final_focused_handle = -1;
        stack_nav_cur = false;
    }

    glm::mat4 projection = glm::ortho(0.0f, globals.window.window_width, 0.0f, globals.window.window_height);
    shader_set_mat4(font_char_t::ui_opengl_data.shader, "projection", projection);
    ui_will_update = globals.window.resized;

    for (int i = 0; i < font_modes.size(); i++) {
        font_modes[i].used_last_frame = false;
    }

    ui_text_values.clear();
    styles_stack.clear();
    style_t default_style;
    styles_stack.push_back(default_style);

    if (curframe_widget_arr == &widgets_arr1) {
        curframe_widget_arr = &widgets_arr2;
        curframe_widget_stack = &widget_stack2;
        curframe_ui_element_status = &ui_element_status2;
        prevframe_widget_arr = &widgets_arr1;
        prevframe_widget_stack = &widget_stack1;
        prevframe_ui_element_status = &ui_element_status1;
    } else {
        curframe_widget_arr = &widgets_arr1;
        curframe_widget_stack = &widget_stack1;
        curframe_ui_element_status = &ui_element_status1;
        prevframe_widget_arr = &widgets_arr2;
        prevframe_widget_stack = &widget_stack2;
        prevframe_ui_element_status = &ui_element_status2;
    }

    curframe_widget_arr->clear();
    curframe_widget_stack->clear();
    clear_element_status(*curframe_ui_element_status);

    cur_widget_count = 0;

    constraint_vars.clear();
    constraints.clear();
    constraint_running_cnt = 0;
}

void push_style(style_t& style) {
    styles_stack.push_back(style);
}

void pop_style() {
    if (styles_stack.size() == 0) {
        std::cout << "styles stack is empty" << std::endl;
        return;
    }
    styles_stack.pop_back();
}

void pop_widget() {
    if (curframe_widget_stack->size() > 0) {
        auto& arr = *curframe_widget_arr;
        auto& stack = *curframe_widget_stack;
        if (arr[stack[stack.size()-1]].stacked_navigation) {
            stacked_nav_widget_in_stack = false;
        }
        stack.pop_back();
    }
}

widget_t create_widget() {
    widget_t widget;
    widget.style = styles_stack[styles_stack.size() - 1];
    return widget;
}

void register_absolute_widget(widget_t& widget, const char* key, bool push_onto_stack) {
    widget.handle = cur_widget_count++;
    widget.absolute = true;
    memcpy(widget.key, key, strlen(key));
    widget.hash = hash(key);

    auto& arr = *curframe_widget_arr;
    auto& stack = *curframe_widget_stack;

    if (stack.size() > 0) widget.parent_widget_handle = stack[stack.size() - 1];
    else widget.parent_widget_handle = -1;
    if (widget.parent_widget_handle != -1) {
        arr[widget.parent_widget_handle].children_widget_handles.push_back(widget.handle);
    }

    arr.push_back(widget);
    if (push_onto_stack) {
        stack.push_back(widget.handle);
    }
}

widget_registration_info_t register_widget(widget_t& widget, const char* key, bool push_onto_stack) {

    widget_registration_info_t info;

    widget.handle = cur_widget_count++;
    memcpy(widget.key, key, strlen(key));
    hash_t new_hash = hash(key);

    auto& arr = *curframe_widget_arr;
    auto& stack = *curframe_widget_stack;

    auto& prev_arr = *prevframe_widget_arr; 

    if (prev_arr.size() <= widget.handle) {
        ui_will_update = true;
    } else {
        hash_t& prev_hash = prev_arr[widget.handle].hash;
        ui_will_update = ui_will_update || !is_same_hash(new_hash, prev_hash);
    }

    widget.hash = new_hash;
    if (stack.size() > 0) widget.parent_widget_handle = stack[stack.size() - 1];
    else widget.parent_widget_handle = -1;
    if (widget.parent_widget_handle != -1) {
        arr[widget.parent_widget_handle].children_widget_handles.push_back(widget.handle);
    }

    // could lead to problem if there is a entire ui switch/scene change but coincidentally,
    // there is the same key in the previous ui and the new ui although the anim player
    // from the previous ui has been cleared

    // this should be fixed because previous array widget's attached_hover_anim_player_handle will
    // be set to -1 when anim players are cleared out, so even if there is a match, the new widget
    // still just holds a -1 in attached_hover_anim_player_handle
    for (int i = 0; i < prev_arr.size(); i++) {
        if (is_same_hash(prev_arr[i].hash, new_hash)) {
            widget.attached_hover_anim_player_handle = prev_arr[i].attached_hover_anim_player_handle;
            for (int j = 0; j < prev_arr[i].attached_anim_player_handles.size(); j++) {
                int anim_player_handle = prev_arr[i].attached_anim_player_handles[j];
                widget.attached_anim_player_handles.push_back(anim_player_handle);
            }
        }
    }

    if (widget.attached_hover_anim_player_handle == -1) {
        for (int k = 0; k < ui_anims.size(); k++) {
            char hover_anim_name[128]{};
            sprintf(hover_anim_name, "%s:hover", widget.key);
            if (strcmp(ui_anims[k].anim_name, hover_anim_name) == 0) {
                widget.attached_hover_anim_player_handle = create_ui_anim_player(widget.key, ui_anims[k].ui_file_handle, ui_anims[k], true); 
            }
        }
    } 

    ui_anim_player_t* hover_anim_player = get_ui_anim_player(widget.attached_hover_anim_player_handle);
    for (int i = 0; i < anims_to_add_this_frame.size(); i++) {
        ui_anim_user_info_t& add_info = anims_to_add_this_frame[i];
        if (strcmp(widget.key, add_info.widget_key) == 0) {
            for (int k = 0; k < ui_anims.size(); k++) {
                ui_anim_t& anim = ui_anims[k];
                if (strcmp(anim.anim_name, add_info.ui_anim_name) == 0) {
                    int anim_player_handle = create_ui_anim_player(widget.key, anim.ui_file_handle, anim, add_info.start_playing, add_info.start_anim_duration_cursor);
                    widget.attached_anim_player_handles.push_back(anim_player_handle);
                }
            }
        }
    }

    ui_anim_user_info_t* start_anim_user_info = NULL;
    for (int i = 0; i < anims_to_start_this_frame.size(); i++) {
        if (strcmp(widget.key, anims_to_start_this_frame[i].widget_key) == 0) {
            start_anim_user_info = &anims_to_start_this_frame[i];
            break;
        }
    }

    if (start_anim_user_info) {
        for (int i = 0; i < widget.attached_anim_player_handles.size(); i++) {
            ui_anim_player_t* attached_player = get_ui_anim_player(widget.attached_anim_player_handles[i]);
            game_assert_msg(attached_player, "could not find anim player for the animation trying to be started");
            ui_anim_t* attached_player_anim = get_ui_anim(attached_player->ui_anim_handle);
            game_assert_msg(attached_player_anim, "could not find anim for attached player");
            if (strcmp(attached_player_anim->anim_name, start_anim_user_info->ui_anim_name) == 0) {
                attached_player->playing = true;
            }
        }
    }

    ui_anim_user_info_t* stop_anim_user_info = NULL;
    for (int i = 0; i < anims_to_stop_this_frame.size(); i++) {
        if (strcmp(widget.key, anims_to_stop_this_frame[i].widget_key) == 0) {
            stop_anim_user_info = &anims_to_stop_this_frame[i];
            break;
        }
    }

    if (stop_anim_user_info) {
        for (int i = 0; i < widget.attached_anim_player_handles.size(); i++) {
            ui_anim_player_t* attached_player = get_ui_anim_player(widget.attached_anim_player_handles[i]);
            game_assert_msg(attached_player, "could not find anim player for the animation trying to be started");
            ui_anim_t* attached_player_anim = get_ui_anim(attached_player->ui_anim_handle);
            game_assert_msg(attached_player_anim, "could not find anim for attached player");
            if (strcmp(attached_player_anim->anim_name, stop_anim_user_info->ui_anim_name) == 0) {
                attached_player->playing = false;
            }
        }
    }   

    style_t original_style = widget.style;
    if (hover_anim_player) {
        widget.style = get_intermediate_style(original_style, *hover_anim_player);
    }

    for (int i = 0; i < widget.attached_anim_player_handles.size(); i++) {
        ui_anim_player_t* attached_player = get_ui_anim_player(widget.attached_anim_player_handles[i]);
        game_assert_msg(attached_player, "could not find animation player");
        if (attached_player->duration_cursor / attached_player->anim_duration > 0) {
            widget.style = get_intermediate_style(original_style, *attached_player);
        }
    }

    arr.push_back(widget);
    if (push_onto_stack) {
        stack.push_back(widget.handle);
        if (widget.stacked_navigation) {
            stacked_nav_widget_in_stack = true;
        }
    }
    
    info.widget_handle = widget.handle;
    
    if (!ui_will_update && (widget.properties & UI_PROP_CLICKABLE)) {
        auto& prev_arr = *prevframe_widget_arr;
        widget_t& cached_widget = prev_arr[widget.handle];

        user_input_t& input_state = globals.window.user_input;
        bool mouse_over_widget = input_state.mouse_x >= (cached_widget.x + cached_widget.style.margin.x) &&
                input_state.mouse_x <= (cached_widget.x + cached_widget.render_width + cached_widget.style.margin.x) &&
                // render x and render y specified as the top left pivot and y in ui is 0 on the
                // bottom and WINDOW_HEIGHT on the top, so cached_widget.y is the top y of the 
                // widget and cached_widget.y - cached_widget.render_height is the bottom y 
                // of the widget
                input_state.mouse_y <= (cached_widget.y - cached_widget.style.margin.y) &&
                input_state.mouse_y >= (cached_widget.y - cached_widget.render_height - cached_widget.style.margin.y);

        if (!mouse_over_widget && strcmp(prevframe_ui_element_status->hovered_over.widget_key.c_str(), key) == 0) {
            curframe_ui_element_status->mouse_left.widget_key = key;
        }

        if (mouse_over_widget && !input_state.game_controller) {

            if (latest_z_pos > curframe_ui_element_status->hovered_over.z_pos) {
                cur_focused_internal_handle = widget.handle;

                curframe_ui_element_status->hovered_over.widget_key = key;
                curframe_ui_element_status->hovered_over.z_pos = latest_z_pos;
            
                if (strcmp(prevframe_ui_element_status->hovered_over.widget_key.c_str(), key) != 0) {
                    curframe_ui_element_status->mouse_enter.widget_key = key;
                }
            }
            info.hovering_over = true;
        }

        if (mouse_over_widget && input_state.left_mouse_release) {
            curframe_ui_element_status->clicked_on.widget_key = key;
            curframe_ui_element_status->clicked_on.z_pos = latest_z_pos;
            info.clicked_on = true;
        }

        if (widget.handle == cur_final_focused_handle && (input_state.enter_pressed || input_state.controller_a_pressed)) {
            curframe_ui_element_status->clicked_on.widget_key = key;
            curframe_ui_element_status->clicked_on.z_pos = latest_z_pos;
            info.clicked_on = true;
        }
    }

    return info;
}

void create_panel(const char* panel_name, int z_pos) {
    widget_t panel = create_widget();
    memcpy(panel.key, panel_name, strlen(panel_name)); 
    panel.z_pos = z_pos;
    latest_z_pos = z_pos;

    panel.style.height = globals.window.window_height;
    panel.style.width = globals.window.window_width;
    panel.style.widget_size_width = WIDGET_SIZE::PIXEL_BASED;
    panel.style.widget_size_height = WIDGET_SIZE::PIXEL_BASED;

    panel.render_width = panel.style.width;
    panel.render_height = panel.style.height;

    panel.content_width = panel.style.width;
    panel.content_height = panel.style.height;

    register_widget(panel, panel_name, true);
}

void end_panel() {
    pop_widget();
}

void create_absolute_panel(const char* panel_name, int z_pos) {
    widget_t panel = create_widget();
    memcpy(panel.key, panel_name, strlen(panel_name)); 
    panel.z_pos = z_pos;
    latest_z_pos = z_pos;

    panel.style.height = globals.window.window_height;
    panel.style.width = globals.window.window_width;
    panel.style.widget_size_width = WIDGET_SIZE::PIXEL_BASED;
    panel.style.widget_size_height = WIDGET_SIZE::PIXEL_BASED;

    panel.render_width = panel.style.width;
    panel.render_height = panel.style.height;

    panel.content_width = panel.style.width;
    panel.content_height = panel.style.height;

    panel.x = 0;
    panel.y = 0;

    register_absolute_widget(panel, panel_name, true);
}

void end_absolute_panel() {
    pop_widget();
}

void create_absolute_container(float x, float y, float width, float height, WIDGET_SIZE widget_size_width, WIDGET_SIZE widget_size_height, const char* container_name) {

    game_assert_msg(widget_size_width == WIDGET_SIZE::PIXEL_BASED, "widget size width of absolute container must be pixel");
    game_assert_msg(widget_size_height == WIDGET_SIZE::PIXEL_BASED, "widget size width of absolute container must be pixel");

    widget_t container = create_widget();
    memcpy(container.key, container_name, strlen(container_name));
    container.style.height = height;
    container.style.width = width;
    container.style.widget_size_width = widget_size_width;
    container.style.widget_size_height = widget_size_height; 

    container.x = x;
    container.y = y;

    container.render_width = container.style.width;
    container.render_height = container.style.height;

    register_absolute_widget(container, container_name, false); 
}

void create_container(float width, float height, WIDGET_SIZE widget_size_width, WIDGET_SIZE widget_size_height, const char* container_name, bool focusable, stacked_nav_handler_func_t func, UI_PROPERTIES ui_properties) {
    widget_t container = create_widget();
    memcpy(container.key, container_name, strlen(container_name));
    container.style.height = height;
    container.style.width = width;
    container.style.widget_size_width = widget_size_width;
    container.style.widget_size_height = widget_size_height; 

    if (focusable) {
        container.properties = container.properties | UI_PROP_FOCUSABLE;
        container.stacked_navigation = true;
        container.stack_nav_handler_func = func;
    }

    container.properties = container.properties | ui_properties;

    register_widget(container, container_name, true); 
}

void end_container() {
    pop_widget();
}

void create_image_container(int texture_handle, float width, float height, WIDGET_SIZE widget_size_width, WIDGET_SIZE widget_size_height, const char* img_name) {

    game_assert(widget_size_width != WIDGET_SIZE::FIT_CONTENT);
    game_assert(widget_size_height != WIDGET_SIZE::FIT_CONTENT);

    texture_t* tex = get_tex(texture_handle);
    game_assert(tex);
    game_assert(tex->tex_slot == 1);

    widget_t widget = create_widget();
    widget.image_based = true;
    widget.texture_handle = texture_handle;
    widget.style.widget_size_width = widget_size_width;
    widget.style.widget_size_height = widget_size_height;
    widget.style.width = width;
    widget.style.height = height;

    register_widget(widget, img_name);
}

bool create_selector(int selected_option, const char** options, int num_options, float width, float height, int& updated_selected_option, const char* selector_summary, int left_arrow_user_handle, int right_arrow_user_handle) {
    style_t container_style;
    container_style.display_dir = DISPLAY_DIR::HORIZONTAL;
    container_style.horizontal_align_val = ALIGN::SPACE_BETWEEN;
    container_style.vertical_align_val = ALIGN::CENTER;
    push_style(container_style);
    create_container(width, height, WIDGET_SIZE::PIXEL_BASED, WIDGET_SIZE::PIXEL_BASED, "selector container");
    pop_style();
    
    bool changed = false;
    
    style_t enabled;
	enabled.hover_background_color = DARK_BLUE;
	enabled.hover_color = WHITE;
    enabled.color = DARK_BLUE;
    enabled.padding = glm::vec2(10);
    
    style_t disabled;
	disabled.hover_background_color = LIGHT_GREY;
    disabled.color = GREY;
    disabled.padding = glm::vec2(10);
    
    bool can_go_left = selected_option >= 1;
    if (can_go_left) {
        push_style(enabled);
    } else {
        push_style(disabled);
    }
    
    if (create_button("<", 10, left_arrow_user_handle)) {
        if (can_go_left) {
            updated_selected_option = selected_option - 1;
            changed = true;
        }
    }
    pop_style();

    char text_buffer[256]{};
    sprintf(text_buffer, "%s###%s", options[selected_option], selector_summary);
    create_text(text_buffer);

    bool can_go_right = selected_option <= num_options - 2;
    if (can_go_right) {
        push_style(enabled);
    } else {
        push_style(disabled);
    }
    if (create_button(">", 10, right_arrow_user_handle)) {
        if (can_go_right) {
            updated_selected_option = selected_option + 1;
            changed = true;
        }
    }
    pop_style();
    
    end_container();
    return changed;
}

void create_text(const char* text, int font_size, bool focusable) {
    const char* key = text;
    int text_len = strlen(text);
    const char* triple_hash = strstr(text, "###");
    if (triple_hash != NULL) {
        key = triple_hash + 3;
        text_len = triple_hash - text;
    }

    widget_t widget = create_widget();
    widget.text_based = true;
    memcpy(widget.text_info.text, text, fmin(sizeof(widget.text_info.text), text_len));
    widget.text_info.font_size = font_size;

    if (focusable) {
        widget.properties = static_cast<UI_PROPERTIES>(UI_PROPERTIES::UI_PROP_FOCUSABLE);
    }

    text_dim_t text_dim = get_text_dimensions(text, font_size);

    widget.style.widget_size_width = WIDGET_SIZE::PIXEL_BASED;
    widget.style.widget_size_height = WIDGET_SIZE::PIXEL_BASED;
    
    style_t& latest_style = styles_stack[styles_stack.size() - 1];
    widget.style.width = text_dim.width + (2 * latest_style.padding.x);
    widget.style.height = text_dim.max_height_above_baseline + (2 * latest_style.padding.y);

    widget.render_width = widget.style.width;
    widget.render_height = widget.style.height;

    register_widget(widget, key);
}

bool create_button(const char* text, int font_size, int user_handle) {

    const char* key = text;
    int text_len = strlen(text);
    const char* triple_hash = strstr(text, "###");
    if (triple_hash != NULL) {
        key = triple_hash + 3;
        text_len = triple_hash - text;
    }

    widget_t widget = create_widget();
    widget.text_based = true;
    memcpy(widget.text_info.text, text, fmin(sizeof(widget.text_info.text), text_len));
    widget.text_info.font_size = font_size;

    text_dim_t text_dim = get_text_dimensions(text, font_size);

    widget.style.widget_size_width = WIDGET_SIZE::PIXEL_BASED;
    widget.style.widget_size_height = WIDGET_SIZE::PIXEL_BASED;
    style_t& latest_style = styles_stack[styles_stack.size() - 1];
    widget.style.width = text_dim.width + (2 * latest_style.padding.x);
    widget.style.height = text_dim.max_height_above_baseline + (2 * latest_style.padding.y);

    widget.render_width = widget.style.width;
    widget.render_height = widget.style.height;

    widget.properties = widget.properties | UI_PROP_CLICKABLE | UI_PROP_HOVERABLE;
    auto& stack = *curframe_widget_stack;
    if (stacked_nav_widget_in_stack) {
        widget.user_handle = user_handle;
    } else {
        widget.properties = widget.properties | UI_PROP_FOCUSABLE;
    }

    widget_registration_info_t widget_info = register_widget(widget, key);

    return widget_info.clicked_on;
}


bool traverse_to_right_focusable_helper(int widget_handle, bool focus_from_cur) {
    auto& arr = *curframe_widget_arr;
    widget_t& widget = arr[widget_handle];
    for (int child_handle : widget.children_widget_handles) {
        bool refocused = traverse_to_right_focusable_helper(child_handle, focus_from_cur);
        if (refocused) return true;
    }
    if (widget.properties & UI_PROPERTIES::UI_PROP_FOCUSABLE) {
        if (!focus_from_cur || (focus_from_cur && cur_focused_internal_handle < widget.handle)) {
            cur_focused_internal_handle = widget_handle;
            stack_nav_cur = widget.stacked_navigation;
            // widget.properties = static_cast<UI_PROPERTIES>(widget.properties | UI_PROPERTIES::UI_PROP_CURRENTLY_FOCUSED);
            printf("focused on %s\n", widget.key);
            return true;
        }
    }
    return false;
}

void traverse_to_right_focusable() {
    bool refocused = traverse_to_right_focusable_helper(0, true);
    if (!refocused) traverse_to_right_focusable_helper(0, false);
}

bool traverse_to_left_focusable_helper(int widget_handle, bool focus_from_cur) {
    auto& arr = *curframe_widget_arr;
    widget_t& widget = arr[widget_handle];
    for (int i = widget.children_widget_handles.size() - 1; i >= 0; i--) {
        int child_handle = widget.children_widget_handles[i];
        bool refocused = traverse_to_left_focusable_helper(child_handle, focus_from_cur);
        if (refocused) return true;
    }
    if (widget.properties & UI_PROPERTIES::UI_PROP_FOCUSABLE) {
        if (!focus_from_cur || (focus_from_cur && cur_focused_internal_handle > widget.handle)) {
            cur_focused_internal_handle = widget_handle;
            stack_nav_cur = widget.stacked_navigation;
            // widget.properties = static_cast<UI_PROPERTIES>(widget.properties | UI_PROPERTIES::UI_PROP_CURRENTLY_FOCUSED);
            printf("focused on %s\n", widget.key);
            return true;
        }
    }
    return false;
}

void traverse_to_left_focusable() {
    bool refocused = traverse_to_left_focusable_helper(0, true);
    if (!refocused) traverse_to_left_focusable_helper(0, false);
}

void update_custom_stack_nav(bool right_move, bool left_move, bool up_move, bool down_move) {
    auto& arr = *curframe_widget_arr;
    widget_t& widget = arr[cur_focused_internal_handle];
    int focused_user_handle = widget.stack_nav_handler_func(right_move, left_move, up_move, down_move);
    if (focused_user_handle == -1) {
        stack_nav_cur = false;
    } else {
        for (widget_t& widget : arr) {
            if (widget.user_handle == focused_user_handle) {
                cur_final_focused_handle = widget.handle;
            }
        }
    }
}

void end_imgui() {
    static bool controller_centered_x = true;
    static bool controller_centered_y = true;

    user_input_t& input_state = globals.window.user_input;
    controller_centered_x = controller_centered_x || fabs(input_state.controller_x_axis) <= 0.4f;
    controller_centered_y = controller_centered_y || fabs(input_state.controller_y_axis) <= 0.4f;

    bool right_move = false;
    bool left_move = false;
    bool up_move = false;
    bool down_move = false;

    if (controller_centered_x && input_state.controller_x_axis >= 0.9f) {
        right_move = true;
        controller_centered_x = false;
    } else if (get_down('d')) {
        right_move = true;
    }

    if (controller_centered_x && input_state.controller_x_axis <= -0.9f) {
        left_move = true;
        controller_centered_x = false;
    } else if (get_down('a')) {
        left_move = true;
    }

    if (controller_centered_y && input_state.controller_y_axis <= -0.9f) {
        down_move = true;
        controller_centered_y = false;
    } else if (get_down('s')) {
        down_move = true;
    }

    if (controller_centered_y && input_state.controller_y_axis >= 0.9f) {
        up_move = true;
        controller_centered_y = false;
    } else if (get_down('w')) {
        up_move = true;
    } 

    if (ui_will_update) {
        stack_nav_cur = false;   
    }

    if (!stack_nav_cur) {
        if (right_move || down_move) {
            traverse_to_right_focusable();
        }

        if (left_move || up_move) {
            traverse_to_left_focusable();
        }
    }

    if (stack_nav_cur) {
        update_custom_stack_nav(right_move, left_move, up_move, down_move);

        if (!stack_nav_cur) {
            if (right_move || down_move) {
                traverse_to_right_focusable();
            }
        
            if (left_move || up_move) {
                traverse_to_left_focusable();
            }

            if (stack_nav_cur) {
                update_custom_stack_nav(right_move, left_move, up_move, down_move);
            } else {
                cur_final_focused_handle = cur_focused_internal_handle;
            }
        }

    } else { 
        cur_final_focused_handle = cur_focused_internal_handle;
    }  

    anims_to_add_this_frame.clear();
    anims_to_start_this_frame.clear();
    anims_to_stop_this_frame.clear();
}

struct helper_info_t {
    int content_width = 0;
    int content_height = 0;
};

helper_info_t resolve_positions(int widget_handle, int x_pos_handle, int y_pos_handle) {
    auto& cur_arr = *curframe_widget_arr;
    widget_t& widget = cur_arr[widget_handle];

    if (widget.text_based) {
        // render_width and render_height for text_based things are calculated upon instantiation
        helper_info_t helper_info;
        helper_info.content_width = widget.content_width;
        helper_info.content_height = widget.content_height;
        return helper_info;
    }

    int widget_width_handle = create_constraint_var_constant(widget.content_width);
    int widget_height_handle = create_constraint_var_constant(widget.content_height);

    int space_var_hor = create_constraint_var("spacing hor", NULL);
    int space_var_vert = create_constraint_var("spacing vert", NULL);
    int start_offset_hor = create_constraint_var("start offset hor", NULL);
    int start_offset_vert = create_constraint_var("start offset vert", NULL);

    int idx = 0;
    if (widget.style.display_dir == DISPLAY_DIR::HORIZONTAL && widget.style.horizontal_align_val == ALIGN::SPACE_AROUND ||
        widget.style.display_dir == DISPLAY_DIR::VERTICAL && widget.style.vertical_align_val == ALIGN::SPACE_AROUND) {
        idx++;
    }
    glm::vec2 content_size(0);

    for (int child_widget_handle : widget.children_widget_handles) {

        widget_t& child_widget = cur_arr[child_widget_handle];
        
        int child_widget_x_pos_handle = create_constraint_var("widget_x", &child_widget.x);
        int child_widget_y_pos_handle = create_constraint_var("widget_y", &child_widget.y);

        if (widget.style.display_dir == DISPLAY_DIR::HORIZONTAL) {
            std::vector<constraint_term_t> x_pos_terms;
            float constant = content_size.x;
            x_pos_terms.push_back(create_constraint_term(x_pos_handle, 1));
            x_pos_terms.push_back(create_constraint_term(start_offset_hor, 1));
            x_pos_terms.push_back(create_constraint_term(space_var_hor, idx));
            create_constraint(child_widget_x_pos_handle, x_pos_terms, constant);

            float vert_constant = 0;
            std::vector<constraint_term_t> y_pos_terms;
            y_pos_terms.push_back(create_constraint_term(y_pos_handle, 1));
            if (widget.style.vertical_align_val == ALIGN::END) {
                y_pos_terms.push_back(create_constraint_term(widget_height_handle, -1.f));
                vert_constant = child_widget.content_height;
            } else {
                y_pos_terms.push_back(create_constraint_term(start_offset_vert, 1));
                if (widget.style.vertical_align_val == ALIGN::CENTER) {
                    y_pos_terms.push_back(create_constraint_term(widget_height_handle, -0.5f));
                    vert_constant = 0.5f * child_widget.content_height;
                }
            }
            create_constraint(child_widget_y_pos_handle, y_pos_terms, vert_constant);
        } else {
            std::vector<constraint_term_t> y_pos_terms;
            float constant = -content_size.y;
            y_pos_terms.push_back(create_constraint_term(y_pos_handle, 1));
            y_pos_terms.push_back(create_constraint_term(start_offset_vert, 1));
            y_pos_terms.push_back(create_constraint_term(space_var_vert, idx));
            create_constraint(child_widget_y_pos_handle, y_pos_terms, constant);

            float hor_constant = 0;
            std::vector<constraint_term_t> x_pos_terms;
            x_pos_terms.push_back(create_constraint_term(x_pos_handle, 1));
            if (widget.style.horizontal_align_val == ALIGN::END) {
                x_pos_terms.push_back(create_constraint_term(widget_width_handle, 1));
                hor_constant = -child_widget.content_width;
            } else {
                x_pos_terms.push_back(create_constraint_term(start_offset_hor, 1));
                if (widget.style.horizontal_align_val == ALIGN::CENTER) {
                    x_pos_terms.push_back(create_constraint_term(widget_width_handle, 0.5f));
                    hor_constant = -0.5f * child_widget.content_width;
                }
            }
            create_constraint(child_widget_x_pos_handle, x_pos_terms, hor_constant);
        }

        helper_info_t child_helper_info = resolve_positions(child_widget_handle, child_widget_x_pos_handle, child_widget_y_pos_handle);
        if (widget.style.display_dir == DISPLAY_DIR::HORIZONTAL) {
            content_size.x += child_helper_info.content_width;
            content_size.y = fmax(content_size.y, child_helper_info.content_height);
        } else {
            content_size.x = fmax(content_size.x, child_helper_info.content_width);
            content_size.y += child_helper_info.content_height;
        }
        idx++;
    }

    resolve_constraints();

    int num_children = widget.children_widget_handles.size();
    switch (widget.style.horizontal_align_val) {
        case ALIGN::SPACE_BETWEEN: {
            if (widget.style.display_dir == DISPLAY_DIR::HORIZONTAL) {
                if (num_children == 1) {
                    make_constraint_value_constant(space_var_hor, (widget.content_width - content_size.x) / 2);
                } else {
                    make_constraint_value_constant(space_var_hor, (widget.content_width - content_size.x) / (num_children - 1));
                }
                make_constraint_value_constant(start_offset_hor, 0);
            } else {
                make_constraint_value_constant(space_var_hor, 0);
                make_constraint_value_constant(start_offset_hor, 0);
            }
        }
            break;
        case ALIGN::SPACE_AROUND: {
            if (widget.style.display_dir == DISPLAY_DIR::HORIZONTAL) {
                make_constraint_value_constant(space_var_hor, (widget.content_width - content_size.x) / (num_children + 1));
                make_constraint_value_constant(start_offset_hor, 0);
            } else {
                make_constraint_value_constant(space_var_hor, 0);
                make_constraint_value_constant(start_offset_hor, 0);
            }
        }
            break;
        case ALIGN::CENTER: {
            make_constraint_value_constant(space_var_hor, widget.style.content_spacing);
            if (widget.style.display_dir == DISPLAY_DIR::HORIZONTAL) {
                float remaining_space = 0.f;
                remaining_space = widget.content_width - content_size.x - (widget.style.content_spacing * (num_children - 1));
                float offset = remaining_space / 2.f;
                make_constraint_value_constant(start_offset_hor, offset);
            } else {
                make_constraint_value_constant(start_offset_hor, 0);
            }
        }
            break;
        case ALIGN::START: {
            make_constraint_value_constant(space_var_hor, widget.style.content_spacing);
            make_constraint_value_constant(start_offset_hor, 0);
        }
            break;
        case ALIGN::END: {
            make_constraint_value_constant(space_var_hor, widget.style.content_spacing);
            float offset = 0.f;
            if (widget.style.display_dir == DISPLAY_DIR::HORIZONTAL) {
                offset = (widget.content_width - content_size.x) - (widget.style.content_spacing * (widget.children_widget_handles.size() + 1.f));
            } else {
                offset = widget.content_width - content_size.x;
            }
            make_constraint_value_constant(start_offset_hor, offset);
        }
            break;
        default: {
            make_constraint_value_constant(space_var_hor, 0);
            make_constraint_value_constant(start_offset_hor, 0);
        }
    }
    
    // because pivot is top left but screen coordinate's pivot is bottom left, so y must be negated
    switch (widget.style.vertical_align_val) {
        case ALIGN::SPACE_BETWEEN: {
            if (widget.style.display_dir == DISPLAY_DIR::VERTICAL) {
                if (num_children == 1) {
                    make_constraint_value_constant(space_var_vert, -(widget.content_height - content_size.y) / 2);
                } else {
                    make_constraint_value_constant(space_var_vert, -(widget.content_height - content_size.y) / (num_children - 1));
                }
                make_constraint_value_constant(start_offset_vert, 0);
            } else {
                make_constraint_value_constant(space_var_vert, 0);
                make_constraint_value_constant(start_offset_vert, 0);
            }
        }
            break;
        case ALIGN::SPACE_AROUND: {
            if (widget.style.display_dir == DISPLAY_DIR::VERTICAL) {
                make_constraint_value_constant(space_var_vert, -(widget.content_height - content_size.y) / (num_children + 1));
                make_constraint_value_constant(start_offset_vert, 0);
            } else {
                make_constraint_value_constant(space_var_vert, 0);
                make_constraint_value_constant(start_offset_vert, 0);
            }
        }
            break;
        case ALIGN::CENTER: {
            make_constraint_value_constant(space_var_vert, -widget.style.content_spacing);
            if (widget.style.display_dir == DISPLAY_DIR::VERTICAL) {
                float remaining_space = remaining_space = widget.content_height - content_size.y - (widget.style.content_spacing * (num_children - 1));
                float offset = remaining_space / 2.f;
                make_constraint_value_constant(start_offset_vert, -offset);
            } else {
                make_constraint_value_constant(start_offset_vert, 0);
            }
        }
            break;
        case ALIGN::START: {
            make_constraint_value_constant(space_var_vert, -widget.style.content_spacing);
            make_constraint_value_constant(start_offset_vert, 0);
        }
            break;
        case ALIGN::END: {
            make_constraint_value_constant(space_var_vert, -widget.style.content_spacing);
            float space_on_top = 0.f;
            if (widget.style.display_dir == DISPLAY_DIR::VERTICAL) {
                space_on_top = (widget.content_height - content_size.y) - (widget.style.content_spacing * (widget.children_widget_handles.size() + 1.f));
            } else {
                space_on_top = widget.content_height - content_size.y;
            }
            make_constraint_value_constant(start_offset_vert, -space_on_top);
        }
            break;
        default: {
            make_constraint_value_constant(space_var_vert, 0);
            make_constraint_value_constant(start_offset_vert, 0);
        }
    }

    resolve_constraints();

    helper_info_t helper_info;
    helper_info.content_width = widget.content_width;
    helper_info.content_height = widget.content_height;

    return helper_info;
}

helper_info_t resolve_dimensions(int cur_widget_handle, int parent_width_handle, int parent_height_handle) {
    auto& cur_arr = *curframe_widget_arr;
    widget_t& widget = cur_arr[cur_widget_handle];
    if (widget.text_based) {
        text_dim_t text_dim = get_text_dimensions(widget.text_info.text, widget.text_info.font_size);
        widget.style.width = text_dim.width;
        // widget.height = text_dim.height;
        widget.style.height = text_dim.max_height_above_baseline;

        widget.render_width = widget.style.width + (widget.style.padding.x * 2);
        widget.render_height = widget.style.height + (widget.style.padding.y * 2);

        widget.content_width = widget.render_width + (widget.style.margin.x * 2);
        widget.content_height = widget.render_height + (widget.style.margin.y * 2);

        helper_info_t helper_info;
        helper_info.content_width = widget.content_width;
        helper_info.content_height = widget.content_height;
        return helper_info;
    }

    int widget_width_handle = create_constraint_var("width", &widget.render_width);
    int widget_height_handle = create_constraint_var("height", &widget.render_height); 

    if (widget.style.widget_size_width == WIDGET_SIZE::PIXEL_BASED) {
        game_assert(widget.style.width >= 0.f);
        float render_width = 0.f;
        if (parent_width_handle == -1 || parent_height_handle == -1) {
            render_width = widget.style.width;
        } else {
            render_width = widget.style.width + (widget.style.padding.x * 2);
        }

        make_constraint_value_constant(widget_width_handle, render_width);
    } else if (widget.style.widget_size_width == WIDGET_SIZE::PARENT_PERCENT_BASED) {
        game_assert(widget.style.width <= 1.f);
        game_assert(widget.style.width >= 0.f);

        std::vector<constraint_term_t> width_terms;
        width_terms.push_back(create_constraint_term(parent_width_handle, widget.style.width));
        create_constraint(widget_width_handle, width_terms, widget.style.padding.x * 2);
    }
    
    if (widget.style.widget_size_height == WIDGET_SIZE::PIXEL_BASED) {
        game_assert(widget.style.height >= 0.f);
        float render_height = 0.f;
        if (parent_width_handle == -1 || parent_height_handle == -1) {
            render_height = widget.style.height;
        } else {
            render_height = widget.style.height + (widget.style.padding.y * 2);
        }
        make_constraint_value_constant(widget_height_handle, render_height);
    } else if (widget.style.widget_size_height == WIDGET_SIZE::PARENT_PERCENT_BASED) {
        game_assert(widget.style.height <= 1.f);
        game_assert(widget.style.height >= 0.f);

        std::vector<constraint_term_t> height_terms;
        height_terms.push_back(create_constraint_term(parent_height_handle, widget.style.height));
        create_constraint(widget_height_handle, height_terms, widget.style.padding.y * 2);
    }

    glm::vec2 content_size(0);

    for (int child_widget_handle : widget.children_widget_handles) {
        widget_t& child_widget = cur_arr[child_widget_handle]; 

        helper_info_t child_helper_info = resolve_dimensions(child_widget_handle, widget_width_handle, widget_height_handle);
    
        if (widget.style.display_dir == DISPLAY_DIR::HORIZONTAL) {
            content_size.x += child_helper_info.content_width;
            content_size.y = fmax(content_size.y, child_helper_info.content_height);
        } else {
            content_size.y += child_helper_info.content_height;
            content_size.x = fmax(content_size.x, child_helper_info.content_width); 
        }
    }

    if (widget.style.display_dir == DISPLAY_DIR::HORIZONTAL) {
        content_size.x += widget.style.content_spacing * (widget.children_widget_handles.size() + 1);
    } else {
        content_size.y += widget.style.content_spacing * (widget.children_widget_handles.size() + 1);
    }

    if (parent_width_handle == -1 || parent_height_handle == -1) {
        helper_info_t helper_info;
        helper_info.content_width = widget.content_width;
        helper_info.content_height = widget.content_height;
        return helper_info;
    }

    game_assert(parent_width_handle != -1);
    game_assert(parent_height_handle != -1);
        
    if (widget.style.widget_size_width == WIDGET_SIZE::FIT_CONTENT) {
        float render_width = content_size.x + (widget.style.padding.x * 2.f);
        make_constraint_value_constant(widget_width_handle, render_width);
    }
    
    if (widget.style.widget_size_height == WIDGET_SIZE::FIT_CONTENT) {
        float render_height = content_size.y + (widget.style.padding.y * 2);
        make_constraint_value_constant(widget_height_handle, render_height);
    }

    resolve_constraints();

    widget.content_width = widget.render_width + (widget.style.margin.x * 2);
    widget.content_height = widget.render_height + (widget.style.margin.y * 2);

    helper_info_t helper_info;
    helper_info.content_width = widget.content_width;
    helper_info.content_height = widget.content_height;

    return helper_info;    
}

/*

<!-- <container id="top_part" width="0.9" height="60" widget_size_width="parent" widget_size_height="pixel" ver_align="center">
                <button background_color="220,75,0" hover_background_color="180,115,0"
                padding="15,10" border_radius="10">Back</button>
            </container> -->



*/

void autolayout_hierarchy() {

    game_assert_msg(curframe_widget_stack->size() == 0, "widgets stack for the current frame is not empty when laying out elements");
    game_assert_msg(styles_stack.size() == 1, "styles stack has more than 1 style");

    auto& cur_arr = *curframe_widget_arr;
    for (int i = 0; i < cur_arr.size(); i++) {
        widget_t& cur_widget = cur_arr[i];
        if (cur_widget.parent_widget_handle != -1 || cur_widget.absolute) continue;
        resolve_dimensions(cur_widget.handle, -1, -1);
    }

    for (int i = 0; i < cur_arr.size(); i++) {
        widget_t& cur_widget = cur_arr[i];
        if (cur_widget.parent_widget_handle != -1 || cur_widget.absolute) continue;
        cur_widget.x = 0 + cur_widget.style.translate.x;
        cur_widget.y = cur_widget.render_height + cur_widget.style.translate.y;
        int x_var = create_constraint_var_constant(cur_widget.x);
        int y_var = create_constraint_var_constant(cur_widget.y);
        resolve_positions(cur_widget.handle, x_var, y_var);
    }

    resolve_constraints();
}

int create_constraint_var(const char* var_name, float* val) {
    constraint_var_t constraint_value;
    constraint_value.handle = constraint_running_cnt++;
    memcpy(constraint_value.name, var_name, strlen(constraint_value.name));
    constraint_value.constant = false;
    constraint_value.value = val;
    constraint_value.cur_val = 0;
    constraint_vars[constraint_value.handle] = constraint_value;
    return constraint_value.handle;
}

int create_constraint_var_constant(float value) {
    constraint_var_t constraint_value;
    constraint_value.handle = constraint_running_cnt++;
    constraint_value.constant = true;
    constraint_value.cur_val = value;
    constraint_vars[constraint_value.handle] = constraint_value;
    return constraint_value.handle;
}

constraint_term_t create_constraint_term(int var_handle, float coefficient) {
    constraint_term_t term;
    term.coefficient = coefficient;
    term.var_handle = var_handle;
    return term;
}

void make_constraint_value_constant(int constraint_handle, float value) {
    constraint_var_t& constraint_var = constraint_vars[constraint_handle];
    constraint_var.constant = true;
    constraint_var.cur_val = value;
    if (constraint_var.value) *constraint_var.value = value;
}

void create_constraint(int constraint_var_handle, std::vector<constraint_term_t>& right_side_terms, float constant) {
    constraint_t constraint;
    constraint.left_side_var_handle = constraint_var_handle;
    for (constraint_term_t& right_side : right_side_terms) {
        constraint.right_side.push_back(right_side);
    }
    constraint.constant_handles.push_back(create_constraint_var_constant(constant));
    constraints.push_back(constraint);
}

void resolve_constraints() {
    bool something_changed = false;
    do {
        something_changed = false;
        for (constraint_t& constraint : constraints) {
            constraint_var_t& left_value = constraint_vars[constraint.left_side_var_handle];
            if (left_value.constant) continue;
            float running_right_side = 0.f;
            bool constaint_resolved = true;
            for (constraint_term_t& term : constraint.right_side) {
                if (constraint_vars[term.var_handle].constant) {
                    running_right_side += constraint_vars[term.var_handle].cur_val * term.coefficient;
                } else {
                    constaint_resolved = false;
                    break;
                }
            } 
            if (constaint_resolved) {
                for (int const_handle : constraint.constant_handles) {
                    running_right_side += constraint_vars[const_handle].cur_val;
                }
                make_constraint_value_constant(constraint.left_side_var_handle, running_right_side);
                something_changed = true;
            }
        }
    } while (something_changed);
}

void move_ui_anim_player_forward(ui_anim_player_t& player) {
    player.duration_cursor = fmin(player.anim_duration, player.duration_cursor + game::time_t::delta_time);
}

void move_ui_anim_player_backward(ui_anim_player_t& player) {
    player.duration_cursor = fmax(0, player.duration_cursor - game::time_t::delta_time);
}

style_t get_intermediate_style(style_t& original_style, ui_anim_player_t& player) {
    float anim_weight = player.duration_cursor / player.anim_duration;

    style_t new_style = original_style;
    ui_anim_t* anim = get_ui_anim(player.ui_anim_handle);
    game_assert_msg(anim, "anim not found");
    if (anim->style_params_overriden.background_color) {
#if 0
        if (anim_weight > 0) {
            new_style.background_color = glm::vec4(0,1,0,0.3f);
        }
#else
        if (original_style.background_color == TRANSPARENT_COLOR) {
            new_style.background_color = glm::vec4(anim->style.background_color.r, anim->style.background_color.g, anim->style.background_color.b, anim_weight * anim->style.background_color.a);
        } else {
            new_style.background_color = (anim_weight * anim->style.background_color) + (1 - anim_weight) * original_style.background_color;
        }
#endif
    }
    if (anim->style_params_overriden.color) {
        new_style.color = (anim_weight * anim->style.color) + (1 - anim_weight) * original_style.color;
    }
    if (anim->style_params_overriden.width) {
        new_style.width = (anim_weight * anim->style.width) + (1 - anim_weight) * original_style.width;
    }
    if (anim->style_params_overriden.height) {
        new_style.height = (anim_weight * anim->style.height) + (1 - anim_weight) * original_style.height;
    }
    if (anim->style_params_overriden.translate) {
        new_style.translate = (anim_weight * anim->style.translate) + (1 - anim_weight) * original_style.translate;
    }

    return new_style;
}

void render_ui_helper(widget_t& widget) {

    if (widget.attached_hover_anim_player_handle != -1) {
        ui_anim_player_t* hover_player = get_ui_anim_player(widget.attached_hover_anim_player_handle);
        game_assert_msg(hover_player, "hover player not found");
        if (widget.handle == cur_final_focused_handle) {
            move_ui_anim_player_forward(*hover_player);
        } else {
            move_ui_anim_player_backward(*hover_player);
        }
    }

    for (int i = 0; i < widget.attached_anim_player_handles.size(); i++) {
        ui_anim_player_t* attached_player = get_ui_anim_player(widget.attached_anim_player_handles[i]);
        game_assert_msg(attached_player, "attached player not found");
        if (attached_player->playing) {
            move_ui_anim_player_forward(*attached_player);
        } else {
            move_ui_anim_player_backward(*attached_player);
        }
    }

    if ((widget.style.bck_mode == BCK_SOLID && widget.style.background_color != TRANSPARENT_COLOR) || 
        widget.style.bck_mode == BCK_GRADIENT_TOP_LEFT_TO_BOTTOM_RIGHT ||
        widget.style.bck_mode == BCK_GRADIENT_4_CORNERS) {
        draw_background(widget);
    }

    if (widget.image_based) {
        draw_image_container(widget);
    } else if (widget.text_based) { 
        draw_text(widget.text_info.text, glm::vec2(widget.x + widget.style.padding.x + widget.style.margin.x, widget.y - widget.style.padding.y - widget.style.margin.y), widget.text_info.font_size, widget.style.color);
    } 

    auto& cur_arr = *curframe_widget_arr;
    for (int child_handle : widget.children_widget_handles) {
        render_ui_helper(cur_arr[child_handle]);
    }
}

bool set_parameter_in_style(style_t& style, const char* name, const char* content) {
    if (strcmp(name, "display_dir") == 0) {
        DISPLAY_DIR dir = DISPLAY_DIR::HORIZONTAL;
        if (strcmp(content, "vertical") == 0) {
            dir = DISPLAY_DIR::VERTICAL;
        } else if (strcmp(content, "horizontal") == 0) {
            dir = DISPLAY_DIR::HORIZONTAL;
        }
        style.display_dir = dir;
        return true;
    } else if (strcmp(name, "hor_align") == 0) {
        ALIGN hor_align = ALIGN::START;
        if (strcmp(content, "start") == 0) {
            hor_align = ALIGN::START;
        } else if (strcmp(content, "center") == 0) {
            hor_align = ALIGN::CENTER;
        } else if (strcmp(content, "end") == 0) {
            hor_align = ALIGN::END;
        } else if (strcmp(content, "space_around") == 0) {
            hor_align = ALIGN::SPACE_AROUND;
        } else if (strcmp(content, "space_between") == 0) {
            hor_align = ALIGN::SPACE_BETWEEN;
        }
        style.horizontal_align_val = hor_align;
        return true;
    } else if (strcmp(name, "ver_align") == 0) {
        ALIGN ver_align = ALIGN::START;
        if (strcmp(content, "start") == 0) {
            ver_align = ALIGN::START;
        } else if (strcmp(content, "center") == 0) {
            ver_align = ALIGN::CENTER;
        } else if (strcmp(content, "end") == 0) {
            ver_align = ALIGN::END;
        } else if (strcmp(content, "space_around") == 0) {
            ver_align = ALIGN::SPACE_AROUND;
        } else if (strcmp(content, "space_between") == 0) {
            ver_align = ALIGN::SPACE_BETWEEN;
        }
        style.vertical_align_val = ver_align;
        return true;
    } else if (strcmp(name, "padding") == 0) {
        glm::vec2 padding(0);
        sscanf(content, "%f,%f", &padding.x, &padding.y);
        style.padding = padding;
        return true;
    } else if (strcmp(name, "margin") == 0) {
        glm::vec2 margin(0);
        sscanf(content, "%f,%f", &margin.x, &margin.y);
        style.margin = margin;
        return true;
    } else if (strcmp(name, "content_spacing") == 0) {
        style.content_spacing = atof(content);
        return true;
    } else if (strcmp(name, "background_color") == 0) {
        glm::vec3 color(0);
        sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
        style.background_color = create_color(color.r, color.g, color.b, 1);
        return true;
    }  else if (strcmp(name, "color") == 0) {
        glm::vec3 color(0);
        sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
        style.color = create_color(color.r, color.g, color.b);
        return true;
    } else if (strcmp(name, "border_radius") == 0) {
        style.border_radius = atof(content);
        return true;
    } else if (strcmp(name, "tl_border_radius") == 0) {
        style.tl_border_radius = atof(content);
        return true;
    } else if (strcmp(name, "tr_border_radius") == 0) {
        style.tr_border_radius = atof(content);
        return true;
    } else if (strcmp(name, "bl_border_radius") == 0) {
        style.bl_border_radius = atof(content);
        return true;
    } else if (strcmp(name, "br_border_radius") == 0) {
        style.br_border_radius = atof(content);
        return true;
    } else if (strcmp(name, "border_radius_mode") == 0) {
        BORDER_RADIUS_MODE mode = BR_SINGLE_VALUE;
        if (strcmp(content, "single") == 0) {
            mode = BR_SINGLE_VALUE;
        } else if (strcmp(content, "4-corners") == 0) {
            mode = BR_4_CORNERS;
        }
        style.border_radius_mode = mode;
        return true;
    } else if (strcmp(name, "hover_background_color") == 0) {
        glm::vec3 color(0);
        sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
        style.hover_background_color = create_color(color.r, color.g, color.b);
        return true;
    } else if (strcmp(name, "hover_color") == 0) {
        glm::vec3 color(0);
        sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
        style.hover_color = create_color(color.r, color.g, color.b);
        return true;
    } else if (strcmp(name, "bck_mode") == 0) {
        BCK_MODE bck_mode = BCK_SOLID;
        if (strcmp(content, "solid") == 0) {
            bck_mode = BCK_SOLID;
        } else if (strcmp(content, "gradient-tl-br") == 0) {
            bck_mode = BCK_GRADIENT_TOP_LEFT_TO_BOTTOM_RIGHT;
        } else if (strcmp(content, "gradient") == 0) {
            bck_mode = BCK_GRADIENT_4_CORNERS;
        }
        style.bck_mode = bck_mode;
        return true;
    } else if (strcmp(name, "tl_bck") == 0) {
        glm::vec3 color(0);
        sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
        style.top_left_bck_color = create_color(color.r, color.g, color.b, 1);
        return true;
    } else if (strcmp(name, "br_bck") == 0) {
        glm::vec3 color(0);
        sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
        style.bottom_right_bck_color = create_color(color.r, color.g, color.b, 1);
        return true;
    } else if (strcmp(name, "bl_bck") == 0) {
        glm::vec3 color(0);
        sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
        style.bottom_left_bck_color = create_color(color.r, color.g, color.b, 1);
        return true;
    } else if (strcmp(name, "tr_bck") == 0) {
        glm::vec3 color(0);
        sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
        style.top_right_bck_color = create_color(color.r, color.g, color.b, 1);
        return true;
    } else if (strcmp(name, "width") == 0) {
        style.width = atof(content);
    } else if (strcmp(name, "height") == 0) {
        style.height = atof(content);
    } else if (strcmp(name, "widget_size_width") == 0) {
        WIDGET_SIZE size = WIDGET_SIZE::PIXEL_BASED;
        if (strcmp(content, "pixel") == 0) {
            size = WIDGET_SIZE::PIXEL_BASED;
        } else if (strcmp(content, "parent") == 0) {
            size = WIDGET_SIZE::PARENT_PERCENT_BASED;
        } else if (strcmp(content, "fit") == 0) {
            size = WIDGET_SIZE::FIT_CONTENT;
        }
        style.widget_size_width = size;
    } else if (strcmp(name, "widget_size_height") == 0) {
        WIDGET_SIZE size = WIDGET_SIZE::PIXEL_BASED;
        if (strcmp(content, "pixel") == 0) {
            size = WIDGET_SIZE::PIXEL_BASED;
        } else if (strcmp(content, "parent") == 0) {
            size = WIDGET_SIZE::PARENT_PERCENT_BASED;
        } else if (strcmp(content, "fit") == 0) {
            size = WIDGET_SIZE::FIT_CONTENT;
        }
        style.widget_size_height = size;
    } else if (strcmp(name, "translate") == 0) {
        sscanf(content, "%f,%f", &style.translate.x, &style.translate.y);
    }


    return false;
}

parsed_ui_attributes_t get_style_and_key(xml_attribute** attributes) {
    parsed_ui_attributes_t ui_attrs;
    style_t& style = ui_attrs.style;
    size_t num_attrs = get_zero_terminated_array_attributes(attributes);
    for (size_t i = 0; i < num_attrs; i++) {
        xml_attribute* attr = attributes[i];
        char* name = xml_get_zero_terminated_buffer(attr->name);
        char* content = xml_get_zero_terminated_buffer(attr->content);
        if (set_parameter_in_style(style, name, content)) {

        } else if (strcmp(name, "id") == 0) {
            memcpy(ui_attrs.id, content, fmin(strlen(content), 64));
        } else if (strcmp(name, "image_src") == 0) {
            char buffer[256]{};
            get_resources_folder_path(buffer);
            sprintf(ui_attrs.image_path, "%s\\%s\\%s", buffer, UI_FOLDER, content);
        } else if (strcmp(name, "font_size") == 0) {
            int font_size = 0;
            sscanf(content, "%i", &ui_attrs.font_size);
        } else if (strcmp(name, "clickable") == 0) {
            ui_attrs.ui_properties = ui_attrs.ui_properties | UI_PROP_CLICKABLE;
        } else if (strcmp(name, "z") == 0) {
            ui_attrs.z = atoi(content);
        }
        free(name);
        free(content);
    }
    return ui_attrs;
}

void set_ui_value(std::string& key, std::string& val) {
    ui_text_values[key] = val;
}

void draw_from_ui_file_layout_helper(xml_node* node) {
    xml_string* element_name = node->name;
    char* zero_terminated_element_name = xml_get_zero_terminated_buffer(element_name);
    
    parsed_ui_attributes_t attrs = get_style_and_key(node->attributes);
    push_style(attrs.style);
    if (strcmp(zero_terminated_element_name, "panel") == 0) {
        game_assert_msg(attrs.id[0] != '\0', "panel not given a name through id attribute");
        create_panel(attrs.id, attrs.z);
    } else if (strcmp(zero_terminated_element_name, "container") == 0) {
        if (attrs.id[0] == '\0') {
            game_error_log("container not given a name through id attribute");
            // make memory location of node the id
            sprintf(attrs.id, "%i\n", node);
        }
        create_container(attrs.style.width, attrs.style.height, attrs.style.widget_size_width, attrs.style.widget_size_height, attrs.id, false, 0, attrs.ui_properties);
    }  else if (strcmp(zero_terminated_element_name, "text") == 0 || strcmp(zero_terminated_element_name, "button") == 0) {
        bool is_text_element = strcmp(zero_terminated_element_name, "text") == 0;
        xml_string* content = node->content;
        char* zero_terminated_content = xml_get_zero_terminated_buffer(content);
        char* first_curly_braces = strstr(zero_terminated_content, "{{{");
        if (first_curly_braces != NULL) {
            *first_curly_braces = 0;
            char* key = first_curly_braces + 3;
            char* ending_curly_braces = strstr(key, "}}}");
            *ending_curly_braces = 0;
            char* remaining_str = ending_curly_braces + 3;
            
            char* final_str = (char*)calloc(128, sizeof(char));
            std::string key_string = key;
            std::string val = ui_text_values[key_string];
            if (attrs.id[0] != '\0') {
                sprintf(final_str, "%s%s%s###%s", zero_terminated_content, val.c_str(), remaining_str, attrs.id);
            } else {
                sprintf(final_str, "%s%s%s", zero_terminated_content, val.c_str(), remaining_str);
            }
            
            if (is_text_element) {
                create_text(final_str, attrs.font_size, false);
            } else {
                create_button(final_str, attrs.font_size, -1);
            }

            free(final_str);
        } else {
            const char* text_input = zero_terminated_content;
            char buffer[256]{};
            if (attrs.id[0] != '\0') {
                sprintf(buffer, "%s###%s", zero_terminated_content, attrs.id);
                text_input = buffer;
            }

            if (is_text_element) {
                create_text(text_input, attrs.font_size, false);
            } else {
                create_button(text_input, attrs.font_size, -1);
            }
        }
        free(zero_terminated_content);
    } else if (strcmp(zero_terminated_element_name, "image") == 0) {
        int texture_handle = create_texture(attrs.image_path, 1);
        create_image_container(texture_handle, attrs.style.width, attrs.style.height, attrs.style.widget_size_width, attrs.style.widget_size_height, 1 + strrchr(attrs.image_path, '\\'));
    }
    pop_style();

    size_t num_children = get_zero_terminated_array_nodes(node->children); 
    for (size_t i = 0; i < num_children; i++) {
        xml_node* child = node->children[i];
        draw_from_ui_file_layout_helper(child);
    }

    if (strcmp(zero_terminated_element_name, "panel") == 0) {
        end_panel();
    } else if (strcmp(zero_terminated_element_name, "container") == 0) {
        end_container();
    }
    
    free(zero_terminated_element_name);
}

void draw_from_ui_file_layouts() {
    // for (ui_file_layout_t& ui_layout : ui_files) {
    for (int ui_layout_handle : active_ui_file_handles) {
        for (ui_file_layout_t& layout : ui_files) {
            if (layout.handle == ui_layout_handle) {
                xml_node* root = layout.document->root;
                draw_from_ui_file_layout_helper(root);
            }
        }
    }
}

void render_ui() {  

    end_imgui();
	autolayout_hierarchy();

    if (ui_will_update) {
        cur_focused_internal_handle = -1;
        cur_final_focused_handle = -1;
        stack_nav_cur = false;
    }

    auto& cur_arr = *curframe_widget_arr;
    for (widget_t& widget : cur_arr) {
        if (widget.parent_widget_handle == -1) {
            render_ui_helper(widget);
        }
    }

    ui_will_update = false;
    for (int i = 0; i < font_modes.size(); i++) {
        if (!font_modes[i].used_last_frame) {
            for (unsigned char c = 0; c < 128; c++) {
                font_char_t& char_data = font_modes[i].chars[c];
                delete_texture(char_data.texture_handle);
            }
            font_modes[i].chars.clear();
            font_modes.erase(font_modes.begin() + i);
            i--;
        }
    }
}

void load_font(int font_size) {

    for (font_mode_t& font_mode : font_modes) {
        if (font_mode.font_size == font_size) return;
    }

    font_mode_t font_mode;
    std::unordered_map<unsigned char, font_char_t>& chars = font_mode.chars;
    font_mode.font_size = font_size;

    FT_Library lib;
    if (FT_Init_FreeType(&lib)) {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return;
    }
    FT_Face face;

    char resource_path[256]{};
    get_resources_folder_path(resource_path);
    char font_path[256]{};
    // sprintf(font_path, "%s\\%s\\Courier_Prime\\CourierPrime-Regular.ttf", resource_path, FONTS_FOLDER);
    sprintf(font_path, "%s\\%s\\Prosto_One\\ProstoOne-Regular.ttf", resource_path, FONTS_FOLDER);

    if (FT_New_Face(lib, font_path, 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;  
        return;
    }
    FT_Set_Pixel_Sizes(face, 0, font_mode.font_size);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            return;
        }

        font_char_t font_char;
        font_char.c = c; 
        if (c == ' ') {
            font_char.width = font_mode.font_size * 0.35f;
            font_char.advance = font_char.width;
            font_char.height = 20;
            font_char.bearing.x = 5;
            font_char.bearing.y = 5;
            font_char.texture_handle = -1;
        } else {
            font_char.width = face->glyph->bitmap.width;
            font_char.advance = font_char.width * 1.025f;
            if (c == '.') {
                font_char.advance = font_char.width * 2.f;
            }
            font_char.height = face->glyph->bitmap.rows;
            font_char.bearing.x = face->glyph->bitmap_left;
            font_char.bearing.y = face->glyph->bitmap_top;
            font_char.texture_handle = create_texture(face->glyph->bitmap.buffer, font_char.width, font_char.height, 0);
        }

        chars[c] = font_char;
    }

    font_modes.push_back(font_mode);
}

void init_ui() {
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	
	render_object_data& data = font_char_t::ui_opengl_data;
	data.vao = create_vao();
	data.vbo = create_dyn_vbo(4 * sizeof(vertex_t));
	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3
	};

    // set up ebo with indicies
	data.ebo = create_ebo(indices, sizeof(indices));

	// bind_vao(data.vao);
	vao_enable_attribute(data.vao, data.vbo, 0, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, position));
	vao_enable_attribute(data.vao, data.vbo, 1, 4, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
	vao_enable_attribute(data.vao, data.vbo, 2, 2, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex_coord));
    vao_bind_ebo(data.vao, data.ebo);
	// bind_ebo(data.ebo);
	// unbind_vao();
	// unbind_ebo();

	data.shader = create_shader("text.vert", "text.frag");
	glm::mat4 projection = glm::ortho(0.0f, globals.window.window_width, 0.0f, globals.window.window_height);
	shader_set_mat4(data.shader, "projection", projection);
	shader_set_int(data.shader, "character_tex", 0);
	shader_set_int(data.shader, "image_tex", 1);

    if (detect_gfx_error()) {
		std::cout << "error loading the ui data" << std::endl;
	} else {
		std::cout << "successfully init ui data" << std::endl;
    }

    char ui_folder[256]{};
    char buffer[256]{};
    get_resources_folder_path(buffer);
    sprintf(ui_folder, "%s\\%s", buffer, UI_FOLDER);

    for (const auto& entry : fs::directory_iterator(ui_folder)) {
        auto& path = entry.path();
        std::string path_string = entry.path().string();
        const char* path_char = path_string.c_str();
        if (strcmp(get_file_extension(path_char), "xml") == 0) {
            static int cnt = 0;
            FILE* ui_file_c = fopen(path_char, "r");
            game_assert_msg(ui_file_c, "play file not found");
            ui_file_layout_t ui_file;
            ui_file.document = xml_open_document(ui_file_c);

            struct stat ui_file_stat;
            if (stat(path_char, &ui_file_stat) < 0) return;
            ui_file.last_modified_time = ui_file_stat.st_mtime;
            memcpy(ui_file.path, path_char, strlen(path_char));

            ui_file.handle = cnt++;

            ui_files.push_back(ui_file);
        } else if (strcmp(get_file_extension(path_char), "json") == 0) {
            parse_ui_anims(path_char);
        }
    } 

    // for (const auto& entry : fs::directory_iterator(ui_folder)) {
    //     auto& xml = entry.path();
    //     std::string xml_string = entry.path().string();
    //     const char* xml_path = xml_string.c_str();
    //     if (strcmp(get_file_extension(xml_path), "xml") != 0) continue;
    //     static int cnt = 0;
    //     FILE* ui_file_c = fopen(xml_path, "r");
    //     game_assert_msg(ui_file_c, "play file not found");
    //     ui_file_layout_t ui_file;
    //     ui_file.document = xml_open_document(ui_file_c);

    //     struct stat ui_file_stat;
    //     if (stat(xml_path, &ui_file_stat) < 0) return;
    //     ui_file.last_modified_time = ui_file_stat.st_mtime;
    //     memcpy(ui_file.path, xml_path, strlen(xml_path));

    //     ui_file.handle = cnt++;

    //     ui_files.push_back(ui_file);
    // }

}

text_dim_t get_text_dimensions(const char* text, int font_size) {
	text_dim_t running_dim;
    for (font_mode_t& font_mode : font_modes) {
        if (font_mode.font_size == font_size) {
            for (int i = 0; i < strlen(text); i++) {
                unsigned char c = text[i];	
                font_char_t& fc = font_mode.chars[c];
                running_dim.width += fc.advance;
                running_dim.max_height_above_baseline = fmax(fc.bearing.y, running_dim.max_height_above_baseline);
                running_dim.max_height_below_baseline = fmax(fc.height - fc.bearing.y, running_dim.max_height_below_baseline);
            }
        }
    }
    // running_dim.height = running_dim.max_height_above_baseline + running_dim.max_height_below_baseline;
    // running_dim.height = running_dim.max_height_above_baseline;
	return running_dim;
}

void draw_background(widget_t& widget) {
	shader_set_float(font_char_t::ui_opengl_data.shader, "tex_influence", 0.f);
	shader_set_int(font_char_t::ui_opengl_data.shader, "round_vertices", 1);

    if (widget.style.border_radius_mode == BR_SINGLE_VALUE) {
        shader_set_float(font_char_t::ui_opengl_data.shader, "tl_border_radius", widget.style.border_radius);
        shader_set_float(font_char_t::ui_opengl_data.shader, "tr_border_radius", widget.style.border_radius);
        shader_set_float(font_char_t::ui_opengl_data.shader, "bl_border_radius", widget.style.border_radius);
        shader_set_float(font_char_t::ui_opengl_data.shader, "br_border_radius", widget.style.border_radius);
    } else if (widget.style.border_radius_mode == BR_4_CORNERS) {
        shader_set_float(font_char_t::ui_opengl_data.shader, "tl_border_radius", widget.style.tl_border_radius);
        shader_set_float(font_char_t::ui_opengl_data.shader, "tr_border_radius", widget.style.tr_border_radius);
        shader_set_float(font_char_t::ui_opengl_data.shader, "bl_border_radius", widget.style.bl_border_radius);
        shader_set_float(font_char_t::ui_opengl_data.shader, "br_border_radius", widget.style.br_border_radius);
    }

    float x = widget.x + widget.style.margin.x;
    float y = widget.y - widget.style.margin.y;
    float width = widget.render_width;
    float height = widget.render_height;

    glm::vec4 top_left_color(0), top_right_color(0), bottom_left_color(0), bottom_right_color(0);
    if (widget.style.bck_mode == BCK_SOLID) {
        top_left_color = widget.style.background_color;
        top_right_color = widget.style.background_color;
        bottom_left_color = widget.style.background_color;
        bottom_right_color = widget.style.background_color;
        if (top_left_color.a == 0.3f) {
            glm::to_string(top_left_color);
        }
    } else if (widget.style.bck_mode == BCK_GRADIENT_TOP_LEFT_TO_BOTTOM_RIGHT) {
        top_left_color = widget.style.top_left_bck_color;
        bottom_right_color = widget.style.bottom_right_bck_color;
        bottom_left_color = (top_left_color + bottom_right_color) / glm::vec4(2.f);
        top_right_color = (top_left_color + bottom_right_color) / glm::vec4(2.f);
    } else if (widget.style.bck_mode == BCK_GRADIENT_4_CORNERS) {
        top_left_color = widget.style.top_left_bck_color;
        top_right_color = widget.style.top_right_bck_color;
        bottom_left_color = widget.style.bottom_left_bck_color;
        bottom_right_color = widget.style.bottom_right_bck_color;
    }

	glm::vec3 origin = glm::vec3(x, y, 0);

	vertex_t updated_vertices[4];
    updated_vertices[0] = create_vertex(origin + glm::vec3(width, 0, 0.0f), top_right_color, glm::vec2(0,0)); // top right
    updated_vertices[1] = create_vertex(origin + glm::vec3(width, -height, 0.0f), bottom_right_color, glm::vec2(0,0)); // bottom right
    updated_vertices[2] = create_vertex(origin + glm::vec3(0, -height, 0.0f), bottom_left_color, glm::vec2(0,0)); // bottom left
    updated_vertices[3] = create_vertex(origin, top_left_color, glm::vec2(0,0)); // top left

    update_vbo_data(font_char_t::ui_opengl_data.vbo, (float*)updated_vertices, sizeof(updated_vertices));

    shader_set_vec3(font_char_t::ui_opengl_data.shader, "top_left", updated_vertices[3].position);
    shader_set_vec3(font_char_t::ui_opengl_data.shader, "top_right", updated_vertices[0].position);
    shader_set_vec3(font_char_t::ui_opengl_data.shader, "bottom_left", updated_vertices[2].position);
    shader_set_vec3(font_char_t::ui_opengl_data.shader, "bottom_right", updated_vertices[1].position);

    set_fill_mode();
    // draw the rectangle render after setting all shader parameters
    draw_obj(font_char_t::ui_opengl_data);
}

void draw_text(const char* text, glm::vec2 starting_pos, int font_size, glm::vec3& color) {
	// shader_set_vec3(font_char_t::ui_opengl_data.shader, "color", color);
	shader_set_float(font_char_t::ui_opengl_data.shader, "tex_influence", 1.f);
	shader_set_int(font_char_t::ui_opengl_data.shader, "is_character_tex", 1);
	shader_set_int(font_char_t::ui_opengl_data.shader, "round_vertices", 0);
    text_dim_t text_dim = get_text_dimensions(text, font_size);
	glm::vec2 origin = starting_pos - glm::vec2(0, text_dim.max_height_above_baseline);
	vertex_t updated_vertices[4];

    load_font(font_size);

    glm::vec4 color_vec4 = glm::vec4(color.r, color.g, color.b, 1);
    for (font_mode_t& font_mode : font_modes) {
        if (font_mode.font_size == font_size) {
            font_mode.used_last_frame = true;
            for (int i = 0; i < strlen(text); i++) {
                unsigned char c = text[i];	
                font_char_t& fc = font_mode.chars[c];

                if (fc.texture_handle != -1) {
                    bind_texture(fc.texture_handle, true);
                    glm::vec2 running_pos;
                    running_pos.x = origin.x + fc.bearing.x + (fc.width / 2);
                    running_pos.y = origin.y + fc.bearing.y - (fc.height / 2);

                    updated_vertices[0] = create_vertex(glm::vec3(running_pos.x + (fc.width / 2), running_pos.y + (fc.height / 2), 0.0f), color_vec4, glm::vec2(1,0)); // top right
                    updated_vertices[1] = create_vertex(glm::vec3(running_pos.x + (fc.width / 2), running_pos.y - (fc.height / 2), 0.0f), color_vec4, glm::vec2(1,1)); // bottom right
                    updated_vertices[2] = create_vertex(glm::vec3(running_pos.x - (fc.width / 2), running_pos.y - (fc.height / 2), 0.0f), color_vec4, glm::vec2(0,1)); // bottom left
                    updated_vertices[3] = create_vertex(glm::vec3(running_pos.x - (fc.width / 2), running_pos.y + (fc.height / 2), 0.0f), color_vec4, glm::vec2(0,0)); // top left
                    update_vbo_data(font_char_t::ui_opengl_data.vbo, (float*)updated_vertices, sizeof(updated_vertices));

                    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                    set_fill_mode();
                    // draw the rectangle render after setting all shader parameters
                    draw_obj(font_char_t::ui_opengl_data);
                }
                origin.x += fc.advance;
            }
        }
    }
}

void draw_image_container(widget_t& widget) {
	shader_set_float(font_char_t::ui_opengl_data.shader, "tex_influence", 1.f);
	shader_set_int(font_char_t::ui_opengl_data.shader, "is_character_tex", 0);
	shader_set_int(font_char_t::ui_opengl_data.shader, "round_vertices", 0);
	vertex_t updated_vertices[4];

    bind_texture(widget.texture_handle, true);

    glm::vec2 top_left(widget.x, widget.y);

    updated_vertices[0] = create_vertex(glm::vec3(top_left.x + widget.render_width, top_left.y, 0.0f), glm::vec4(0,1,1,1), glm::vec2(1,1)); // top right
    updated_vertices[1] = create_vertex(glm::vec3(top_left.x + widget.render_width, top_left.y - widget.render_height, 0.f), glm::vec4(0,0,1,1), glm::vec2(1,0)); // bottom right
    updated_vertices[2] = create_vertex(glm::vec3(top_left.x, top_left.y - widget.render_height, 0.0f), glm::vec4(0,1,0,1), glm::vec2(0,0)); // bottom left
    updated_vertices[3] = create_vertex(glm::vec3(top_left.x, top_left.y, 0.0f), glm::vec4(1,0,0,1), glm::vec2(0,1)); // top left
    update_vbo_data(font_char_t::ui_opengl_data.vbo, (float*)updated_vertices, sizeof(updated_vertices));

    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    set_fill_mode();
    // draw the rectangle render after setting all shader parameters
    draw_obj(font_char_t::ui_opengl_data);
}

bool get_if_key_clicked_on(const char* key) {
    return strcmp(prevframe_ui_element_status->clicked_on.widget_key.c_str(), key) == 0;
}

bool get_if_key_hovered_over(const char* key) {
    return strcmp(prevframe_ui_element_status->hovered_over.widget_key.c_str(), key) == 0;
}

bool get_if_key_mouse_enter(const char* key) {
    return strcmp(prevframe_ui_element_status->mouse_enter.widget_key.c_str(), key) == 0;
}

bool get_if_key_mouse_left(const char* key) {
    return strcmp(prevframe_ui_element_status->mouse_left.widget_key.c_str(), key) == 0;
}

bool is_some_element_clicked_on() {
    for (int i = 0; i < prevframe_widget_arr->size(); i++) {
        widget_t& widget = (*prevframe_widget_arr)[i];
        if (widget.properties & UI_PROP_CLICKABLE) {
            auto& prev_arr = *prevframe_widget_arr;
            widget_t& cached_widget = prev_arr[widget.handle];

            user_input_t& input_state = globals.window.user_input;
            bool mouse_over_widget = input_state.mouse_x >= (cached_widget.x + cached_widget.style.margin.x) &&
                    input_state.mouse_x <= (cached_widget.x + cached_widget.render_width + cached_widget.style.margin.x) &&
                    // render x and render y specified as the top left pivot and y in ui is 0 on the
                    // bottom and WINDOW_HEIGHT on the top, so cached_widget.y is the top y of the 
                    // widget and cached_widget.y - cached_widget.render_height is the bottom y 
                    // of the widget
                    input_state.mouse_y <= (cached_widget.y - cached_widget.style.margin.y) &&
                    input_state.mouse_y >= (cached_widget.y - cached_widget.render_height - cached_widget.style.margin.y);

            if (mouse_over_widget && !input_state.game_controller && globals.window.user_input.left_mouse_release) {
                return true;
            }
        }
    }
    return false;
}

void clear_active_ui_files() {
    active_ui_file_handles.clear();
}

void add_active_ui_file(const char* file_name) {
    for (ui_file_layout_t& file_layout : ui_files) {
        const char* ui_file_file_name = strrchr(file_layout.path, '\\') + 1;
        if (strcmp(ui_file_file_name, file_name) == 0) {
            active_ui_file_handles.push_back(file_layout.handle);
            return;
        }
    }
}

void add_active_ui_anim_file(const char* file_name) {
    for (ui_anim_file_t& file : ui_anim_files) {
        const char* ui_file_name = strrchr(file.path, '\\') + 1;
        if (strcmp(ui_file_name, file_name) == 0) {
            active_ui_anim_file_handles.push_back(file.handle);
            return;
        }
    }
}

void clear_active_ui_anim_files() {
    active_ui_anim_file_handles.clear();
    ui_anim_players.clear();
    for (int i = 0; i < prevframe_widget_arr->size(); i++) {
        (*prevframe_widget_arr)[i].attached_hover_anim_player_handle = -1;
    }
}

// void set_background_color_override(const char* widget_key, glm::vec3 color) {
//     bck_color_override_t ovrride;
//     memcpy(ovrride.widget_key, widget_key, strlen(widget_key));
//     ovrride.background_color = color;
//     ovrride.bck_mode = BCK_SOLID;
//     bck_color_overrides.push_back(ovrride);
// }

// void set_background_color_gradient_4_corners_override(const char* widget_key, glm::vec3 top_left_color, glm::vec3 bottom_right_color) {
//     bck_color_override_t ovrride;
//     memcpy(ovrride.widget_key, widget_key, strlen(widget_key));
//     ovrride.top_left_bck_color = top_left_color;
//     ovrride.bottom_right_bck_color = bottom_right_color;
// }

void parse_ui_anims(const char* path) {
    static int cnt = 0;

    ui_anim_file_t anim_file;
    anim_file.handle = cnt++;
    memcpy(anim_file.path, path, strlen(path));

    json_document_t* anims_json = json_read_document(path);
    int num_anims = anims_json->root_node->num_children;
    json_node_t** anims = anims_json->root_node->children;
    for (int i = 0; i < num_anims; i++) {
        static int ui_anim_cnt = 0;
        ui_anim_t ui_anim;
        ui_anim.handle = ui_anim_cnt++;
        ui_anim.ui_file_handle = anim_file.handle;

        json_node_t* anim_node = anims[i];
        memcpy(ui_anim.anim_name, anim_node->key->buffer, anim_node->key->length);

        style_override_t& ov = ui_anim.style_params_overriden;

        int num_anim_props = anim_node->num_children;
        json_node_t** anim_props = anim_node->children;
        for (int j = 0; j < num_anim_props; j++) {
            json_node_t* anim_prop = anim_props[j];
            const char* name = anim_prop->key->buffer;
            const char* content = anim_prop->value->buffer;
            if (!set_parameter_in_style(ui_anim.style, name, content)) {
                if (strcmp(name, "anim_time") == 0) {
                    ui_anim.anim_duration = atof(content);
                }
            }

            if (strcmp(name, "display_dir") == 0) {
                ov.display_dir = true;
            } else if (strcmp(name, "hor_align") == 0) {
                ov.horizontal_align_val = true;
            } else if (strcmp(name, "ver_align") == 0) {
                ov.vertical_align_val = true;
            } else if (strcmp(name, "padding") == 0) {
                ov.padding = true;
            } else if (strcmp(name, "margin") == 0) {
                ov.margin = true;
            } else if (strcmp(name, "content_spacing") == 0) {
                ov.content_spacing = true;
            } else if (strcmp(name, "background_color") == 0) {
                ov.background_color = true;
            }  else if (strcmp(name, "color") == 0) {
                ov.color = true;
            } else if (strcmp(name, "border_radius") == 0) {
                ov.border_radius = true;
            } else if (strcmp(name, "tl_border_radius") == 0) {
                ov.tl_border_radius = true;
            } else if (strcmp(name, "tr_border_radius") == 0) {
                ov.tr_border_radius = true;
            } else if (strcmp(name, "bl_border_radius") == 0) {
                ov.bl_border_radius = true;
            } else if (strcmp(name, "br_border_radius") == 0) {
                ov.br_border_radius = true;
            } else if (strcmp(name, "border_radius_mode") == 0) {
                ov.border_radius_mode = true;
            } else if (strcmp(name, "bck_mode") == 0) {
                ov.bck_mode = true;
            } else if (strcmp(name, "tl_bck") == 0) {
                ov.top_left_bck_color = true;
            } else if (strcmp(name, "br_bck") == 0) {
                ov.bottom_right_bck_color = true;
            } else if (strcmp(name, "bl_bck") == 0) {
                ov.bottom_left_bck_color = true;
            } else if (strcmp(name, "tr_bck") == 0) {
                ov.top_right_bck_color = true;
            } else if (strcmp(name, "width") == 0) {
                ov.width = true;
            } else if (strcmp(name, "height") == 0) {
                ov.height = true;
            } else if (strcmp(name, "translate") == 0) {
                ov.translate = true;
            }
        }

        ui_anims.push_back(ui_anim);
        anim_file.ui_anims.push_back(ui_anim.handle);
    }
    json_free_document(anims_json);

    ui_anim_files.push_back(anim_file);
}

int create_ui_anim_player(const char* widget_key, int ui_anim_file_handle, ui_anim_t& ui_anim, bool play_upon_initialize, time_count_t starting_cursor) {
    ui_anim_player_t anim_player;
    static int cnt = 0;
    anim_player.handle = cnt++;
    memcpy(anim_player.widget_key, widget_key, strlen(widget_key));
    anim_player.anim_duration = ui_anim.anim_duration;
    anim_player.ui_anim_handle = ui_anim.handle;
    anim_player.ui_anim_file_handle = ui_anim_file_handle;
    anim_player.duration_cursor = starting_cursor;
    anim_player.playing = play_upon_initialize;
    ui_anim_players.push_back(anim_player);
    return anim_player.handle;
}

void add_ui_anim_to_widget(const char* widget_key, const char* ui_anim_name, time_count_t start_anim_duration_cursor, bool start_playing) {
    ui_anim_user_info_t add;
    memcpy(add.widget_key, widget_key, strlen(widget_key));
    memcpy(add.ui_anim_name, ui_anim_name, strlen(ui_anim_name));
    add.start_anim_duration_cursor = start_anim_duration_cursor;
    add.start_playing = start_playing;
    anims_to_add_this_frame.push_back(add);
}

void play_ui_anim_player(const char* widget_key, const char* ui_anim_name) {
    ui_anim_user_info_t play;
    memcpy(play.widget_key, widget_key, strlen(widget_key));
    memcpy(play.ui_anim_name, ui_anim_name, strlen(ui_anim_name));
    anims_to_start_this_frame.push_back(play);
}

void stop_ui_anim_player(const char* widget_key, const char* ui_anim_name) {
    ui_anim_user_info_t stop;
    memcpy(stop.widget_key, widget_key, strlen(widget_key));
    memcpy(stop.ui_anim_name, ui_anim_name, strlen(ui_anim_name));
    anims_to_stop_this_frame.push_back(stop);
}

ui_anim_player_t* get_ui_anim_player(int handle) {
    for (int i = 0; i < ui_anim_players.size(); i++) {
        if (ui_anim_players[i].handle == handle) {
            return &ui_anim_players[i];
        }
    }
    return NULL;
}

ui_anim_t* get_ui_anim(int handle) {
    for (int i = 0; i < ui_anims.size(); i++) {
        if (ui_anims[i].handle == handle) {
            return &ui_anims[i];
        }
    }
    return NULL;
}

void set_translate_in_ui_anim(const char* anim_name, glm::vec2 translate) {
    for (int i = 0; i < ui_anims.size(); i++) {
        if (strcmp(ui_anims[i].anim_name, anim_name) == 0) {
            ui_anims[i].style.translate = translate;
            ui_anims[i].style_params_overriden.translate = true;
        }
    }
}

void clear_element_status(ui_element_status_t& status) {
    status.clicked_on.widget_key = "";
    status.clicked_on.z_pos = INT_MIN;
    status.hovered_over.widget_key = "";
    status.hovered_over.z_pos = INT_MIN;
    status.mouse_enter.widget_key = "";
    status.mouse_enter.z_pos = INT_MIN;
    status.mouse_left.widget_key = "";
    status.mouse_left.z_pos = INT_MIN;
}