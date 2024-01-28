#pragma once

#include <vector>

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

struct helper_info_t {
    int content_width = 0;
    int content_height = 0;
};

helper_info_t resolve_positions(int widget_handle, int x_pos_handle, int y_pos_handle);
helper_info_t resolve_dimensions(int cur_widget_handle, int parent_width_handle, int parent_height_handle);

void autolayout_hierarchy();
void clear_constraints();