#include "renderer.h"

#include "window.h"
#include "quad.h"

void render() {
	clear_window();

	// regular stuff
	draw_quad_renders();

	render_window();
}

bool detect_gfx_error() {
	GLenum error = glGetError();
	if (error == GL_NO_ERROR) return false;
	return true;
}