#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "sys/stat.h"

#include "constants.h"
#include "gfx/gfx_data/object_data.h"
#include "ui_hash.h"
#include "utils/time.h"
#include "utils/xml.h"

#include "glm/glm.hpp"

#define UI_RELOADING 0

#define GREEN glm::vec3(0, 1, 0)
#define BLUE glm::vec3(0, 0, 1)
#define RED glm::vec3(1, 0, 0)
#define DARK_BLUE glm::vec3(.003f, 0.1137f, 0.17647f)
#define SELECTED glm::vec3(.003f, 0.0537f, 0.085647f)
#define GREY glm::vec3(0.6274f,0.6274f,0.6274f)
#define LIGHT_GREY glm::vec3(0.8774f,0.8774f,0.8774f)
#define WHITE glm::vec3(1, 1, 1)

struct font_char_t {
	glm::vec2 bearing = glm::vec2(0);
	float width = 0;
	float height = 0;
	char c = 0;
	int texture_handle = -1;
	float advance = -1;

	static render_object_data ui_opengl_data;
};

struct font_mode_t {
    int font_size = 0;
    std::unordered_map<unsigned char, font_char_t> chars;
    bool used_last_frame = true;
};
void load_font(int font_size);

struct text_dim_t {
	float width = 0;
    float max_height_above_baseline = 0;
	float max_height_below_baseline = 0;
};
text_dim_t get_text_dimensions(const char* text, int font_size);

void init_ui();

enum class DISPLAY_DIR {
    VERTICAL, HORIZONTAL
};

enum class ALIGN {
    START, CENTER, END, SPACE_AROUND, SPACE_BETWEEN
};

#define TRANSPARENT_COLOR glm::vec4(0)

enum BCK_MODE {
    BCK_SOLID = 0,
    BCK_GRADIENT_TOP_LEFT_TO_BOTTOM_RIGHT,
    BCK_GRADIENT_4_CORNERS
};

enum BORDER_RADIUS_MODE {
    BR_SINGLE_VALUE = 0,
    BR_4_CORNERS
};

enum class WIDGET_SIZE {
    NONE,
    PIXEL_BASED,
    PARENT_PERCENT_BASED,
    FIT_CONTENT
};

struct style_t {
    DISPLAY_DIR display_dir = DISPLAY_DIR::VERTICAL;
    ALIGN horizontal_align_val = ALIGN::START;
    ALIGN vertical_align_val = ALIGN::START;
    glm::vec2 padding = glm::vec2(0);
    glm::vec2 margin = glm::vec2(0);
    float content_spacing = 0;

    glm::vec2 translate = glm::vec2(0);

    BCK_MODE bck_mode = BCK_SOLID;
    glm::vec4 background_color = TRANSPARENT_COLOR;
    glm::vec4 top_left_bck_color = TRANSPARENT_COLOR;
    glm::vec4 top_right_bck_color = TRANSPARENT_COLOR;
    glm::vec4 bottom_right_bck_color = TRANSPARENT_COLOR;
    glm::vec4 bottom_left_bck_color = TRANSPARENT_COLOR;

    BORDER_RADIUS_MODE border_radius_mode = BR_SINGLE_VALUE;
    float border_radius = 0;
    float tl_border_radius = 0;
    float tr_border_radius = 0;
    float bl_border_radius = 0;
    float br_border_radius = 0;

    glm::vec3 color = glm::vec3(1,1,1);

    // width of widget without padding or margins
    WIDGET_SIZE widget_size_width = WIDGET_SIZE::PIXEL_BASED;
    WIDGET_SIZE widget_size_height = WIDGET_SIZE::PIXEL_BASED;
    float width = -1.f;
    float height = -1.f; 

    // hover
    glm::vec3 hover_background_color = TRANSPARENT_COLOR;
    glm::vec3 hover_color = TRANSPARENT_COLOR;
};

struct style_override_t {
    bool display_dir = false;
    bool horizontal_align_val = false;
    bool vertical_align_val = false;
    bool padding = false;
    bool margin = false;
    bool content_spacing = false;

    bool translate = false;

    bool bck_mode = false;
    bool background_color = false;
    bool top_left_bck_color = false;
    bool top_right_bck_color = false;
    bool bottom_right_bck_color = false;
    bool bottom_left_bck_color = false;

    bool border_radius_mode = false;
    bool border_radius = false;
    bool tl_border_radius = false;
    bool tr_border_radius = false;
    bool bl_border_radius = false;
    bool br_border_radius = false;

    bool width = false;
    bool height = false;

    bool color = false;
};

void ui_start_of_frame();
void end_imgui();

void traverse_to_right_focusable();
void traverse_to_left_focusable();

void push_style(style_t& style);
void pop_style();

struct widget_registration_info_t {
    int widget_handle = -1;
    bool clicked_on = false;
    bool hovering_over = false;
};

struct widget_t;

widget_registration_info_t register_widget(widget_t& widget, const char* key, bool push_onto_stack = false);
void register_absolute_widget(widget_t& widget, const char* key, bool push_onto_stack);

enum UI_PROPERTIES;
struct parsed_ui_attributes_t {
    style_t style;
    char id[64]{};
    int font_size = 25;
    UI_PROPERTIES ui_properties;
    char image_path[256]{};
    int z = 0;
};
bool set_parameter_in_style(style_t& style, const char* key, const char* value);
parsed_ui_attributes_t get_style_and_key(xml_attribute** attributes);

struct ui_file_layout_t {
    int handle = -1;
    xml_document* document = NULL;
    time_t last_modified_time = 0;
    char path[256]{};
};
void add_active_ui_file(const char* file_name);
void clear_active_ui_files();
void update_ui_files();

// bool is_some_element_clicked_on();

void set_ui_value(std::string& key, std::string& val);
void draw_from_ui_file_layouts();
void render_ui();

struct ui_mouse_event_t {
    std::string widget_key;
    int z_pos = 0;
    int widget_handle;
};

struct ui_element_status_t {
    ui_mouse_event_t clicked_on;
    ui_mouse_event_t hovered_over;
    ui_mouse_event_t mouse_enter;
    ui_mouse_event_t mouse_left;
};
void clear_element_status(ui_element_status_t& status);
bool get_if_key_clicked_on(const char* key);
bool get_if_key_hovered_over(const char* key);
bool get_if_key_mouse_enter(const char* key);
bool get_if_key_mouse_left(const char* key);

struct ui_anim_t {
    int handle = -1;
    int ui_file_handle = -1;
    char anim_name[128]{};
    style_t style;
    style_override_t style_params_overriden;
    time_count_t anim_duration = .25;
};

struct ui_anim_file_t {
    int handle = -1;
    char path[256]{};
    std::vector<int> ui_anims;
};

struct ui_anim_player_t {
    int handle = -1;
    int ui_anim_handle = -1;
    int ui_anim_file_handle = -1;
    char widget_key[256]{};
    time_count_t anim_duration = 0;
    time_count_t duration_cursor = 0;
    bool playing = false;
};

struct ui_anim_user_info_t {
    char widget_key[128]{};
    char ui_anim_name[128]{};
    time_count_t start_anim_duration_cursor = 0;
    bool start_playing = false;
};

style_t get_intermediate_style(style_t& original_style, ui_anim_player_t& player);
int create_ui_anim_player(const char* widget_key, int ui_anim_file_handle, ui_anim_t& ui_anim, bool play_upon_initialize, time_count_t starting_cursor = 0);
void move_ui_anim_player_forward(ui_anim_player_t& player);
void move_ui_anim_player_backward(ui_anim_player_t& player);
ui_anim_player_t* get_ui_anim_player_by_idx(int idx);
ui_anim_player_t* get_ui_anim_player_by_handle(int handle);
ui_anim_t* get_ui_anim(int handle);
void set_translate_in_ui_anim(const char* anim_name, glm::vec2 translate);
void reset_ui_anim_player_cnt();

void add_ui_anim_to_widget(const char* widget_key, const char* ui_anim_name, time_count_t start_anim_duration_cursor = 0, bool start_playing = false);
void play_ui_anim_player(const char* widget_key, const char* ui_anim_name);
void stop_ui_anim_player(const char* widget_key, const char* ui_anim_name);

void parse_ui_anims(const char* path);
void add_active_ui_anim_file(const char* file_name);
void clear_active_ui_anim_files();

struct ui_info_t {
    std::vector<int> widget_stack;
    std::vector<widget_t> widgets_arr;    
    ui_element_status_t ui_element_status;
};

// enum UI_CONTROL_MODE {
//     UI_CONTROL_NONE = 0,
//     UI_CONTROL_KBD = 1 << 0,
//     UI_CONTROL_CNTLR = 1 << 1,
// };
// OR_ENUM_DECLARATION(UI_CONTROL_MODE);
// AND_ENUM_DECLARATION(UI_CONTROL_MODE);
// AND_W_INT_ENUM_DECLARATION(UI_CONTROL_MODE);

struct ui_state_t {
    // UI_CONTROL_MODE control_mode = UI_CONTROL_KBD | UI_CONTROL_CNTLR;
    bool kbd_controlled = true;
    bool ctrl_controlled = true;
};
void ui_disable_controller_support();
void ui_enable_controller_support();

bool partially_behind_widget(widget_t& widget, widget_t& check_against_widget);