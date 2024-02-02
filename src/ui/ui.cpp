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
#include "ui_layout.h"
#include "ui_elements.h"
#include "ui_rendering.h"

#include <glm/gtx/string_cast.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H  

#define TRAVERSE_UI_EXTRA_DEBUG 0

namespace fs = std::filesystem;

static bool panel_left_used = false;

extern globals_t globals;

int latest_z_pos;
int max_z_pos_visible;

static std::vector<ui_file_layout_t> ui_files;
static std::vector<int> active_ui_file_handles;

static std::vector<ui_anim_file_t> ui_anim_files;
static std::vector<ui_anim_t> ui_anims;
static std::vector<int> active_ui_anim_file_handles;

static std::vector<ui_anim_player_t> ui_anim_players;

static int cur_widget_count = 0;

static ui_info_t ui_info_buffer1;
static ui_info_t ui_info_buffer2;

ui_info_t* curframe_ui_info = &ui_info_buffer2;
ui_info_t* prevframe_ui_info = &ui_info_buffer1;
bool stacked_nav_widget_in_stack = false;

static ui_state_t ui_state;

std::vector<style_t> styles_stack;

static bool ui_will_update = false;
bool ui_positions_changed = false;

static std::unordered_map<int, hash_t> handle_hashes;

static std::unordered_map<std::string, std::string> ui_text_values;

std::vector<font_mode_t> font_modes;
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
    latest_z_pos = INT_MIN;
    max_z_pos_visible = INT_MIN;
#if UI_RELOADING
    update_ui_files();
#endif

    // if (!globals.window.user_input.game_controller) {
    //     cur_focused_internal_handle = -1;
    //     cur_final_focused_handle = -1;
    //     stack_nav_cur = false;
    // }

    glm::mat4 projection = glm::ortho(0.0f, globals.window.window_width, 0.0f, globals.window.window_height);
    shader_set_mat4(font_char_t::ui_opengl_data.shader, "projection", projection);
    ui_will_update = globals.window.resized;
    ui_positions_changed = false;

    for (int i = 0; i < font_modes.size(); i++) {
        font_modes[i].used_last_frame = false;
    }

    ui_text_values.clear();
    styles_stack.clear();
    style_t default_style;
    styles_stack.push_back(default_style);

    if (curframe_ui_info == &ui_info_buffer1) {
        curframe_ui_info = &ui_info_buffer2;
        prevframe_ui_info = &ui_info_buffer1;
    } else {
        curframe_ui_info = &ui_info_buffer1;
        prevframe_ui_info = &ui_info_buffer2;
    }
    
    curframe_ui_info->widgets_arr.clear();
    curframe_ui_info->widget_stack.clear();
    
    clear_element_status(curframe_ui_info->ui_element_status);
    if (is_controller_connected()) {
        curframe_ui_info->ui_element_status.hovered_over = prevframe_ui_info->ui_element_status.hovered_over;
    }

    cur_widget_count = 0;

    clear_constraints();
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

void register_absolute_widget(widget_t& widget, const char* key, bool push_onto_stack) {
    widget.handle = cur_widget_count++;
    widget.absolute = true;
    memcpy(widget.key, key, strlen(key));
    widget.hash = hash(key);

    auto& arr = curframe_ui_info->widgets_arr;
    auto& stack = curframe_ui_info->widget_stack;

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

void set_widget_as_hover(int widget_handle, std::string& key) {
    ui_element_status_t& cur_elem_status = curframe_ui_info->ui_element_status;
    ui_element_status_t& prev_elem_status = prevframe_ui_info->ui_element_status;

    cur_elem_status.hovered_over.widget_key = key;
    cur_elem_status.hovered_over.z_pos = latest_z_pos;
    cur_elem_status.hovered_over.widget_handle = widget_handle;

    printf("widget %s is hovered now\n", key.c_str());

    if (strcmp(prev_elem_status.hovered_over.widget_key.c_str(), key.c_str()) != 0) {
        printf("widget %s is mouse entered\n", key.c_str());
        cur_elem_status.mouse_enter.widget_key = key;
        cur_elem_status.mouse_enter.widget_handle = widget_handle;
    }
}

void set_widget_mouse_leave(int widget_handle, std::string& key) {
    ui_element_status_t& cur_elem_status = curframe_ui_info->ui_element_status;
    cur_elem_status.mouse_left.widget_key = key;
    cur_elem_status.mouse_left.widget_handle = widget_handle;
    printf("widget %s is mouse left\n", key.c_str());
}

widget_registration_info_t register_widget(widget_t& widget, const char* key, bool push_onto_stack) {

    widget_registration_info_t info;

    widget.handle = cur_widget_count++;
    widget.z_pos = latest_z_pos;
    memcpy(widget.key, key, strlen(key)); 
    hash_t new_hash = hash(key);

    auto& arr = curframe_ui_info->widgets_arr;
    auto& stack = curframe_ui_info->widget_stack;

    auto& prev_arr = prevframe_ui_info->widgets_arr; 

    if (prev_arr.size() <= widget.handle) {
        ui_will_update = true;
    } else {
        hash_t& prev_hash = prev_arr[widget.handle].hash;
        bool same_element = is_same_hash(new_hash, prev_hash);
        ui_will_update = ui_will_update || !same_element;
        if (same_element) {
            widget.index_to_prev_for_same_widget = widget.handle;
        }
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
            ui_anim_player_t* attached_player = get_ui_anim_player_by_idx(widget.attached_anim_player_handles[i]);
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
            ui_anim_player_t* attached_player = get_ui_anim_player_by_idx(widget.attached_anim_player_handles[i]);
            game_assert_msg(attached_player, "could not find anim player for the animation trying to be started");
            ui_anim_t* attached_player_anim = get_ui_anim(attached_player->ui_anim_handle);
            game_assert_msg(attached_player_anim, "could not find anim for attached player");
            if (strcmp(attached_player_anim->anim_name, stop_anim_user_info->ui_anim_name) == 0) {
                attached_player->playing = false;
            }
        }
    }   

    style_t original_style = widget.style;
    ui_anim_player_t* hover_anim_player = get_ui_anim_player_by_idx(widget.attached_hover_anim_player_handle);
    if (hover_anim_player && hover_anim_player->duration_cursor > 0) {
        widget.style = get_intermediate_style(original_style, *hover_anim_player);
    }

    for (int i = 0; i < widget.attached_anim_player_handles.size(); i++) {
        ui_anim_player_t* attached_player = get_ui_anim_player_by_idx(widget.attached_anim_player_handles[i]);
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
        auto& prev_arr = prevframe_ui_info->widgets_arr;
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

        ui_element_status_t& cur_elem_status = curframe_ui_info->ui_element_status;
        ui_element_status_t& prev_elem_status = prevframe_ui_info->ui_element_status;

        if (!is_controller_connected() && !mouse_over_widget && strcmp(prev_elem_status.hovered_over.widget_key.c_str(), key) == 0) {
            set_widget_mouse_leave(widget.handle, std::string(key));
        }

        if (mouse_over_widget && !is_controller_connected()) {

            if (latest_z_pos > cur_elem_status.hovered_over.z_pos) {
                set_widget_as_hover(widget.handle, std::string(key));
                info.hovering_over = true;
            }
        } 
    
        bool clicked_on_cur_element = (prev_elem_status.hovered_over.widget_handle == widget.handle && (get_released(LEFT_MOUSE) || (ui_state.ctrl_controlled && get_pressed(CONTROLLER_A))));
        if (clicked_on_cur_element) {
            cur_elem_status.clicked_on.widget_key = key;
            cur_elem_status.clicked_on.z_pos = latest_z_pos;
            cur_elem_status.clicked_on.widget_handle = widget.handle;
            info.clicked_on = true;
        }
    } 

    return info;
}

float quantize_angle(float angle, int num_quanta) {
    // float angle_per_quanta = 360.f / num_quanta;
    float angle_per_quanta = 2 * 3.141526f / num_quanta;
    angle += (angle_per_quanta / 2.f);
    int section = fmin(num_quanta - 1, floor(angle / angle_per_quanta));
    return section * angle_per_quanta;
}

bool partially_behind_widget(widget_t& widget, widget_t& check_against_widget) {

    glm::mat4 re_centering_matrix(1.0f);
    re_centering_matrix = glm::translate(re_centering_matrix, glm::vec3(-check_against_widget.x, -(check_against_widget.y - check_against_widget.render_height), 0));

    glm::mat4 four_corners(1.0f);
    four_corners[0] = glm::vec4(widget.x, widget.y, 0, 1);
    four_corners[1] = glm::vec4(widget.x, widget.y - widget.render_height, 0, 1);
    four_corners[2] = glm::vec4(widget.x + widget.render_width, widget.y, 0, 1);
    four_corners[3] = glm::vec4(widget.x + widget.render_width, widget.y - widget.render_height, 0, 1);

	glm::mat4 normalize = glm::ortho(0.0f, check_against_widget.render_width, 0.0f, check_against_widget.render_height);

    // glm::mat4 centered_corners = re_centering_matrix * four_corners;
    glm::mat4 centered_corners = re_centering_matrix * four_corners;
    glm::mat4 normalized_corners = normalize * centered_corners;

    for (int i = 0; i < 4; i++) {
        glm::vec4 corner = normalized_corners[i];
        if (corner.x >= -1 && corner.x <= 1 && corner.y >= -1 && corner.y <= 1) {
            return true;
        }
    }

    return false;
}

static bool widget_has_background(widget_t& widget) {
    return (widget.style.background_color != TRANSPARENT_COLOR && widget.style.bck_mode == BCK_SOLID) ||
        (widget.style.bck_mode == BCK_GRADIENT_4_CORNERS && widget.style.top_left_bck_color != TRANSPARENT_COLOR) ||
        (widget.style.bck_mode == BCK_GRADIENT_TOP_LEFT_TO_BOTTOM_RIGHT && widget.style.top_left_bck_color != TRANSPARENT_COLOR);
}

static bool hover_widget_partially_covered_helper(widget_t& hover_widget, widget_t& possible_cover_widget) {
    if (possible_cover_widget.text_based || possible_cover_widget.image_based || widget_has_background(possible_cover_widget)) {
        bool fully_behind = partially_behind_widget(hover_widget, possible_cover_widget);
        if (fully_behind) {
            return true;
        }
    }

    for (int i = 0; i < possible_cover_widget.children_widget_handles.size(); i++) {
        int child_handle = possible_cover_widget.children_widget_handles[i];
        widget_t* child_widget = get_widget(child_handle);
        game_assert_msg(child_widget, "child widget cannot be found");
        if (hover_widget_partially_covered_helper(hover_widget, *child_widget)) {
            return true;
        }
    }

    return false;
}

static bool hover_widget_partially_covered(widget_t& hover_widget) {
    for (int j = 0; j < curframe_ui_info->widgets_arr.size(); j++) {
        widget_t& widget = curframe_ui_info->widgets_arr[j];
        if (widget.parent_widget_handle == -1 && widget.z_pos > hover_widget.z_pos) {
            if (hover_widget_partially_covered_helper(hover_widget, widget)) {
                return true;
            }
        }
    }

    return false;
}

static void traverse_ui(int widget_handle, glm::vec2 dir) {
    auto& arr = curframe_ui_info->widgets_arr;
    widget_t* cur_hover_widget = get_widget(widget_handle);
#if TRAVERSE_UI_EXTRA_DEBUG == 1
    static int test_index;
    test_index = 0;
#endif
    if (!cur_hover_widget) {
        for (int i = 0; i < arr.size(); i++) {
            widget_t& widget = arr[i];
            if ((widget.properties & UI_PROP_HOVERABLE) == 0) continue;

            float other_x = widget.x + widget.style.margin.x + (widget.render_width / 2.f);
            float other_y = widget.y - widget.style.margin.y - (widget.render_height / 2.f);
            glm::vec2 other_widget_pos(other_x, other_y);
            if (!within_screen(other_widget_pos)) continue;
            if (hover_widget_partially_covered(widget)) continue;

            set_widget_as_hover(widget.handle, std::string(widget.key));
            return;
        }
    } else {

        struct running_info_t {
            float angle = FLT_MAX;
            float distance = FLT_MAX;
            float weight = FLT_MAX;
            int widget_handle = -1;
            std::string widget_key;
        };

        running_info_t running_info;

        float cur_x = cur_hover_widget->x + cur_hover_widget->style.margin.x + (cur_hover_widget->render_width / 2.f);
        float cur_y = cur_hover_widget->y - cur_hover_widget->style.margin.y - (cur_hover_widget->render_height / 2.f);
        glm::vec2 cur_widget_pos(cur_x, cur_y);

#if TRAVERSE_UI_EXTRA_DEBUG == 1
        printf("\nverts = [\n");
#endif
        for (int i = 0; i < arr.size(); i++) {
            widget_t& widget = arr[i];
            float other_x = widget.x + widget.style.margin.x + (widget.render_width / 2.f);
            float other_y = widget.y - widget.style.margin.y - (widget.render_height / 2.f);
            glm::vec2 other_widget_pos(other_x, other_y);
            if ((widget.properties & UI_PROP_HOVERABLE) == 0) continue; 
            if (!within_screen(other_widget_pos)) continue;

#if TRAVERSE_UI_EXTRA_DEBUG == 1
            printf("\t(%f, %f)%s # %i %s\n", other_widget_pos.x, other_widget_pos.y, i == arr.size() - 1 ? "" : ",", test_index++, widget.key);
#endif
            if (widget.handle == widget_handle) continue;
            if (hover_widget_partially_covered(widget)) continue;

            glm::vec2 diff = other_widget_pos - cur_widget_pos;
            glm::vec2 diff_normalized = glm::normalize(diff);
            float dotted = glm::dot(dir, diff_normalized);
            if (dotted <= 0) continue;

            glm::vec2 diff_screen_norm = normalized_screen_coord(diff);
            float screen_norm_distance = glm::distance(diff_screen_norm, glm::vec2(0)) * 10.f;
            
            float parallel_to_diff_screen = glm::dot(dir, diff_screen_norm);
            float perpen_to_diff_screen_squared = (screen_norm_distance * screen_norm_distance) - (parallel_to_diff_screen * parallel_to_diff_screen);

            // will be between 0 and pi/2
            float pre_quantized_angle = acos(dotted);
            float angle = quantize_angle(pre_quantized_angle, 16);

            // weight is distance is screen normalized vec was stretched on the non-dir axis
            float weight = glm::pow(parallel_to_diff_screen * 10, 2) + glm::pow(perpen_to_diff_screen_squared * 100, 2);

            bool update_running = (angle == 0 && running_info.angle != 0) || 
                (running_info.angle == angle && screen_norm_distance < running_info.distance) || (running_info.angle != 0 && angle != 0 && weight < running_info.weight);

            if (update_running) {
                running_info.angle = angle;
                running_info.distance = screen_norm_distance;
                running_info.weight = weight;
                running_info.widget_handle = widget.handle;
                running_info.widget_key = widget.key;
            }
        }
#if TRAVERSE_UI_EXTRA_DEBUG == 1
        printf("]\n");
#endif

        if (running_info.widget_handle != -1) {
            set_widget_as_hover(running_info.widget_handle, running_info.widget_key);
            set_widget_mouse_leave(widget_handle, std::string(cur_hover_widget->key));
        }
        else {
            printf("none found\n");
        }
    }
}

void end_imgui() {
    if (globals.window.user_input.controller_state_changed) {
        clear_element_status(curframe_ui_info->ui_element_status);
    }

    if (!is_controller_connected() || !ui_state.ctrl_controlled) return;

    if (ui_will_update || ui_positions_changed) {
        ui_element_status_t original_curframe = curframe_ui_info->ui_element_status;
        clear_element_status(curframe_ui_info->ui_element_status);
        for (int i = 0; i < curframe_ui_info->widgets_arr.size(); i++) {
            widget_t& widget = curframe_ui_info->widgets_arr[i];
            if (strcmp(widget.key, "") == 0) continue;
            if (strcmp(widget.key, original_curframe.hovered_over.widget_key.c_str()) == 0) {
                widget_t* hover_widget = get_widget(widget.handle);
                game_assert_msg(hover_widget, "hover widget not found");
                if (within_screen(glm::vec2(hover_widget->x, hover_widget->y)) && !hover_widget_partially_covered(*hover_widget)) {
                    curframe_ui_info->ui_element_status.hovered_over.widget_handle = widget.handle;
                    curframe_ui_info->ui_element_status.hovered_over.widget_key = widget.key;
                }
            }
            if (strcmp(widget.key, original_curframe.clicked_on.widget_key.c_str()) == 0) {
                curframe_ui_info->ui_element_status.clicked_on.widget_handle = widget.handle;
                curframe_ui_info->ui_element_status.clicked_on.widget_key = widget.key;
            }
            if (strcmp(widget.key, original_curframe.mouse_enter.widget_key.c_str()) == 0) {
                curframe_ui_info->ui_element_status.mouse_enter.widget_handle = widget.handle;
                curframe_ui_info->ui_element_status.mouse_enter.widget_key = widget.key;
            }
            if (strcmp(widget.key, original_curframe.mouse_left.widget_key.c_str()) == 0) {
                curframe_ui_info->ui_element_status.mouse_left.widget_handle = widget.handle;
                curframe_ui_info->ui_element_status.mouse_left.widget_key = widget.key;
            }
        }
    }

    static bool controller_centered_x = true;
    static bool controller_centered_y = true;

    user_input_t& input_state = globals.window.user_input;
    controller_centered_x = controller_centered_x || fabs(input_state.controller_x_axis) <= 0.4f;
    controller_centered_y = controller_centered_y || fabs(input_state.controller_y_axis) <= 0.4f;

    bool right_move = false;
    bool left_move = false;
    bool up_move = false;
    bool down_move = false;

    glm::vec2 dir_vec(0);
    if (controller_centered_x && input_state.controller_x_axis >= 0.9f) {
        right_move = true;
        controller_centered_x = false;
        dir_vec = glm::vec2(1,0);
    } else if (controller_centered_x && input_state.controller_x_axis <= -0.9f) {
        left_move = true;
        controller_centered_x = false;
        dir_vec = glm::vec2(-1,0);
    } else if (controller_centered_y && input_state.controller_y_axis <= -0.9f) {
        down_move = true;
        controller_centered_y = false;
        dir_vec = glm::vec2(0,-1);
    } else if (controller_centered_y && input_state.controller_y_axis >= 0.9f) {
        up_move = true;
        controller_centered_y = false;
        dir_vec = glm::vec2(0,1);
    } 

    if (dir_vec != glm::vec2(0)) {
        traverse_ui(curframe_ui_info->ui_element_status.hovered_over.widget_handle, dir_vec);
    }

    anims_to_add_this_frame.clear();
    anims_to_start_this_frame.clear();
    anims_to_stop_this_frame.clear();
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
        // if (anim_weight > 0 && anim_weight < 1) {
        // }
    }

    return new_style;
}

void render_ui_helper(widget_t& widget) {

    if (widget.attached_hover_anim_player_handle != -1) {
        ui_anim_player_t* hover_player = get_ui_anim_player_by_idx(widget.attached_hover_anim_player_handle);
        game_assert_msg(hover_player, "hover player not found");
        // if (widget.handle == cur_final_focused_handle) {
        if (widget.handle == curframe_ui_info->ui_element_status.hovered_over.widget_handle) {
            move_ui_anim_player_forward(*hover_player);
        } else {
            move_ui_anim_player_backward(*hover_player);
        }
    }

    for (int i = 0; i < widget.attached_anim_player_handles.size(); i++) {
        ui_anim_player_t* attached_player = get_ui_anim_player_by_idx(widget.attached_anim_player_handles[i]);
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

    auto& cur_arr = curframe_ui_info->widgets_arr;
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
    ui_attrs.ui_properties = UI_PROP_NONE;
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
            ui_attrs.ui_properties = ui_attrs.ui_properties | UI_PROP_CLICKABLE | UI_PROP_HOVERABLE;
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

	autolayout_hierarchy();
    end_imgui();

    // if (ui_will_update) {
    //     cur_focused_internal_handle = -1;
    //     cur_final_focused_handle = -1;
    //     stack_nav_cur = false;
    // }

    auto& cur_arr = curframe_ui_info->widgets_arr;
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

	vao_enable_attribute(data.vao, data.vbo, 0, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, position));
	vao_enable_attribute(data.vao, data.vbo, 1, 4, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
	vao_enable_attribute(data.vao, data.vbo, 2, 2, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, tex_coord));
    vao_bind_ebo(data.vao, data.ebo);

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
	return running_dim;
}

bool get_if_key_clicked_on(const char* key) {
    return strcmp(prevframe_ui_info->ui_element_status.clicked_on.widget_key.c_str(), key) == 0;
}

bool get_if_key_hovered_over(const char* key) {
    return strcmp(prevframe_ui_info->ui_element_status.hovered_over.widget_key.c_str(), key) == 0;
}

bool get_if_key_mouse_enter(const char* key) {
    return strcmp(prevframe_ui_info->ui_element_status.mouse_enter.widget_key.c_str(), key) == 0;
}

bool get_if_key_mouse_left(const char* key) {
    return strcmp(prevframe_ui_info->ui_element_status.mouse_left.widget_key.c_str(), key) == 0;
}

// bool is_some_element_clicked_on() {
//     for (int i = 0; i < prevframe_widget_arr->size(); i++) {
//         widget_t& widget = (*prevframe_widget_arr)[i];
//         if (widget.properties & UI_PROP_CLICKABLE) {
//             auto& prev_arr = *prevframe_widget_arr;
//             widget_t& cached_widget = prev_arr[widget.handle];

//             user_input_t& input_state = globals.window.user_input;
//             bool mouse_over_widget = input_state.mouse_x >= (cached_widget.x + cached_widget.style.margin.x) &&
//                     input_state.mouse_x <= (cached_widget.x + cached_widget.render_width + cached_widget.style.margin.x) &&
//                     // render x and render y specified as the top left pivot and y in ui is 0 on the
//                     // bottom and WINDOW_HEIGHT on the top, so cached_widget.y is the top y of the 
//                     // widget and cached_widget.y - cached_widget.render_height is the bottom y 
//                     // of the widget
//                     input_state.mouse_y <= (cached_widget.y - cached_widget.style.margin.y) &&
//                     input_state.mouse_y >= (cached_widget.y - cached_widget.render_height - cached_widget.style.margin.y);

//             //if (mouse_over_widget && !input_state.game_controller && globals.window.user_input.left_mouse_release) {
//             if (mouse_over_widget && !input_state.game_controller && get_released(LEFT_MOUSE)) {
//                 return true;
//             }
//         }
//     }
//     return false;
// }

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
    auto& prev_arr = prevframe_ui_info->widgets_arr;
    for (int i = 0; i < prev_arr.size(); i++) {
        prev_arr[i].attached_hover_anim_player_handle = -1;
    }
}

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

static int ui_anim_player_cnt = 0;
int create_ui_anim_player(const char* widget_key, int ui_anim_file_handle, ui_anim_t& ui_anim, bool play_upon_initialize, time_count_t starting_cursor) {
    ui_anim_player_t anim_player;
    anim_player.handle = ui_anim_player_cnt++;
    memcpy(anim_player.widget_key, widget_key, strlen(widget_key));
    anim_player.anim_duration = ui_anim.anim_duration;
    anim_player.ui_anim_handle = ui_anim.handle;
    anim_player.ui_anim_file_handle = ui_anim_file_handle;
    anim_player.duration_cursor = fmin(starting_cursor, ui_anim.anim_duration);
    anim_player.playing = play_upon_initialize;
    ui_anim_players.push_back(anim_player);
    return anim_player.handle;
}

void reset_ui_anim_player_cnt() {
    ui_anim_player_cnt = 0;
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

ui_anim_player_t* get_ui_anim_player_by_idx(int idx) {
    if (idx >= ui_anim_players.size()) return NULL;
    return &ui_anim_players[idx]; 
}

ui_anim_player_t* get_ui_anim_player_by_handle(int handle) {
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
    status.clicked_on.widget_handle = -1;

    status.hovered_over.widget_key = "";
    status.hovered_over.z_pos = INT_MIN;
    status.hovered_over.widget_handle = -1;

    status.mouse_enter.widget_key = "";
    status.mouse_enter.z_pos = INT_MIN;
    status.mouse_enter.widget_handle = -1;

    status.mouse_left.widget_key = "";
    status.mouse_left.z_pos = INT_MIN;
    status.mouse_left.widget_handle = -1;
}

// OR_ENUM_DEFINITION(UI_CONTROL_MODE)
// AND_ENUM_DEFINITION(UI_CONTROL_MODE);
// AND_W_INT_ENUM_DEFINITION(UI_CONTROL_MODE);

void ui_disable_controller_support() {
    ui_state.ctrl_controlled = false;
    // if ((ui_state.control_mode & UI_CONTROL_CNTLR) != 0) {
    //     ui_state.control_mode = ui_state.control_mode & (~UI_CONTROL_CNTLR);
    // }
}

void ui_enable_controller_support() {
    ui_state.ctrl_controlled = true;
    // ui_state.control_mode = ui_state.control_mode | UI_CONTROL_CNTLR;
}