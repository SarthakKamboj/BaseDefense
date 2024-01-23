#version 410 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec4 in_color;
layout (location = 2) in vec2 in_tex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 tex_coord;
out vec2 local_rel_pos;

void main() {
	gl_Position = projection * view * model * vec4(in_pos, 1.0);
    tex_coord = in_tex; 
    local_rel_pos = in_pos.xy;
}