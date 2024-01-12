#version 410 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_tex;

uniform mat4 projection;

out vec2 pos;
out vec2 tex_coord;

void main() {
    pos = in_pos.xy;
	gl_Position = projection * vec4(in_pos.xy, 0.0, 1.0);
    tex_coord = in_tex;
}