#include "ui_elements.h"

#include "gfx/gfx_data/texture.h"
#include "globals.h"

extern globals_t globals;

extern std::vector<style_t> styles_stack;
extern ui_info_t* curframe_ui_info;
// extern std::vector<int>* curframe_widget_stack;
// extern std::vector<widget_t>* curframe_widget_arr;

extern int latest_z_pos;
extern int max_z_pos_visible;

// extern bool stacked_nav_widget_in_stack;

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
    
    widget.style.width = text_dim.width + (2 * widget.style.padding.x);
    widget.style.height = text_dim.max_height_above_baseline + (2 * widget.style.padding.y);

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
    // auto& stack = *curframe_widget_stack;
    auto& stack = curframe_ui_info->widget_stack;
    // if (stacked_nav_widget_in_stack) {
    //     widget.user_handle = user_handle;
    // } else {
    widget.properties = widget.properties | UI_PROP_FOCUSABLE;
    // }

    widget_registration_info_t widget_info = register_widget(widget, key);

    return widget_info.clicked_on;
}

void create_image_container(int texture_handle, float width, float height, WIDGET_SIZE widget_size_width, WIDGET_SIZE widget_size_height, const char* img_name) {

    game_assert(widget_size_width != WIDGET_SIZE::FIT_CONTENT);
    game_assert(widget_size_height != WIDGET_SIZE::FIT_CONTENT);

    texture_t* tex = get_tex(texture_handle);
    game_assert_msg(tex, "texture for image container not found");
    game_assert(tex->tex_slot == 1, "the texture slot for the texture must be 1");

    widget_t widget = create_widget();
    widget.image_based = true;
    widget.texture_handle = texture_handle;
    widget.style.widget_size_width = widget_size_width;
    widget.style.widget_size_height = widget_size_height;
    widget.style.width = width;
    widget.style.height = height;

    register_widget(widget, img_name);
}

widget_t create_widget() {
    widget_t widget;
    widget.style = styles_stack[styles_stack.size() - 1];
    return widget;
}

void pop_widget() {
    auto& arr = curframe_ui_info->widgets_arr;
    auto& stack = curframe_ui_info->widget_stack;
    if (stack.size() > 0) {
        // if (arr[stack[stack.size()-1]].stacked_navigation) {
        //     stacked_nav_widget_in_stack = false;
        // }
        stack.pop_back();
    }
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

widget_t* get_widget(int widget_handle) {
    auto& arr = curframe_ui_info->widgets_arr;
    for (int i = 0; i < arr.size(); i++) {
        if (arr[i].handle == widget_handle) return &arr[i];
    }
    return NULL;
}