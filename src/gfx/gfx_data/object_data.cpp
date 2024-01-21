#include "object_data.h"
#include "glad/glad.h"

void draw_obj(const render_object_data& data) {
	bind_shader(data.shader);
	bind_vao(data.vao);
	draw_ebo(data.ebo);
	unbind_vao();
	unbind_ebo();
	unbind_shader();
}
