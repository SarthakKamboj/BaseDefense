#version 410 core

in vec2 tex_coord;
in vec2 local_rel_pos;

out vec4 frag_color;

uniform sampler2D selector_image_tex;
uniform vec2 selection_point;
uniform vec3 color;

void main() {
    vec4 tex_color = texture(selector_image_tex, tex_coord);
    float cos_theta = dot(normalize(selection_point), normalize(local_rel_pos));
    bool in_section = cos_theta > cos(radians(60));
    frag_color = tex_color;
    if (in_section) {
        frag_color = tex_color * vec4(color, 1);
    }
}