#include "ui.h"

#include <iostream>
#include <unordered_map>
#include <unordered_set>
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

#include <ft2build.h>
#include FT_FREETYPE_H  

namespace fs = std::filesystem;

static bool panel_left_used = false;

static std::vector<bck_color_override_t> bck_color_overrides;

extern globals_t globals;

// static char xml_path[256];

// static ui_file_layout_t ui_file;
static std::vector<ui_file_layout_t> ui_files;
static std::vector<int> active_ui_file_handles;

static int cur_focused_internal_handle = -1;
static int cur_final_focused_handle = -1;
static bool stack_nav_cur = false;

static int cur_widget_count = 0;

static std::vector<int> widget_stack1;
static std::vector<widget_t> widgets_arr1;
static std::unordered_set<std::string> clicked_on_keys1;

static std::vector<int> widget_stack2;
static std::vector<widget_t> widgets_arr2;
static std::unordered_set<std::string> clicked_on_keys2;

static std::vector<int>* curframe_widget_stack = &widget_stack2;
static std::vector<widget_t>* curframe_widget_arr = &widgets_arr2;
static std::unordered_set<std::string>* curframe_clicked_on_keys;
static bool stacked_nav_widget_in_stack = false;

static std::vector<int>* prevframe_widget_stack = &widget_stack1;
static std::vector<widget_t>* prevframe_widget_arr = &widgets_arr1;
static std::unordered_set<std::string>* prevframe_clicked_on_keys;

static std::vector<style_t> styles_stack;

static std::unordered_map<int, constraint_var_t> constraint_vars;
static std::vector<constraint_t> constraints;

static int constraint_running_cnt = 0;

static bool ui_will_update = false;

static std::unordered_map<int, hash_t> handle_hashes;

static std::unordered_map<std::string, std::string> ui_text_values;

static std::vector<font_mode_t> font_modes;
// static font_mode_t font_modes[2] = {
//     font_mode_t {
//         // TEXT_SIZE::TITLE,
//         65
//     },
//     font_mode_t {
//         // TEXT_SIZE::REGULAR,
//         25
//     }
// };

render_object_data font_char_t::ui_opengl_data{};

bool is_same_hash(hash_t& hash1, hash_t& hash2) {
    for (int i = 0; i < 8; i++) {
        if (hash1.unsigned_ints[i] != hash2.unsigned_ints[i]) {
            return false;
        }
    }
    return true;
}

void print_byte(uint8_t in) {
    for (int i = 7; i >= 0; i--) {
        uint8_t bit = (in >> i) & (0x01);
        if (bit) {
            printf("1");
        } else {
            printf("0");
        }
    }
}

void print_512(uint512_t& in) {
    for (int i = 0; i < 64; i++) {
        uint8_t v = in.unsigned_bytes[i];
        // printf(" %b ", v);
        print_byte(v);
        printf(" ");
    }
    printf("\n");
}

void print_sha(hash_t& sha) {
	printf("%0llx %0llx %0llx %0llx", sha.unsigned_double[0], sha.unsigned_double[1], sha.unsigned_double[2], sha.unsigned_double[3]);
}

uint32_t wrap(uint32_t in, int num) {
    uint32_t res = 0;
    res = in >> num;
    uint32_t top = in << (32 - num);
    res = res | num;
    return res;
}

uint32_t bitwise_add_mod2(uint32_t a, uint32_t b, uint32_t c) {
    // return (~a & ~b & c) | (~a & b & ~c) | (a & b & c) | (a & ~b & ~c);
    return a ^ b ^ c;
}

uint32_t epsilon0(uint32_t in) {
    uint32_t wrap7 = wrap(in, 7);
    uint32_t wrap18 = wrap(in, 18);
    uint32_t shift3 = in >> 3; 
    return bitwise_add_mod2(wrap7, wrap18, shift3);
}

uint32_t epsilon1(uint32_t in) {
    uint32_t wrap17 = wrap(in, 17);
    uint32_t wrap19 = wrap(in, 19);
    uint32_t shift10 = in >> 10; 
    return bitwise_add_mod2(wrap17, wrap19, shift10);
}

uint32_t sigma0(uint32_t in) {
    uint32_t wrap2 = wrap(in, 2);
    uint32_t wrap13 = wrap(in, 13);
    uint32_t wrap22 = wrap(in, 22);
    return bitwise_add_mod2(wrap2, wrap13, wrap22);
}

uint32_t sigma1(uint32_t in) {
    uint32_t wrap6 = wrap(in, 6);
    uint32_t wrap11 = wrap(in, 11);
    uint32_t wrap25 = wrap(in, 25);
    return bitwise_add_mod2(wrap6, wrap11, wrap25);
}

uint32_t maj(uint32_t a, uint32_t b, uint32_t c) {
    return (a & b) ^ (a & c) ^ (b & c);
}

uint32_t ch(uint32_t a, uint32_t b, uint32_t c) {
    return (a & b) ^ (~a & c);
}

uint32_t calc_t1(working_variables_t& cur_vars, uint32_t* w, int t) {

    static uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint64_t interm = cur_vars.h + sigma1(cur_vars.e) + ch(cur_vars.e, cur_vars.f, cur_vars.g) + k[t] + w[t];
    uint32_t t1 = mod_2_pow_32(interm);
    return t1;
}

uint32_t calc_t2(working_variables_t& cur_vars) {
    uint64_t s0 = sigma0(cur_vars.a);
    uint64_t interm = s0 + maj(cur_vars.a, cur_vars.b, cur_vars.c);
    return mod_2_pow_32(interm);
}

uint32_t mod_2_pow_32(uint64_t in) {
    uint64_t div = 1;
    div = div << 32;
    uint64_t interm = in % div;
    uint64_t mask = 0x0000ffff;
    uint32_t res = interm & mask;
    return res;
}

// not exactly sha 256 hash but does similar hash
// TODO: for exact sha 256 hashing, need to look more closely at bit layouts
// since things in mem r little endian but working with unions means
// insert values cause diff little endian behavior

// ex) settings the size in the last unsigned double looks fine for the double,
// but when interpreted as unsigned ints, it is in M[14] rather than M[15] which
// is need for sha256 exactly

// all in all, sha256 switches between different representations of the same 512 bits,
// but in acc memory this is weird cause diff representation is formatted diff in mem

// but for rn...this is good enough...so for rn...its like sha-256 but not sha-256
hash_t hash(const char* key) {
    uint512_t input{};
    int key_len = strlen(key);
    uint64_t size = 8 * key_len;
    game_assert_msg(size < 512, "key size must be less than 512 bits");
    
    memcpy(&input, key, key_len);

    /*  PADDING */
    // last 8 bytes reserved for the padding size
    input.unsigned_bytes[key_len] = 0x80;
    input.unsigned_double[7] = size;

    // print_sha()
    // print_512(input);

    uint32_t h[8] = {
        0x6a09e667,
        0xbb67ae85,
        0x3c6ef372,
        0xa54ff53a,
        0x510e527f,
        0x9b05688c,
        0x1f83d9ab,
        0x5be0cd19
    };

    uint32_t w[64]{};
    for (int t = 0; t < 16; t++) {
        w[t] = input.unsigned_ints[t];
    }

    for (int t = 16; t < 63; t++) {
        int64_t ep1 = epsilon1(w[t-2]);
        int64_t ep0 = epsilon0(w[t-15]);
        uint64_t intermediate = ep1 + w[t-7] + ep0 + w[t-16];
        w[t] = mod_2_pow_32(intermediate);
    }

    working_variables_t working_variables{};
    memcpy(&working_variables, h, sizeof(working_variables_t));

    for (int t = 0; t < 63; t++) {
        working_variables_t cur_vars = working_variables;

        uint64_t t1 = calc_t1(cur_vars, w, t);
        uint64_t t2 = calc_t2(cur_vars);

        uint64_t a_interm = t1 + t2;
        working_variables.a = mod_2_pow_32(a_interm);

        working_variables.b = cur_vars.a;
        working_variables.c = cur_vars.b;
        working_variables.d = cur_vars.c;

        uint64_t e_interm = cur_vars.d + t1;
        working_variables.e = mod_2_pow_32(e_interm);

        working_variables.f = cur_vars.e;
        working_variables.g = cur_vars.f;
        working_variables.h = cur_vars.g;

        cur_vars = working_variables;
    }

    for (int i = 0; i < 8; i++) {
        uint64_t v1 = working_variables.vals[i];
        uint64_t v2 = h[i];
        uint64_t v = v1 + v2;
        h[i] = mod_2_pow_32(v);
    }

    hash_t sha;
    for (int i = 0; i < 8; i++) {
        sha.unsigned_ints[i] = h[i];
    }

    return sha;
}

void update_ui_files() {
    // char buffer[256]{};
	// get_resources_folder_path(buffer);
    // char main_menu_ui_xml_path[256]{};
	// sprintf(main_menu_ui_xml_path, "%s\\%s\\play.xml", buffer, UI_FOLDER);
 
    for (int i = 0; i < ui_files.size(); i++) {
        ui_file_layout_t& ui_file = ui_files[i];
        struct stat ui_file_stat {};
        // if (stat(main_menu_ui_xml_path, &ui_file_stat) < 0) return;
        if (stat(ui_file.path, &ui_file_stat) < 0) continue;

        if (ui_file.last_modified_time == ui_file_stat.st_mtime) continue;
        _sleep(10);
        xml_document_free(ui_file.document, true);

        // FILE* main_menu_ui = fopen(main_menu_ui_xml_path, "r");
        FILE* file = fopen(ui_file.path, "r");
        game_assert_msg(file, "ui file not found");
        ui_file.document = xml_open_document(file);
        ui_file.last_modified_time = ui_file_stat.st_mtime;
    }
}

void ui_start_of_frame() {
    panel_left_used = false;
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

    bck_color_overrides.clear();
    ui_text_values.clear();
    styles_stack.clear();
    style_t default_style;
    styles_stack.push_back(default_style);

    if (curframe_widget_arr == &widgets_arr1) {
        curframe_widget_arr = &widgets_arr2;
        curframe_widget_stack = &widget_stack2;
        curframe_clicked_on_keys = &clicked_on_keys2;
        prevframe_widget_arr = &widgets_arr1;
        prevframe_widget_stack = &widget_stack1;
        prevframe_clicked_on_keys = &clicked_on_keys1;
    } else {
        curframe_widget_arr = &widgets_arr1;
        curframe_widget_stack = &widget_stack1;
        curframe_clicked_on_keys = &clicked_on_keys1;
        prevframe_widget_arr = &widgets_arr2;
        prevframe_widget_stack = &widget_stack2;
        prevframe_clicked_on_keys = &clicked_on_keys2;
    }

    curframe_widget_arr->clear();
    curframe_widget_stack->clear();
    curframe_clicked_on_keys->clear();

    cur_widget_count = 0;

    constraint_vars.clear();
    constraints.clear();
    constraint_running_cnt = 0;
}

void push_style(style_t& style) {
    styles_stack.push_back(style);
}

void pop_style() {
    // game_assert(styles_stack.size() > 1);
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
    widget.style = styles_stack[styles_stack.size() - 1];

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

        if (mouse_over_widget && !input_state.game_controller) {
            cur_focused_internal_handle = widget.handle;
        }

        if (mouse_over_widget && input_state.left_mouse_release) {
            curframe_clicked_on_keys->insert(std::string(key));
            info.clicked_on = true;
        }

        if (widget.handle == cur_final_focused_handle && (input_state.enter_pressed || input_state.controller_a_pressed)) {
            curframe_clicked_on_keys->insert(std::string(key));
            info.clicked_on = true;
        }
    }

    return info;
}

float panel_left = 0;

void create_panel(const char* panel_name) {
    widget_t panel;
    memcpy(panel.key, panel_name, strlen(panel_name)); 
    panel.height = globals.window.window_height;
    panel.width = globals.window.window_width;
    // panel.widget_size = WIDGET_SIZE::PIXEL_BASED;
    panel.widget_size_width = WIDGET_SIZE::PIXEL_BASED;
    panel.widget_size_height = WIDGET_SIZE::PIXEL_BASED;

    if (strcmp(panel_name, "store_panel") == 0) {
        panel.x = panel_left;
        panel_left_used = true;
    } else {
        panel.x = 0;
    }
    panel.y = panel.height;
    panel.render_width = panel.width;
    panel.render_height = panel.height;

    panel.content_width = panel.width;
    panel.content_height = panel.height;

    register_widget(panel, panel_name, true);
}

void end_panel() {
    pop_widget();
}

// void create_container(float width, float height, WIDGET_SIZE widget_size) {
void create_container(float width, float height, WIDGET_SIZE widget_size_width, WIDGET_SIZE widget_size_height, const char* container_name, bool focusable, stacked_nav_handler_func_t func, UI_PROPERTIES ui_properties) {
    widget_t container;
    memcpy(container.key, container_name, strlen(container_name));
    container.height = height;
    container.width = width;
    container.widget_size_width = widget_size_width;
    container.widget_size_height = widget_size_height; 

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

    widget_t widget;
    widget.image_based = true;
    widget.texture_handle = texture_handle;
    widget.widget_size_width = widget_size_width;
    widget.widget_size_height = widget_size_height;
    widget.width = width;
    widget.height = height;

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

    widget_t widget;
    widget.text_based = true;
    memcpy(widget.text_info.text, text, fmin(sizeof(widget.text_info.text), text_len));
    widget.text_info.font_size = font_size;

    if (focusable) {
        widget.properties = static_cast<UI_PROPERTIES>(UI_PROPERTIES::UI_PROP_FOCUSABLE);
    }

    text_dim_t text_dim = get_text_dimensions(text, font_size);

    widget.widget_size_width = WIDGET_SIZE::PIXEL_BASED;
    widget.widget_size_height = WIDGET_SIZE::PIXEL_BASED;
    
    style_t& latest_style = styles_stack[styles_stack.size() - 1];
    widget.width = text_dim.width + (2 * latest_style.padding.x);
    // widget.height = text_dim.height + (2 * latest_style.padding.y);
    widget.height = text_dim.max_height_above_baseline + (2 * latest_style.padding.y);

    widget.render_width = widget.width;
    widget.render_height = widget.height;

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

    widget_t widget;
    widget.text_based = true;
    memcpy(widget.text_info.text, text, fmin(sizeof(widget.text_info.text), text_len));
    widget.text_info.font_size = font_size;

    text_dim_t text_dim = get_text_dimensions(text, font_size);

    widget.widget_size_width = WIDGET_SIZE::PIXEL_BASED;
    widget.widget_size_height = WIDGET_SIZE::PIXEL_BASED;
    style_t& latest_style = styles_stack[styles_stack.size() - 1];
    widget.width = text_dim.width + (2 * latest_style.padding.x);
    widget.height = text_dim.max_height_above_baseline + (2 * latest_style.padding.y);

    widget.render_width = widget.width;
    widget.render_height = widget.height;

    widget.properties = widget.properties | UI_PROP_CLICKABLE;
    auto& stack = *curframe_widget_stack;
    if (stacked_nav_widget_in_stack) {
        widget.user_handle = user_handle;
    } else {
        widget.properties = widget.properties | UI_PROP_FOCUSABLE;
    }

    widget_registration_info_t widget_info = register_widget(widget, key);
    return widget_info.clicked_on;

    // if (!ui_will_update) {
    //     auto& prev_arr = *prevframe_widget_arr;
    //     widget_t& cached_widget = prev_arr[widget_handle];

    //     user_input_t& input_state = globals.window.user_input;
    //     bool mouse_over_widget = input_state.mouse_x >= (cached_widget.x + cached_widget.style.margin.x) &&
    //             input_state.mouse_x <= (cached_widget.x + cached_widget.render_width + cached_widget.style.margin.x) &&
    //             // render x and render y specified as the top left pivot and y in ui is 0 on the
    //             // bottom and WINDOW_HEIGHT on the top, so cached_widget.y is the top y of the 
    //             // widget and cached_widget.y - cached_widget.render_height is the bottom y 
    //             // of the widget
    //             input_state.mouse_y <= (cached_widget.y - cached_widget.style.margin.y) &&
    //             input_state.mouse_y >= (cached_widget.y - cached_widget.render_height - cached_widget.style.margin.y);

    //     if (mouse_over_widget && !input_state.game_controller) {
    //         cur_focused_internal_handle = widget_handle;
    //     }

    //     if (mouse_over_widget && input_state.left_mouse_release) {
    //         clicked_on_keys.insert(std::string(key));
    //         return true;
    //     }

    //     if (widget_handle == cur_final_focused_handle && (input_state.enter_pressed || input_state.controller_a_pressed)) {
    //         clicked_on_keys.insert(std::string(key));
    //         return true;
    //     }

    //     return false;
    // }

    // return false;
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
        widget.width = text_dim.width;
        // widget.height = text_dim.height;
        widget.height = text_dim.max_height_above_baseline;

        widget.render_width = widget.width + (widget.style.padding.x * 2);
        widget.render_height = widget.height + (widget.style.padding.y * 2);

        widget.content_width = widget.render_width + (widget.style.margin.x * 2);
        widget.content_height = widget.render_height + (widget.style.margin.y * 2);

        helper_info_t helper_info;
        helper_info.content_width = widget.content_width;
        helper_info.content_height = widget.content_height;
        return helper_info;
    }

    int widget_width_handle = create_constraint_var("width", &widget.render_width);
    int widget_height_handle = create_constraint_var("height", &widget.render_height); 

    if (widget.widget_size_width == WIDGET_SIZE::PIXEL_BASED) {
        game_assert(widget.width >= 0.f);
        float render_width = 0.f;
        if (parent_width_handle == -1 || parent_height_handle == -1) {
            render_width = widget.width;
        } else {
            render_width = widget.width + (widget.style.padding.x * 2);
        }

        make_constraint_value_constant(widget_width_handle, render_width);
    } else if (widget.widget_size_width == WIDGET_SIZE::PARENT_PERCENT_BASED) {
        game_assert(widget.width <= 1.f);
        game_assert(widget.width >= 0.f);

        std::vector<constraint_term_t> width_terms;
        width_terms.push_back(create_constraint_term(parent_width_handle, widget.width));
        create_constraint(widget_width_handle, width_terms, widget.style.padding.x * 2);
    }
    
    if (widget.widget_size_height == WIDGET_SIZE::PIXEL_BASED) {
        game_assert(widget.height >= 0.f);
        float render_height = 0.f;
        if (parent_width_handle == -1 || parent_height_handle == -1) {
            render_height = widget.height;
        } else {
            render_height = widget.height + (widget.style.padding.y * 2);
        }
        make_constraint_value_constant(widget_height_handle, render_height);
    } else if (widget.widget_size_height == WIDGET_SIZE::PARENT_PERCENT_BASED) {
        game_assert(widget.height <= 1.f);
        game_assert(widget.height >= 0.f);

        std::vector<constraint_term_t> height_terms;
        height_terms.push_back(create_constraint_term(parent_height_handle, widget.height));
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
        
    if (widget.widget_size_width == WIDGET_SIZE::FIT_CONTENT) {
        float render_width = content_size.x + (widget.style.padding.x * 2.f);
        make_constraint_value_constant(widget_width_handle, render_width);
    }
    
    if (widget.widget_size_height == WIDGET_SIZE::FIT_CONTENT) {
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
        if (cur_widget.parent_widget_handle != -1) continue;
        resolve_dimensions(cur_widget.handle, -1, -1);
    }

    for (int i = 0; i < cur_arr.size(); i++) {
        widget_t& cur_widget = cur_arr[i];
        if (cur_widget.parent_widget_handle != -1) continue;
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

void render_ui_helper(widget_t& widget) {

    if (widget.handle == cur_final_focused_handle) {
        if (widget.style.hover_background_color != TRANSPARENT_COLOR) {
            widget.style.bck_mode = BCK_SOLID;
            widget.style.background_color = widget.style.hover_background_color;
        }
        if (widget.style.hover_color != TRANSPARENT_COLOR) {
            widget.style.color = widget.style.hover_color;
        }
        for (int i = 0; i < bck_color_overrides.size(); i++) {
            bck_color_override_t& ovrride = bck_color_overrides[i];
            if (strcmp(ovrride.widget_key, widget.key) == 0) {
                widget.style.bck_mode = ovrride.bck_mode;
                widget.style.background_color = ovrride.background_color;
                widget.style.top_left_bck_color = ovrride.top_left_bck_color;
                widget.style.top_right_bck_color = ovrride.top_right_bck_color;
                widget.style.bottom_left_bck_color = ovrride.bottom_left_bck_color;
                widget.style.bottom_right_bck_color = ovrride.bottom_right_bck_color;
            }
        }
        draw_background(widget);
    } else { 
        for (int i = 0; i < bck_color_overrides.size(); i++) {
            bck_color_override_t& ovrride = bck_color_overrides[i];
            if (strcmp(ovrride.widget_key, widget.key) == 0) {
                widget.style.bck_mode = ovrride.bck_mode;
                widget.style.background_color = ovrride.background_color;
                widget.style.top_left_bck_color = ovrride.top_left_bck_color;
                widget.style.top_right_bck_color = ovrride.top_right_bck_color;
                widget.style.bottom_left_bck_color = ovrride.bottom_left_bck_color;
                widget.style.bottom_right_bck_color = ovrride.bottom_right_bck_color;
            }
        }
        if ((widget.style.bck_mode == BCK_SOLID && widget.style.background_color != TRANSPARENT_COLOR) || 
            widget.style.bck_mode == BCK_GRADIENT_TOP_LEFT_TO_BOTTOM_RIGHT ||
            widget.style.bck_mode == BCK_GRADIENT_4_CORNERS) {
            draw_background(widget);
        }
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

parsed_ui_attributes_t get_style_and_key(xml_attribute** attributes) {
    parsed_ui_attributes_t ui_attrs;
    style_t& style = ui_attrs.style;
    size_t num_attrs = get_zero_terminated_array_attributes(attributes);
    for (size_t i = 0; i < num_attrs; i++) {
        xml_attribute* attr = attributes[i];
        char* name = xml_get_zero_terminated_buffer(attr->name);
        char* content = xml_get_zero_terminated_buffer(attr->content);
        if (strcmp(name, "id") == 0) {
            memcpy(ui_attrs.id, content, fmin(strlen(content), 64));
        } else if (strcmp(name, "image_src") == 0) {
            char buffer[256]{};
            get_resources_folder_path(buffer);
            sprintf(ui_attrs.image_path, "%s\\%s\\%s", buffer, UI_FOLDER, content);
        } else if (strcmp(name, "display_dir") == 0) {
            DISPLAY_DIR dir = DISPLAY_DIR::HORIZONTAL;
            if (strcmp(content, "vertical") == 0) {
                dir = DISPLAY_DIR::VERTICAL;
            } else if (strcmp(content, "horizontal") == 0) {
                dir = DISPLAY_DIR::HORIZONTAL;
            }
            style.display_dir = dir;
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
        } else if (strcmp(name, "padding") == 0) {
            glm::vec2 padding(0);
            sscanf(content, "%f,%f", &padding.x, &padding.y);
            style.padding = padding;
        } else if (strcmp(name, "margin") == 0) {
            glm::vec2 margin(0);
            sscanf(content, "%f,%f", &margin.x, &margin.y);
            style.margin = margin;
        } else if (strcmp(name, "content_spacing") == 0) {
            style.content_spacing = atof(content);
        } else if (strcmp(name, "background_color") == 0) {
            glm::vec3 color(0);
            sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
            style.background_color = create_color(color.r, color.g, color.b);
        }  else if (strcmp(name, "color") == 0) {
            glm::vec3 color(0);
            sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
            style.color = create_color(color.r, color.g, color.b);
        } else if (strcmp(name, "border_radius") == 0) {
            style.border_radius = atof(content);
        } else if (strcmp(name, "tl_border_radius") == 0) {
            style.tl_border_radius = atof(content);
        } else if (strcmp(name, "tr_border_radius") == 0) {
            style.tr_border_radius = atof(content);
        } else if (strcmp(name, "bl_border_radius") == 0) {
            style.bl_border_radius = atof(content);
        } else if (strcmp(name, "br_border_radius") == 0) {
            style.br_border_radius = atof(content);
        } else if (strcmp(name, "border_radius_mode") == 0) {
            BORDER_RADIUS_MODE mode = BR_SINGLE_VALUE;
            if (strcmp(content, "single") == 0) {
                mode = BR_SINGLE_VALUE;
            } else if (strcmp(content, "4-corners") == 0) {
                mode = BR_4_CORNERS;
            }
            style.border_radius_mode = mode;
        } else if (strcmp(name, "hover_background_color") == 0) {
            glm::vec3 color(0);
            sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
            style.hover_background_color = create_color(color.r, color.g, color.b);
        } else if (strcmp(name, "hover_color") == 0) {
            glm::vec3 color(0);
            sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
            style.hover_color = create_color(color.r, color.g, color.b);
        } else if (strcmp(name, "width") == 0) {
            ui_attrs.width = atof(content);
        } else if (strcmp(name, "height") == 0) {
            ui_attrs.height = atof(content);
        } else if (strcmp(name, "widget_size_width") == 0) {
            WIDGET_SIZE size = WIDGET_SIZE::PIXEL_BASED;
            if (strcmp(content, "pixel") == 0) {
                size = WIDGET_SIZE::PIXEL_BASED;
            } else if (strcmp(content, "parent") == 0) {
                size = WIDGET_SIZE::PARENT_PERCENT_BASED;
            } else if (strcmp(content, "fit") == 0) {
                size = WIDGET_SIZE::FIT_CONTENT;
            }
            ui_attrs.widget_size_width = size;
        } else if (strcmp(name, "widget_size_height") == 0) {
            WIDGET_SIZE size = WIDGET_SIZE::PIXEL_BASED;
            if (strcmp(content, "pixel") == 0) {
                size = WIDGET_SIZE::PIXEL_BASED;
            } else if (strcmp(content, "parent") == 0) {
                size = WIDGET_SIZE::PARENT_PERCENT_BASED;
            } else if (strcmp(content, "fit") == 0) {
                size = WIDGET_SIZE::FIT_CONTENT;
            }
            ui_attrs.widget_size_height = size;
        } else if (strcmp(name, "font_size") == 0) {
            int font_size = 0;
            sscanf(content, "%i", &ui_attrs.font_size);
        } else if (strcmp(name, "bck_mode") == 0) {
            BCK_MODE bck_mode = BCK_SOLID;
            if (strcmp(content, "solid") == 0) {
                bck_mode = BCK_SOLID;
            } else if (strcmp(content, "gradient-tl-br") == 0) {
                bck_mode = BCK_GRADIENT_TOP_LEFT_TO_BOTTOM_RIGHT;
            } else if (strcmp(content, "gradient") == 0) {
                bck_mode = BCK_GRADIENT_4_CORNERS;
            }
            ui_attrs.style.bck_mode = bck_mode;
        } else if (strcmp(name, "tl_bck") == 0) {
            glm::vec3 color(0);
            sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
            style.top_left_bck_color = create_color(color.r, color.g, color.b);
        } else if (strcmp(name, "br_bck") == 0) {
            glm::vec3 color(0);
            sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
            style.bottom_right_bck_color = create_color(color.r, color.g, color.b);
        } else if (strcmp(name, "bl_bck") == 0) {
            glm::vec3 color(0);
            sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
            style.bottom_left_bck_color = create_color(color.r, color.g, color.b);
        } else if (strcmp(name, "tr_bck") == 0) {
            glm::vec3 color(0);
            sscanf(content, "%f,%f,%f", &color.r, &color.g, &color.b);
            style.top_right_bck_color = create_color(color.r, color.g, color.b);
        } else if (strcmp(name, "clickable") == 0) {
            ui_attrs.ui_properties = ui_attrs.ui_properties | UI_PROP_CLICKABLE;
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
        create_panel(attrs.id);
    } else if (strcmp(zero_terminated_element_name, "container") == 0) {
        if (attrs.id[0] == '\0') {
            game_error_log("container not given a name through id attribute");
            // make memory location of node the id
            sprintf(attrs.id, "%i\n", node);
        }
        create_container(attrs.width, attrs.height, attrs.widget_size_width, attrs.widget_size_height, attrs.id, false, 0, attrs.ui_properties);
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
            
            char* final_str = (char*)calloc(64, sizeof(char));
            std::string key_string = key;
            std::string val = ui_text_values[key_string];
            sprintf(final_str, "%s%s%s", zero_terminated_content, val.c_str(), remaining_str);
            
            if (is_text_element) {
                create_text(final_str, attrs.font_size, false);
            } else {
                create_button(final_str, attrs.font_size, -1);
            }

            free(final_str);
        } else {
            if (is_text_element) {
                create_text(zero_terminated_content, attrs.font_size, false);
            } else {
                create_button(zero_terminated_content, attrs.font_size, -1);
            }
        }
        free(zero_terminated_content);
    } else if (strcmp(zero_terminated_element_name, "image") == 0) {
        int texture_handle = create_texture(attrs.image_path, 1);
        create_image_container(texture_handle, attrs.width, attrs.height, attrs.widget_size_width, attrs.widget_size_height, 1 + strrchr(attrs.image_path, '\\'));
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
	vao_enable_attribute(data.vao, data.vbo, 1, 3, GL_FLOAT, sizeof(vertex_t), offsetof(vertex_t, color));
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

    char xml_folder[256]{};
    char buffer[256]{};
    get_resources_folder_path(buffer);
    sprintf(xml_folder, "%s\\%s", buffer, UI_FOLDER);

    for (const auto& entry : fs::directory_iterator(xml_folder)) {
        auto& xml = entry.path();
        std::string xml_string = entry.path().string();
        const char* xml_path = xml_string.c_str();
        if (strcmp(get_file_extension(xml_path), "xml") != 0) continue;
        static int cnt = 0;
        FILE* ui_file_c = fopen(xml_path, "r");
        game_assert_msg(ui_file_c, "play file not found");
        ui_file_layout_t ui_file;
        ui_file.document = xml_open_document(ui_file_c);

        struct stat ui_file_stat;
        if (stat(xml_path, &ui_file_stat) < 0) return;
        ui_file.last_modified_time = ui_file_stat.st_mtime;
        memcpy(ui_file.path, xml_path, strlen(xml_path));

        ui_file.handle = cnt++;

        ui_files.push_back(ui_file);
    } 

//     for (int i = 0; i < 2; i++) {
//         char xml_path[256]{};
//         char buffer[256]{};
//         get_resources_folder_path(buffer);
// #ifdef UI_TESTING
//         sprintf(xml_path, "%s\\%s\\%s", buffer, UI_FOLDER, files[i]);
// #else
//         sprintf(xml_path, "%s\\%s\\play.xml", buffer, UI_FOLDER);
// #endif
//         // sprintf(xml_path, "%s\\%s\\main_menu.xml", buffer, UI_FOLDER);
//         FILE* ui_file_c = fopen(xml_path, "r");
//         game_assert_msg(ui_file_c, "play file not found");
//         ui_file_layout_t ui_file;
//         ui_file.document = xml_open_document(ui_file_c);

//         struct stat ui_file_stat;
//         if (stat(xml_path, &ui_file_stat) < 0) return;
//         ui_file.last_modified_time = ui_file_stat.st_mtime;
//         memcpy(ui_file.path, xml_path, strlen(xml_path));

//         ui_files.push_back(ui_file);
//     }
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
	// shader_set_vec3(font_char_t::ui_opengl_data.shader, "color", widget.style.background_color);
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

#if 0
	vertex_t updated_vertices[4];
	glm::vec2 origin = glm::vec2(x + (width / 2), y - (height / 2));
    updated_vertices[0] = create_vertex(glm::vec3(x + (width / 2), y + (height / 2), 0.0f), glm::vec3(0,1,1), glm::vec2(0,0)); // top right
    updated_vertices[1] = create_vertex(glm::vec3(x + (width / 2), y - (height / 2), 0.0f), glm::vec3(0,0,1), glm::vec2(0,0)); // bottom right
    updated_vertices[2] = create_vertex(glm::vec3(x - (width / 2), y - (height / 2), 0.0f), glm::vec3(0,1,0), glm::vec2(0,0)); // bottom left
    updated_vertices[3] = create_vertex(glm::vec3(x - (width / 2), y + (height / 2), 0.0f), glm::vec3(1,0,0), glm::vec2(0,0)); // top left
#else

    glm::vec3 top_left_color(0), top_right_color(0), bottom_left_color(0), bottom_right_color(0);
    if (widget.style.bck_mode == BCK_SOLID) {
        top_left_color = widget.style.background_color;
        top_right_color = widget.style.background_color;
        bottom_left_color = widget.style.background_color;
        bottom_right_color = widget.style.background_color;
    } else if (widget.style.bck_mode == BCK_GRADIENT_TOP_LEFT_TO_BOTTOM_RIGHT) {
        top_left_color = widget.style.top_left_bck_color;
        bottom_right_color = widget.style.bottom_right_bck_color;
        bottom_left_color = (top_left_color + bottom_right_color) / glm::vec3(2.f);
        top_right_color = (top_left_color + bottom_right_color) / glm::vec3(2.f);
    } else if (widget.style.bck_mode == BCK_GRADIENT_4_CORNERS) {
        top_left_color = widget.style.top_left_bck_color;
        top_right_color = widget.style.top_right_bck_color;
        bottom_left_color = widget.style.bottom_left_bck_color;
        bottom_right_color = widget.style.bottom_right_bck_color;
    }

	glm::vec3 origin = glm::vec3(x, y, 0);
    // updated_vertices[0] = create_vertex(origin + glm::vec3(width, 0, 0.0f), top_right_color, glm::vec2(0,0)); // top right
    // updated_vertices[1] = create_vertex(origin + glm::vec3(width, -height, 0.0f), bottom_right_color, glm::vec2(0,0)); // bottom right
    // updated_vertices[2] = create_vertex(origin + glm::vec3(0, -height, 0.0f), bottom_left_color, glm::vec2(0,0)); // bottom left
    // updated_vertices[3] = create_vertex(origin, top_left_color, glm::vec2(0,0)); // top left

	vertex_t updated_vertices[4];
#if 1
    updated_vertices[0] = create_vertex(origin + glm::vec3(width, 0, 0.0f), top_right_color, glm::vec2(0,0)); // top right
    updated_vertices[1] = create_vertex(origin + glm::vec3(width, -height, 0.0f), bottom_right_color, glm::vec2(0,0)); // bottom right
    updated_vertices[2] = create_vertex(origin + glm::vec3(0, -height, 0.0f), bottom_left_color, glm::vec2(0,0)); // bottom left
    updated_vertices[3] = create_vertex(origin, top_left_color, glm::vec2(0,0)); // top left
#else
    updated_vertices[0] = create_vertex(origin + glm::vec3(width, 0, 0.0f), top_right_color, glm::vec2(0,0)); // top right
    updated_vertices[1] = create_vertex(origin + glm::vec3(width, -height, 0.0f), bottom_right_color, glm::vec2(0,0)); // bottom right
    updated_vertices[2] = create_vertex(origin + glm::vec3(0, -height, 0.0f), bottom_left_color, glm::vec2(0,0)); // bottom left
    updated_vertices[3] = create_vertex(origin, top_left_color, glm::vec2(0,0)); // top left
#endif

#endif
    update_vbo_data(font_char_t::ui_opengl_data.vbo, (float*)updated_vertices, sizeof(updated_vertices));

    shader_set_vec3(font_char_t::ui_opengl_data.shader, "top_left", updated_vertices[3].position);
    shader_set_vec3(font_char_t::ui_opengl_data.shader, "top_right", updated_vertices[0].position);
    shader_set_vec3(font_char_t::ui_opengl_data.shader, "bottom_left", updated_vertices[2].position);
    shader_set_vec3(font_char_t::ui_opengl_data.shader, "bottom_right", updated_vertices[1].position);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

                    updated_vertices[0] = create_vertex(glm::vec3(running_pos.x + (fc.width / 2), running_pos.y + (fc.height / 2), 0.0f), color, glm::vec2(1,0)); // top right
                    updated_vertices[1] = create_vertex(glm::vec3(running_pos.x + (fc.width / 2), running_pos.y - (fc.height / 2), 0.0f), color, glm::vec2(1,1)); // bottom right
                    updated_vertices[2] = create_vertex(glm::vec3(running_pos.x - (fc.width / 2), running_pos.y - (fc.height / 2), 0.0f), color, glm::vec2(0,1)); // bottom left
                    updated_vertices[3] = create_vertex(glm::vec3(running_pos.x - (fc.width / 2), running_pos.y + (fc.height / 2), 0.0f), color, glm::vec2(0,0)); // top left
                    update_vbo_data(font_char_t::ui_opengl_data.vbo, (float*)updated_vertices, sizeof(updated_vertices));

                    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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

    updated_vertices[0] = create_vertex(glm::vec3(top_left.x + widget.render_width, top_left.y, 0.0f), glm::vec3(0,1,1), glm::vec2(1,1)); // top right
    updated_vertices[1] = create_vertex(glm::vec3(top_left.x + widget.render_width, top_left.y - widget.render_height, 0.f), glm::vec3(0,0,1), glm::vec2(1,0)); // bottom right
    updated_vertices[2] = create_vertex(glm::vec3(top_left.x, top_left.y - widget.render_height, 0.0f), glm::vec3(0,1,0), glm::vec2(0,0)); // bottom left
    updated_vertices[3] = create_vertex(glm::vec3(top_left.x, top_left.y, 0.0f), glm::vec3(1,0,0), glm::vec2(0,1)); // top left
    update_vbo_data(font_char_t::ui_opengl_data.vbo, (float*)updated_vertices, sizeof(updated_vertices));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // draw the rectangle render after setting all shader parameters
    draw_obj(font_char_t::ui_opengl_data);
}

bool get_if_key_clicked_on(const char* key) {
    // return clicked_on_keys.find(std::string(key)) != clicked_on_keys.end();
    return prevframe_clicked_on_keys->find(std::string(key)) != prevframe_clicked_on_keys->end();
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

void set_background_color_override(const char* widget_key, glm::vec3 color) {
    bck_color_override_t ovrride;
    memcpy(ovrride.widget_key, widget_key, strlen(widget_key));
    ovrride.background_color = color;
    ovrride.bck_mode = BCK_SOLID;
    bck_color_overrides.push_back(ovrride);
}

void set_background_color_gradient_4_corners_override(const char* widget_key, glm::vec3 top_left_color, glm::vec3 bottom_right_color) {
    bck_color_override_t ovrride;
    memcpy(ovrride.widget_key, widget_key, strlen(widget_key));
    ovrride.top_left_bck_color = top_left_color;
    ovrride.bottom_right_bck_color = bottom_right_color;
}