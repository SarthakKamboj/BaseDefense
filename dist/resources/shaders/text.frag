#version 410 core

out vec4 frag_color;

in vec2 tex_coord;
in vec2 pos;
in vec4 out_color;

uniform bool round_vertices;

uniform vec3 top_left;
uniform vec3 top_right;
uniform vec3 bottom_left;
uniform vec3 bottom_right;

uniform float tl_border_radius;
uniform float tr_border_radius;
uniform float bl_border_radius;
uniform float br_border_radius;

uniform sampler2D character_tex;
uniform sampler2D image_tex;

// uniform vec3 color;
uniform float tex_influence;
uniform int is_character_tex;

void main() {
    vec4 color = out_color;
    float t = tex_influence;
    float c = texture(character_tex, tex_coord).r;
    vec4 character_tex_part = color * vec4(1,1,1,c);
    vec4 image_tex_part = texture(image_tex, tex_coord);
    vec4 tex_part = (is_character_tex * character_tex_part) + ((1 - is_character_tex) * image_tex_part);
    frag_color = (t * tex_part) + ((1-t) * color);

    if (round_vertices) {
        vec3 top_left_inner = top_left + vec3(tl_border_radius, -tl_border_radius, 0);
        vec3 top_right_inner = top_right + vec3(-tr_border_radius, -tr_border_radius, 0);
        vec3 bottom_left_inner = bottom_left + vec3(bl_border_radius, bl_border_radius, 0);
        vec3 bottom_right_inner = bottom_right + vec3(-br_border_radius, br_border_radius, 0);

        if (pos.x >= top_right_inner.x && pos.y >= top_right_inner.y) {
            vec2 pos_rel_inner = pos - top_right_inner.xy;
            if (dot(pos_rel_inner, pos_rel_inner) >= tr_border_radius * tr_border_radius) {
                frag_color = vec4(0,0,0,0);
            }
        } else if (pos.x <= top_left_inner.x && pos.y >= top_left_inner.y) {
            vec2 pos_rel_inner = pos - top_left_inner.xy;
            if (dot(pos_rel_inner, pos_rel_inner) >= tl_border_radius * tl_border_radius) {
                frag_color = vec4(0,0,0,0);
            }
        } else if (pos.x >= bottom_right_inner.x && pos.y <= bottom_right_inner.y) {
            vec2 pos_rel_inner = pos - bottom_right_inner.xy;
            if (dot(pos_rel_inner, pos_rel_inner) >= br_border_radius * br_border_radius) {
                frag_color = vec4(0,0,0,0);
            }
        } else if (pos.x <= bottom_left_inner.x && pos.y <= bottom_left_inner.y) {
            vec2 pos_rel_inner = pos - bottom_left_inner.xy;
            if (dot(pos_rel_inner, pos_rel_inner) >= bl_border_radius * bl_border_radius) {
                frag_color = vec4(0,0,0,0);
            }
        }
    }
}