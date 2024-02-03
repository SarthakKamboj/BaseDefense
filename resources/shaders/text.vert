#version 410 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec4 in_color;
layout (location = 2) in vec2 in_tex;

uniform mat4 projection;

out vec3 pos;
out vec2 tex_coord;
out vec4 out_color;

void main() {
    pos = in_pos;
	gl_Position = projection * vec4(in_pos, 1.0);
    tex_coord = in_tex;
    out_color = in_color;
}