#pragma once

#include <vector>
#include <unordered_map>
#include "sys/stat.h"

#include "constants.h"
#include "gfx/gfx_data/object_data.h"
#include "ui_hash.h"
#include "utils/time.h"
#include "utils/xml.h"

#include "glm/glm.hpp"

#define UI_RELOADING 1

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

enum UI_PROPERTIES : int {
    UI_PROP_NONE = 0,
    UI_PROP_CLICKABLE = 1<<0,
    UI_PROP_HOVERABLE = 1<<1,
    UI_PROP_FOCUSABLE = 1<<2,
    UI_PROP_CURRENTLY_FOCUSED = 1<<3
};
OR_ENUM_DECLARATION(UI_PROPERTIES)

enum class WIDGET_SIZE {
    NONE,
    PIXEL_BASED,
    PARENT_PERCENT_BASED,
    FIT_CONTENT
};

struct font_mode_t {
    int font_size = 0;
    std::unordered_map<unsigned char, font_char_t> chars;
    bool used_last_frame = true;
};
void load_font(int font_size);

struct text_dim_t {
	float width = 0;
	// float height = 0;
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

#define TRANSPARENT_COLOR glm::vec3(-1)

enum BCK_MODE {
    BCK_SOLID = 0,
    BCK_GRADIENT_TOP_LEFT_TO_BOTTOM_RIGHT,
    BCK_GRADIENT_4_CORNERS
};

enum BORDER_RADIUS_MODE {
    BR_SINGLE_VALUE = 0,
    BR_4_CORNERS
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
    glm::vec3 background_color = TRANSPARENT_COLOR;
    glm::vec3 top_left_bck_color = TRANSPARENT_COLOR;
    glm::vec3 top_right_bck_color = TRANSPARENT_COLOR;
    glm::vec3 bottom_right_bck_color = TRANSPARENT_COLOR;
    glm::vec3 bottom_left_bck_color = TRANSPARENT_COLOR;

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

struct text_t {
    char text[256]{};
    // TEXT_SIZE text_size = TEXT_SIZE::REGULAR;
    int font_size = 25;
};
void create_text(const char* text, int font_size = 0, bool focusable = false);
bool create_button(const char* text, int font_size = 0, int user_handle = -1, bool* hovering_over = NULL);

struct image_container_t {
    int texture_handle = -1;
    float width = 0.f;
    float height = 0.f;
};
void create_image_container(int texture_handle, float width, float height, WIDGET_SIZE widget_size_width, WIDGET_SIZE widget_size_height, const char* img_name);

bool create_selector(int selected_option, const char** options, int num_options, float width, float height, int& updated_selected_option, const char* selector_summary, int left_arrow_user_handle, int right_arrow_user_handle);

typedef int(*stacked_nav_handler_func_t)(bool right, bool left, bool up, bool down);

struct ui_anim_player_t;
struct widget_t {
    int handle = -1;

    bool text_based = false;
    text_t text_info;

    bool image_based = false;
    int texture_handle = -1;

    std::vector<int> children_widget_handles;
    int parent_widget_handle = NULL;

    char key[256]{};
    hash_t hash;

    UI_PROPERTIES properties = UI_PROPERTIES::UI_PROP_NONE;
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

void ui_start_of_frame();
void end_imgui();

void traverse_to_right_focusable();
void traverse_to_left_focusable();

void push_style(style_t& style);
void pop_style();

void pop_widget();

void create_panel(const char* panel_name);
void end_panel();

void create_container(float width, float height, WIDGET_SIZE widget_size_width, WIDGET_SIZE widget_size_height, const char* container_name, bool focusable = false, stacked_nav_handler_func_t func = NULL, UI_PROPERTIES ui_properties = UI_PROP_NONE);
void end_container();

struct widget_registration_info_t {
    int widget_handle = -1;
    bool clicked_on = false;
    bool hovering_over = false;
};

widget_t create_widget();
widget_registration_info_t register_widget(widget_t& widget, const char* key, bool push_onto_stack = false);

void autolayout_hierarchy();

struct constraint_var_t {
    int handle = -1;
    char name[256]{};
    bool constant = false;
    float* value = NULL;
    float cur_val = 0;
};

struct constraint_term_t {
    float coefficient = 0;
    int var_handle = -1;
};

struct constraint_t {
    int left_side_var_handle;
    std::vector<constraint_term_t> right_side;
    std::vector<int> constant_handles;
};

int create_constraint_var(const char* var_name, float* val);
int create_constraint_var_constant(float value);
constraint_term_t create_constraint_term(int var_handle, float coefficient);
void make_constraint_value_constant(int constraint_handle, float value);

void create_constraint(int constraint_var_handle, std::vector<constraint_term_t>& right_side_terms, float constant);
void resolve_constraints();

void draw_background(widget_t& widget);
void draw_image_container(widget_t& widget);
void draw_text(const char* text, glm::vec2 starting_pos, int font_size, glm::vec3& color);

struct parsed_ui_attributes_t {
    style_t style;
    char id[64]{};
    int font_size = 25;
    UI_PROPERTIES ui_properties = UI_PROP_NONE;
    char image_path[256]{};
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

bool is_some_element_clicked_on();

void set_ui_value(std::string& key, std::string& val);
bool get_if_key_clicked_on(const char* key);
bool get_if_key_hovered_over(const char* key);

void draw_from_ui_file_layouts();
void render_ui();

struct bck_color_override_t {
    char widget_key[256]{};
    BCK_MODE bck_mode = BCK_SOLID;
    glm::vec3 background_color = TRANSPARENT_COLOR;
    glm::vec3 top_left_bck_color = TRANSPARENT_COLOR;
    glm::vec3 top_right_bck_color = TRANSPARENT_COLOR;
    glm::vec3 bottom_right_bck_color = TRANSPARENT_COLOR;
    glm::vec3 bottom_left_bck_color = TRANSPARENT_COLOR;
};
void set_background_color_override(const char* widget_key, glm::vec3 color);
void set_background_color_gradient_4_corners_override(const char* widget_key, glm::vec3 top_left_color, glm::vec3 bottom_right_color);

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
};

style_t get_intermediate_style(style_t& original_style, ui_anim_player_t& player);
int create_ui_anim_player(const char* widget_key, int ui_anim_file_handle, ui_anim_t& ui_anim, bool play_upon_initialize);
void move_ui_anim_player_forward(ui_anim_player_t& player);
void move_ui_anim_player_backward(ui_anim_player_t& player);
ui_anim_player_t* get_ui_anim_player(int handle);
ui_anim_t* get_ui_anim(int handle);

void add_ui_anim_to_widget(const char* widget_key, const char* ui_anim_name);
void play_ui_anim_player(const char* widget_key, const char* ui_anim_name);
void stop_ui_anim_player(const char* widget_key, const char* ui_anim_name);

void parse_ui_anims(const char* path);
void add_active_ui_anim_file(const char* file_name);
void clear_active_ui_anim_files();
