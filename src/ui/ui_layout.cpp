#include "ui_layout.h"

#include "constants.h"
#include "ui/ui.h"
#include "ui_elements.h"

#include <unordered_map>

static std::unordered_map<int, constraint_var_t> constraint_vars;
static std::vector<constraint_t> constraints;

static int constraint_running_cnt = 0;

// extern std::vector<widget_t>* curframe_widget_arr;
// extern std::vector<int>* curframe_widget_stack;
extern ui_info_t* curframe_ui_info;
extern std::vector<style_t> styles_stack;

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

void autolayout_hierarchy() {

    game_assert_msg(curframe_ui_info->widget_stack.size() == 0, "widgets stack for the current frame is not empty when laying out elements");
    game_assert_msg(styles_stack.size() == 1, "styles stack has more than 1 style");

    auto& cur_arr = curframe_ui_info->widgets_arr;
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

helper_info_t resolve_positions(int widget_handle, int x_pos_handle, int y_pos_handle) {
    auto& cur_arr = curframe_ui_info->widgets_arr;
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
    auto& cur_arr = curframe_ui_info->widgets_arr;
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

void clear_constraints() {
    constraint_vars.clear();
    constraints.clear();
    constraint_running_cnt = 0;
}