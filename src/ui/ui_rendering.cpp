#include "ui_rendering.h"

#include "gfx/renderer.h"
#include "gfx/gfx_data/vertex.h"
#include "gfx/gfx_data/texture.h"

#include <glm/gtx/string_cast.hpp>

#include <string>

extern std::vector<font_mode_t> font_modes;

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

	glm::vec3 origin = glm::vec3(x, y, widget.z_pos);

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

void draw_text(const char* text, glm::vec2 starting_pos, int font_size, glm::vec3& color, int z_pos) {
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

                    updated_vertices[0] = create_vertex(glm::vec3(running_pos.x + (fc.width / 2), running_pos.y + (fc.height / 2), z_pos), color_vec4, glm::vec2(1,0)); // top right
                    updated_vertices[1] = create_vertex(glm::vec3(running_pos.x + (fc.width / 2), running_pos.y - (fc.height / 2), z_pos), color_vec4, glm::vec2(1,1)); // bottom right
                    updated_vertices[2] = create_vertex(glm::vec3(running_pos.x - (fc.width / 2), running_pos.y - (fc.height / 2), z_pos), color_vec4, glm::vec2(0,1)); // bottom left
                    updated_vertices[3] = create_vertex(glm::vec3(running_pos.x - (fc.width / 2), running_pos.y + (fc.height / 2), z_pos), color_vec4, glm::vec2(0,0)); // top left
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

    updated_vertices[0] = create_vertex(glm::vec3(top_left.x + widget.render_width, top_left.y, widget.z_pos), glm::vec4(0,1,1,1), glm::vec2(1,1)); // top right
    updated_vertices[1] = create_vertex(glm::vec3(top_left.x + widget.render_width, top_left.y - widget.render_height, widget.z_pos), glm::vec4(0,0,1,1), glm::vec2(1,0)); // bottom right
    updated_vertices[2] = create_vertex(glm::vec3(top_left.x, top_left.y - widget.render_height, widget.z_pos), glm::vec4(0,1,0,1), glm::vec2(0,0)); // bottom left
    updated_vertices[3] = create_vertex(glm::vec3(top_left.x, top_left.y, widget.z_pos), glm::vec4(1,0,0,1), glm::vec2(0,1)); // top left
    update_vbo_data(font_char_t::ui_opengl_data.vbo, (float*)updated_vertices, sizeof(updated_vertices));

    // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    set_fill_mode();
    // draw the rectangle render after setting all shader parameters
    draw_obj(font_char_t::ui_opengl_data);
}

