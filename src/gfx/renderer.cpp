#include "renderer.h"

#include "window.h"
#include "quad.h"
#include "ui/ui.h"
#include "gameplay/bck.h"
#include "gameplay/gos.h"
#include "gameplay/preview_manager.h"
#include "utils/time.h"
#include "globals.h"
#include "utils/general.h"

#include <string>

extern globals_t globals;

extern PREVIEW_MODE preview_mode;

void render() {
	clear_window();

	if (globals.scene_manager.cur_level > LEVEL_5 || globals.scene_manager.cur_level < 0) {
		draw_from_ui_file_layouts();
		render_ui();
		render_window();
		return;
	}

	// regular stuff
	draw_quad_renders();
	render_preview_mode();
	draw_from_ui_file_layouts();
	render_ui();
	render_window();
}

bool detect_gfx_error() {
	GLenum error = glGetError();
	if (error == GL_NO_ERROR) return false;
	return true;
}

void set_fill_mode() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void set_wireframe_mode() {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}